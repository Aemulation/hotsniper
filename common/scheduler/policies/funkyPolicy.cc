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
          state(State::INIT){
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

    std::cout << "Requirement " << taskCoreRequirement << std::endl;

    for (uint i = 0; i < taskCoreRequirement; i++) {
        if(i >= bCores.size()){
            std::vector<int> empty;
            return empty;
        } else {
            std::cout << "[Scheduler][FunkyPolicy]: Core " << bCores.at(i) << " is available and big" << std::endl;
            cores.push_back(bCores.at(i));
        }
    }

    return cores;
}

std::vector<migration> FunkyPolicy::migrate(
        SubsecondTime time,
        const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores) {
    std::cout << "[Scheduler][FunkyPolicy]: Migrate" << std::endl;
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

    // if (hotBigCores > 0) {
    //   if (coldBigCores > 0) {
    //     // 2
    //     migration mi = bigBigMigration(availableCores);
    //     migrations.push_back(mi);
    //     availableCores.at(mi.toCore) = false;
    //     availableCores.at(mi.fromCore) = true;
    //     goto start;
    //   } else {
    //     // 3
    //     int dvfsPerformance = 0;
    //     int migrationPerformance = 1;

    //     if (migrationPerformance > dvfsPerformance) {
    //       // 4
    //       migration mi = bigSmallMigration(availableCores);
    //       migrations.push_back(mi);
    //       availableCores.at(mi.toCore) = false;
    //       availableCores.at(mi.fromCore) = true;
    //       goto start;
    //     } else {
    //       // 5
    //       // todo do dvfs
    //     }
    //   }
    // }

    if(state == State::INIT){
        if (hotBigCores > 0) {
            if (coldBigCores > 0) {
                migration mi = bigBigMigration(availableCores);
                migrations.push_back(mi);
                availableCores.at(mi.toCore) = false;
                availableCores.at(mi.fromCore) = true;
            } else {
                FUNKY_PRINT << "Big cores are hot, but no cold big cores found. Entering cooling phase." << std::endl;
                if (nMig == -1) {
                    // TODO do at start
                    state = State::ESTIMATION;
                    estimationState = EstimationState::START;
                } else if (nMig > nDVFS) {
                    state = State::BIG_SMALL_MIGRATION;
                } else {
                    state = State::DVFS;
                }

            }
        }
    }

    if(state == State::BIG_SMALL_MIGRATION){
        migrations = bigSmallMigration(availableCores);
    } else if(state == State::ESTIMATION){
        migrations = estimation(time, availableCores);
    } else if (state == State::DVFS){
        std::vector<int> bCores = bigCores();
        bool allBigCoresCold = true;
        for(int c : bCores){
            if(performanceCounters->getTemperatureOfCore(c) > criticalTemperature){
                allBigCoresCold = false;
                break;
            }
        }

        if (allBigCoresCold) {
            // set back freqs
            for(int c : bCores){
                fCores.at(c) = maxFrequencyBig;
            }
            state == State::INIT;
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
    // FUNKY_PRINT << "Setting frequencies: ";

    // for (int c = 0; c < coreRows * coreColumns; c++) {
    //     std::cout << (isBig(c) ? "B" : "S") << c << " " << fCores.at(c) << " ";

        // if(isBig(c) && oldFrequencies.size() > c){
        //     if(fCores.at(c) == oldFrequencies.at(c)){
        //         // TODO: Do something smarter here
        //         tBigFreqs.at(c) += 1;
        //     } else {
        //         tBigFreqs.at(c) = 0;
        //     }
        // }
    // }

    // std::cout << std::endl;

    return fCores;
}

int FunkyPolicy::getColdestCore(const std::vector<bool> &availableCores) {
    int coldestCore = -1;
    float coldestTemperature = 0;

    // iterate all cores to find coldest
    for (uint c = 0; c < coreRows * coreColumns; c++) {
        if (availableCores.at(c)) {
            float temperature = performanceCounters->getTemperatureOfCore(c);
            if ((coldestCore == -1) || (temperature < coldestTemperature)) {
                coldestCore = c;
                coldestTemperature = temperature;
            }
        }
    }

    return coldestCore;
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

        estimationState = EstimationState::SMALLEST_AT_MAX;
    } else if(estimationState == EstimationState::SMALLEST_AT_MAX){
        for(int c : sCores){
            CPIHighestSmall = max(CPIHighestSmall, performanceCounters->getCPIOfCore(c));
        }

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
            FUNKY_PRINT << "Core " << c << " set to " << maxFrequencyBig << " MHz" << std::endl;
        }

        tHighestBigStart = time;

        estimationState = EstimationState::BIGGEST_AT_MAX;
    } else if(estimationState == EstimationState::BIGGEST_AT_MAX){
        for (int c : bCores){
            CPIHighestBig = max(CPIHighestBig, performanceCounters->getCPIOfCore(c));
        }
        
        for(int c : bCores){
            if(performanceCounters->getTemperatureOfCore(c) > criticalTemperature){
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
                tDVFSStart = time;
                tDVFSCycleStart = time;

                FUNKY_PRINT << "Entering DVFS estimation phase" << std::endl;
                goto estimationFinal;
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

        if(allBigCoresCold){
            FUNKY_PRINT << "All big cores are cold again" << std::endl;

            for(int c : bCores){
                fCores.at(c) = maxFrequencyBig;
                FUNKY_PRINT << "Core " << c << " set to " << maxFrequencyBig << " MHz" << std::endl;
            }
            
            // store values
            double maxCPIBig = 0;
            for(int c : bCores){
                maxCPIBig = max(maxCPIBig, performanceCounters->getCPIOfCore(c));
            }
            
            tBigFreqs.push_back(time - tDVFSCycleStart);
            fBig.push_back(fCores.at(bCores.at(0)));

            if(maxCPIBig > 100){
                CPIBig.push_back(maxCPIBig);
            } else {
                CPIBig.push_back(CPIBig.back());
            }

            tHighestBigStart = time;

            estimationState = EstimationState::DVFS_BIGGEST_AT_MAX;
        } else {
            double maxCPIBig = 0;
            for(int c : bCores){
                maxCPIBig = max(maxCPIBig, performanceCounters->getCPIOfCore(c));
            }

            tBigFreqs.push_back(time - tDVFSCycleStart);
            fBig.push_back(fCores.at(bCores.at(0)));

            if(maxCPIBig > 100){
                CPIBig.push_back(maxCPIBig);
            } else {
                CPIBig.push_back(CPIBig.back());
            }
            
            tDVFSCycleStart = time;
            int newFreq = max(minFrequencyBig, (int)(fCores.at(bCores.at(0)) * 0.8));
            for(int c : bCores){
                fCores.at(c) = newFreq;
            }
        }
    } else if(estimationState == EstimationState::DVFS_BIGGEST_AT_MAX){
        for(int c : sCores){
            CPIHighestBig = max(CPIHighestBig, performanceCounters->getCPIOfCore(c));
        }
        
        for(int c : bCores){
            if(performanceCounters->getTemperatureOfCore(c) > criticalTemperature){

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
                state = State::INIT;

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

        bigSmallMigrationState = BigSmallMigrationState::BIGGEST_ARE_COLD;
    } else if(bigSmallMigrationState == BigSmallMigrationState::BIGGEST_ARE_COLD){
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
        state = State::INIT;
    } 

    bigSmallMigrationFinal:
    return migrations;
}