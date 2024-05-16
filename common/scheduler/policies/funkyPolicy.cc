#include <iomanip>
#include "funkyPolicy.h"

using namespace std;

#define FUNKY_PRINT std::cout << "[Scheduler][FunkyPolicy]: "


std::vector<int> FunkyPolicy::fCores;

FunkyPolicy::FunkyPolicy(
        const PerformanceCounters *performanceCounters,
        int coreRows,
        int coreColumns,
        float criticalTemperature,
        int minFrequencyBig,
        int maxFrequencyBig,
        int minFrequencySmall,
        int maxFrequencySmall)
        : performanceCounters(performanceCounters),
          coreRows(coreRows),
          coreColumns(coreColumns),
          criticalTemperature(criticalTemperature),
          minFrequencyBig(minFrequencyBig),
          maxFrequencyBig(maxFrequencyBig),
          minFrequencySmall(minFrequencySmall),
          maxFrequencySmall(maxFrequencySmall),
          bigSmallMigrationState(BigSmallMigrationState::DONE),
          estimationState(EstimationState::DONE),
          state(State::INIT),
          nMig(-1),
          nDVFS(-1){
            nMig = -1;
            fCores = std::vector<int>();
            for(int i = 0; i < coreRows * coreColumns; i++){
                if(isBig(i)){
                    fCores.push_back(maxFrequencyBig);
                } else {
                    fCores.push_back(maxFrequencySmall);
                }
            }
          }

std::vector<int> FunkyPolicy::map(
        String taskName,
        int taskCoreRequirement,
        const std::vector<bool> &availableCoresRO,
        const std::vector<bool> &activeCores) {

    std::vector<int> cores;
    std::vector<int> bCores = bigCores();
    std::vector<int> sCores = smallCores();

    std::cout << "Requirement " << taskCoreRequirement << std::endl;

    for (uint i = 0; i < taskCoreRequirement; i++) {
        if(i >= bCores.size()){
            std::vector<int> empty;
            return empty;
        } else {
            FUNKY_PRINT << "Core " << bCores.at(i) << " is available and big" << std::endl;
            cores.push_back(bCores.at(i));
        }
    }

    return cores;
}

std::vector<migration> FunkyPolicy::migrate(
        SubsecondTime time,
        const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores) {
    std::vector<migration> migrations;
    std::vector<bool> availableCores(coreRows * coreColumns);
    int bigCoresamount = coreRows * coreColumns / 2;
    int hotBigCores, coldBigCores;
    hotBigCores = 0;

    // 1
    for (uint c = 0; c < coreRows * coreColumns; c++) {
        availableCores.at(c) = taskIds.at(c) == -1;

        // check the number of hot and cold big cores
        if (isBig(c) && performanceCounters->getTemperatureOfCore(c) > criticalTemperature) {
            hotBigCores++;
        }
    }

    logTemperatures(availableCores);

    coldBigCores = bigCoresamount - hotBigCores;

    if(state == State::INIT){
        if (hotBigCores > 0) {
            if (coldBigCores > 0) {
                migration mi = bigBigMigration(availableCores);
                migrations.push_back(mi);
                availableCores.at(mi.toCore) = false;
                availableCores.at(mi.fromCore) = true;
            } else {
                FUNKY_PRINT << "Big cores are hot, but no cold big cores found. Entering cooling phase." << std::endl;

                if(time - tLastEstimation > SubsecondTime::MS(100) || nMig < 0){
                    state = State::ESTIMATION;
                    estimationState = EstimationState::START;
                } else {
                    state = nMig > nDVFS ? State::BIG_SMALL_MIGRATION : State::DVFS;

                    if(state == State::BIG_SMALL_MIGRATION){
                        bigSmallMigrationState = BigSmallMigrationState::START;
                    }
                }
            }
        }
    }

    if(state == State::BIG_SMALL_MIGRATION){
        if(bigSmallMigrationState == BigSmallMigrationState::DONE){
            if (hotBigCores > 0) {
                bigSmallMigrationState = BigSmallMigrationState::START;
            } else {
                state = State::INIT;
            }
        }

        migrations = bigSmallMigration(availableCores);
    } else if(state == State::ESTIMATION){
        migrations = estimation(time, availableCores);

        if(estimationState == EstimationState::DONE){
            FUNKY_PRINT << "Estimation done, selected " << (nMig > nDVFS ? "big-small migration" : "DVFS") << " policy" << std::endl;

            state = State::INIT;
        }
    } else if (state == State::DVFS){
        std::vector<int> bCores = bigCores();

        if (hotBigCores == 0) {
            for(int c : bCores){
                fCores.at(c) = maxFrequencyBig;
            }
            
            state = State::INIT;
        } else {
            // reduce freqs
            int newFreq = max(minFrequencyBig, (int)(fCores.at(bCores.at(0)) * 0.8));
            for(int c : bCores){
                fCores.at(c) = newFreq;
            }
        }
    }

    return migrations;
}

std::vector<int> FunkyPolicy::getFrequencies(
        const std::vector<int> &oldFrequencies,
        const std::vector<bool> &activeCores){
    return fCores;
}

void FunkyPolicy::logTemperatures(const std::vector<bool> &availableCores) {
    FUNKY_PRINT << "Temperatures of available cores:" << endl;
    for (uint y = 0; y < coreRows; y++) {
        for (uint x = 0; x < coreColumns; x++) {
            int coreId = y * coreColumns + x;
            cout << (isBig(coreId) ? "B" : "S") << coreId;

            if (x > 0) {
                cout << " ";
            }

            float temperature = performanceCounters->getTemperatureOfCore(coreId);
            cout << " " << fixed << setprecision(1) << temperature << " ";
        }
        cout << endl;
    }
}

bool FunkyPolicy::isBig(const uint core) {
    if (core < coreRows * coreColumns / 2) {
        return true;
    }
    return false;
}

std::vector<int> FunkyPolicy::bigCores() {
    std::vector<int> cores;
    for (uint c = 0; c < coreRows * coreColumns; c++) {
        if (isBig(c)) {
            cores.push_back(c);
        }
    }
    return cores;
}

std::vector<int> FunkyPolicy::smallCores() {
    std::vector<int> cores;
    for (uint c = 0; c < coreRows * coreColumns; c++) {
        if (!isBig(c)) {
            cores.push_back(c);
        }
    }
    return cores;
}

migration FunkyPolicy::bigBigMigration(const std::vector<bool> &availableCores){
    int coreAboveThreshold = -1;
    int coldestCore = -1;
    float coldestTemperature = 0;
    std::vector<int> bCores = bigCores();

    for(int c : bCores){
        float temperature = performanceCounters->getTemperatureOfCore(c);

        if(temperature > criticalTemperature){
            coreAboveThreshold = c;
            std::cout << "[Scheduler][FunkyPolicy]: Big core " << c << " is too hot!!!" << std::endl;
        }

        if ((coldestCore == -1) || (temperature < coldestTemperature)) {
            coldestCore = c;
            coldestTemperature = temperature;
        }
    }

    if(coreAboveThreshold != -1){
        migration m;
        m.fromCore = coreAboveThreshold;
        m.toCore = coldestCore;
        m.swap = true;
        return m;
    } else {
        throw std::runtime_error("No big core above threshold found");
    }
}

std::vector<migration> FunkyPolicy::estimation(SubsecondTime time, const std::vector<bool> &availableCores){
    std::vector<migration> migrations;
    std::vector<int> bCores = bigCores();
    std::vector<int> sCores = smallCores();

    if(estimationState == EstimationState::START){
        tLastEstimation = time;
        tMigStart = time;
        CPIHighestSmall = 0;
        CPIHighestBig = 0;

        int curSmallCore = 0;

        for(int c : bCores){
            migration m;

            m.fromCore = c;
            m.toCore = sCores.at(curSmallCore);
            m.swap = false;
            migrations.push_back(m);

            curSmallCore++;
            curSmallCore %= sCores.size();

            fCores.at(c) = minFrequencyBig;
            fCores.at(sCores.at(curSmallCore)) = maxFrequencySmall;
            
            FUNKY_PRINT << c << " set to " << minFrequencyBig << " MHz" << std::endl;
        }

        measurements = 0;
        estimationState = EstimationState::SMALLEST_AT_MAX;
    } else if(estimationState == EstimationState::SMALLEST_AT_MAX){
        for(int c : sCores){
            CPIHighestSmall += performanceCounters->getCPIOfCore(c) < 100 ? performanceCounters->getCPIOfCore(c) : 1;
            FUNKY_PRINT << "CPI of core " << c << ": " << performanceCounters->getCPIOfCore(c) << std::endl;
        }

        measurements++;

        for(int c : bCores){
            if(performanceCounters->getTemperatureOfCore(c) > criticalTemperature){
                goto estimationFinal;
            }
        }

        tHighestSmall = time - tMigStart;
        std::cout << "[Scheduler][FunkyPolicy]: All big cores are cold" << std::endl;

        int curBigCore = 0;

        for(int c : sCores){
            migration m;

            m.fromCore = c;
            m.toCore = bCores.at(curBigCore);
            m.swap = false;
            migrations.push_back(m);
            curBigCore++;
            curBigCore %= bCores.size();
        }

        for(int c : bCores){
            fCores.at(c) = maxFrequencyBig;
            FUNKY_PRINT << "Core S" << c << " set to " << maxFrequencyBig << " MHz" << std::endl;
        }

        tHighestBigStart = time;

        CPIHighestSmall /= sCores.size() * measurements;
        measurements = 0;
        estimationState = EstimationState::BIGGEST_AT_MAX;
    } else if(estimationState == EstimationState::BIGGEST_AT_MAX){
        for (int c : bCores){
            CPIHighestBig += performanceCounters->getCPIOfCore(c) < 100 ? performanceCounters->getCPIOfCore(c) : 1;
            FUNKY_PRINT << "CPI of core B" << c << ": " << performanceCounters->getCPIOfCore(c) << std::endl;
        }

        measurements++;
        
        for(int c : bCores){
            if(performanceCounters->getTemperatureOfCore(c) > criticalTemperature){
                CPIHighestBig /= bCores.size() * measurements;

                FUNKY_PRINT << "[Scheduler][FunkyPolicy]: Big core " << c << " is too hot" << std::endl;

                tMig = time - tMigStart;
                tHighestBig = time - tHighestBigStart;
                fHighestBig = maxFrequencyBig;
                fHighestSmall = maxFrequencySmall;
                
                nMig = ((fHighestSmall * tHighestSmall.getUS()) / CPIHighestSmall + (fHighestBig * tHighestBig.getUS()) / CPIHighestBig) / tMig.getUS();
                
                FUNKY_PRINT << "tHighestSmall: " << tHighestSmall.getUS() << " tHighestBig: " << tHighestBig.getUS() << " tMig: " << tMig.getUS() << std::endl;
                FUNKY_PRINT << "fHighestSmall: " << fHighestSmall << " fHighestBig: " << fHighestBig << std::endl;
                FUNKY_PRINT << "CPIHighestSmall: " << CPIHighestSmall << " CPIHighestBig: " << CPIHighestBig << std::endl;
                FUNKY_PRINT << "tMig: " << tMig.getUS() << std::endl;

                FUNKY_PRINT << "nMig: " << setprecision(10) << nMig << std::endl;

                estimationState = EstimationState::DVFS;
                measurements = 0;
                tDVFSStart = time;
                tDVFSCycleStart = time;
                CPIHighestBig = 0;
                CPIHighestSmall = 0;

                FUNKY_PRINT << "Entering DVFS estimation phase" << std::endl;
            }
        }
    } else if(estimationState == EstimationState::DVFS){
        bool allBigCoresCold = true;
        for(int c : bCores){
            if(performanceCounters->getTemperatureOfCore(c) > criticalTemperature){
                allBigCoresCold = false;
                break;
            }
        }

        double avgCPIBig = 0;
        for(int c : bCores){
            FUNKY_PRINT << "CPI of core B" << c << ": " << performanceCounters->getCPIOfCore(c) << std::endl;
            avgCPIBig += performanceCounters->getCPIOfCore(c) < 100 ? performanceCounters->getCPIOfCore(c) : 1;
        }

        avgCPIBig /= bCores.size();
        FUNKY_PRINT << "Avg CPI of big cores: " << avgCPIBig << std::endl;

        tBigFreqs.push_back(time - tDVFSCycleStart);
        fBig.push_back(fCores.at(bCores.at(0)));
        CPIBig.push_back(avgCPIBig);

        if(allBigCoresCold){
            FUNKY_PRINT << "All big cores are cold again" << std::endl;

            for(int c : bCores){
                fCores.at(c) = maxFrequencyBig;
                FUNKY_PRINT << "Core " << c << " set to " << maxFrequencyBig << " MHz" << std::endl;
            }
            
            tHighestBigStart = time;
            estimationState = EstimationState::DVFS_BIGGEST_AT_MAX;
        } else {
            int newFreq = max(minFrequencyBig, (int)(fCores.at(bCores.at(0)) * 0.8));
            for(int c : bCores){
                fCores.at(c) = newFreq;
            }
            tDVFSCycleStart = time;
        }
    } else if(estimationState == EstimationState::DVFS_BIGGEST_AT_MAX){
        for(int c : sCores){
            CPIHighestBig += performanceCounters->getCPIOfCore(c) < 100 ? performanceCounters->getCPIOfCore(c) : 1;
        }

        measurements++;
        
        for(int c : bCores){
            if(performanceCounters->getTemperatureOfCore(c) > criticalTemperature){
                CPIHighestBig /= sCores.size() * measurements;

                for(uint i = 0; i < tBigFreqs.size(); i++){
                    FUNKY_PRINT << "tBigFreqs: " << tBigFreqs.at(i).getUS() << " CPIBig: " << CPIBig.at(i) << " fBig: " << fBig.at(i) << std::endl;
                }

                SubsecondTime tDVFS = time - tDVFSStart;
                tHighestBig = time - tHighestBigStart;

                FUNKY_PRINT << "tHighestBig: " << tHighestBig.getUS() << " tDVFS: " << tDVFS.getUS() << std::endl;

                nDVFS = 0;
                for(uint i = 0; i < tBigFreqs.size(); i++){
                    nDVFS += (fBig.at(i) / CPIBig.at(i)) * tBigFreqs.at(i).getUS();
                }

                nDVFS += fHighestBig * tHighestBig.getUS() / CPIHighestBig;
                nDVFS /= tDVFS.getUS(); 

                FUNKY_PRINT << "nDVFS: " << setprecision(10) << nDVFS << std::endl;
                FUNKY_PRINT << "nMig: " << setprecision(10) << nMig << std::endl;

                estimationState = EstimationState::DONE;

                goto estimationFinal;
            }
        }
    }

    estimationFinal:
    return migrations;
}

std::vector<migration> FunkyPolicy::bigSmallMigration(const std::vector<bool> &availableCores){
    std::vector<migration> migrations;

    std::vector<int> bCores = bigCores();
    std::vector<int> sCores = smallCores();

    if(bigSmallMigrationState == BigSmallMigrationState::START){
        FUNKY_PRINT << "Starting big-small migration" << std::endl;

        int curSmallCore = 0;

        for(int c : bCores){
            migration m;

            m.fromCore = c;
            m.toCore = sCores.at(curSmallCore);
            m.swap = false;
            migrations.push_back(m);
            curSmallCore++;
            curSmallCore %= sCores.size();

            fCores.at(c) = minFrequencyBig;
            FUNKY_PRINT << "Core " << c << " set to " << minFrequencyBig << " MHz" << std::endl;
        }

        bigSmallMigrationState = BigSmallMigrationState::SMALLEST_AT_MAX;
    } else if(bigSmallMigrationState == BigSmallMigrationState::SMALLEST_AT_MAX){
        for(int c : bCores){
            if(performanceCounters->getTemperatureOfCore(c) > criticalTemperature){
                goto bigSmallMigrationFinal;
            }
        }

        FUNKY_PRINT << "All big cores are cold" << std::endl;

        int curBigCore = 0;

        for(int c : sCores){
            migration m;

            m.fromCore = c;
            m.toCore = bCores.at(curBigCore);
            m.swap = false;
            migrations.push_back(m);
            curBigCore++;
            curBigCore %= bCores.size();
        }

        for(int c : bCores){
            fCores.at(c) = maxFrequencyBig;
            std::cout << "[Scheduler][FunkyPolicy]: Core " << c << " set to " << maxFrequencyBig << " MHz" << std::endl;
        }

        bigSmallMigrationState = BigSmallMigrationState::DONE;
    } 

    bigSmallMigrationFinal:
    return migrations;
}