#include "coldestCore.h"
#include <iomanip>

using namespace std;

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
          state(State::INIT){}

std::vector<int> FunkyPolicy::map(
        String taskName,
        int taskCoreRequirement,
        const std::vector<bool> &availableCoresRO,
        const std::vector<bool> &activeCores) {

    std::vector<int> bigCores = bigCores(availableCoresRO);
}

std::vector<migration> FunkyPolicy::migrate(
        SubsecondTime time,
        const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores) {


    std::vector<migration> migrations;
    std::vector<bool> availableCores(coreRows * coreColumns);
    int bigCoresamount = coreRows * coreColumns / 2;
    int hotBigCores, coldBigCores;
    start:
    hotBigCores = 0;

    logTemperatures(availableCores);

    // 1
    for (int c = 0; c < coreRows * coreColumns; c++) {
        availableCores.at(c) = taskIds.at(c) == -1;

        // check the number of hot and cold big cores
        if (isBig(c) && performanceCounters->getTemperatureOfCore(c) > criticalTemperature) {
            hotBigCores++;
        }
    }

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
                state = State::ESTIMATION;
            }
        }
    }

    if(state == State::BIG_SMALL_MIGRATION){
        migrations = bigSmallMigration(availableCores);
    } else if(state == State::ESTIMATION){
        // TODO: DVFS estimation
        migrations = estimation(time, availableCores);
    }

    return migrations;
}

std::vector<int> getFrequencies(
        const std::vector<int> &oldFrequencies,
        const std::vector<bool> &activeCores){
    std::vector<int> frequencies(coreRows * coreColumns);

    for (int c = 0; c < coreRows * coreColumns; c++) {
        frequencies.at(c) = fCores.at(c);

        if(isBig(c)){
            if(frequencies.at(c) == oldFrequencies.at(c)){
                // TODO: Do something smarter here
                tBigFreqs.at(c) += 1;
            } else {
                tBigFreqs.at(c) = 0;
            }
        }
    }

    return frequencies;
}

int FunkyPolicy::getColdestCore(const std::vector<bool> &availableCores) {
    int coldestCore = -1;
    float coldestTemperature = 0;

    // iterate all cores to find coldest
    for (int c = 0; c < coreRows * coreColumns; c++) {
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
    cout << "[Scheduler][coldestCore-map]: temperatures of available cores:" << endl;
    for (int y = 0; y < coreRows; y++) {
        for (int x = 0; x < coreColumns; x++) {
            int coreId = y * coreColumns + x;
            cout << isBig(coreId) ? "B" : "S" << coreId;

            if (x > 0) {
                cout << " ";
            }
            if (!availableCores.at(coreId)) {
                cout << " - ";
            } else {
                float temperature = performanceCounters->getTemperatureOfCore(coreId);
                cout << fixed << setprecision(1) << temperature;
            }
        }
        cout << endl;
    }
}

bool FunkyPolicy::isBig(const int core) {
    if (core < coreRows * coreColumns / 2) {
        return true;
    }
    return false;
}

std::vector<int> FunkyPolicy::bigCores(const std::vector<bool> &availableCores) {
    std::vector<int> bigCores;
    for (int c = 0; c < coreRows * coreColumns; c++) {
        if (availableCores.at(c) && isBig(c)) {
            bigCores.push_back(c);
        }
    }
    return bigCores;
}

std::vector<int> FunkyPolicy::smallCores(const std::vector<bool> &availableCores) {
    std::vector<int> smallCores;
    for (int c = 0; c < coreRows * coreColumns; c++) {
        if (availableCores.at(c) && !isBig(c)) {
            smallCores.push_back(c);
        }
    }
    return smallCores;
}

Optional<migration> FunkyPolicy::bigBigMigration(const std::vector<bool> &availableCores){
    int coreAboveThreshold = -1;
    int coldestCore = -1;
    float coldestTemperature = 0;
    std::vector<int> bigCores = bigCores(availableCores);

    for(int c : bigCores){
        float temperature = performanceCounters->getTemperatureOfCore(c);

        if(temperature > criticalTemperature){
            coreAboveThreshold = c;
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
        m.swap = false;
        return m;
    } else {
        throw std::runtime_error("No big core above threshold found");
    }
}

std::vector<migration> FunkyPolicy::estimation(SubsecondTime time, const std::vector<bool> &availableCores){
    std::vector<migration> migrations;

    if(estimationState == EstimationState::START){
        tMigStart = time;
        CPIHighestSmall = 0;
        CPIHighestBig = 0;

        std::vector<int> bigCores = bigCores(availableCores);
        std::vector<int> smallCores = smallCores(availableCores);

        int curSmallCore = 0;

        for(int c : bigCores){
            migration m;

            m.fromCore = c;
            m.toCore = smallCores.at(curSmallCore);
            m.swap = false;
            migrations.push_back(m);
            curSmallCore++ %= smallCores.size();

            fCores.at(c) = minFrequencyBig;
            std::cout << "[Scheduler][FunkyPolicy]: Core " << c << " set to " << minFrequencyBig << " MHz" << std::endl;
        }

        estimationState = EstimationState::SMALLEST_AT_MAX;
    } else if(estimationState == EstimationState::SMALLEST_AT_MAX){
        std::vector<int> bigCores = bigCores(availableCores);
        std::vector<int> smallCores = smallCores(availableCores);

        for(int c : smallCores){
            CPIHighestSmall = max(CPIHighestSmall, PerformanceCounters.getCPIOfCore(c));
        }

        tHighestSmall = time - tMigStart;

        for(int c : bigCores){
            if(PerformanceCounters.getTemperatureOfCore(c) > criticalTemperature){
                goto estimationFinal;
            }
        }

        std::cout << "[Scheduler][FunkyPolicy]: All big cores are cold" << std::endl;

        estimationState = EstimationState::BIGGEST_ARE_COLD;
    } else if(estimationState == EstimationState::BIGGEST_ARE_COLD){
        std::vector<int> bigCores = bigCores(availableCores);
        std::vector<int> smallCores = smallCores(availableCores);

        int curBigCore = 0;

        for(int c : smallCores){
            migration m;

            m.fromCore = c;
            m.toCore = bigCores.at(curBigCore);
            m.swap = false;
            migrations.push_back(m);
            curBigCore++ %= bigCores.size();
        }

        for(int c : bigCores){
            fCores.at(c) = maxFrequencyBig;
            std::cout << "[Scheduler][FunkyPolicy]: Core " << c << " set to " << maxFrequencyBig << " MHz" << std::endl;
        }

        estimationState = EstimationState::BIGGEST_AT_MAX;
    } else if(estimationState == EstimationState::BIGGEST_AT_MAX){
        std::vector<int> bigCores = bigCores(availableCores);
        std::vector<int> smallCores = smallCores(availableCores);

        for(int c : bigCores){
            if(PerformanceCounters.getTemperatureOfCore(c) > criticalTemperature){
                std::cout << "[Scheduler][FunkyPolicy]: Big core " << c << " is too hot" << std::endl;

                tMig = time - tMigStart;

                // TODO: DVFS estimation
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

    if(bigSmallMigrationState == BigSmallMigrationState::START){
        std::vector<int> bigCores = bigCores(availableCores);
        std::vector<int> smallCores = smallCores(availableCores);

        int curSmallCore = 0;

        for(int c : bigCores){
            migration m;

            m.fromCore = c;
            m.toCore = smallCores.at(curSmallCore);
            m.swap = false;
            migrations.push_back(m);
            curSmallCore++ %= smallCores.size();

            fCores.at(c) = minFrequencyBig;
            std::cout << "[Scheduler][FunkyPolicy]: Core " << c << " set to " << minFrequencyBig << " MHz" << std::endl;
        }

        bigSmallMigrationState = BigSmallMigrationState::SMALLEST_AT_MAX;
    } else if(bigSmallMigrationState == BigSmallMigrationState::SMALLEST_AT_MAX){
        std::vector<int> bigCores = bigCores(availableCores);
        std::vector<int> smallCores = smallCores(availableCores);

        for(int c : bigCores){
            if(PerformanceCounters.getTemperatureOfCore(c) > criticalTemperature){
                goto bigSmallMigrationFinal;
            }
        }

        std::cout << "[Scheduler][FunkyPolicy]: All big cores are cold" << std::endl;

        bigSmallMigrationState = BigSmallMigrationState::BIGGEST_ARE_COLD;
    } else if(bigSmallMigrationState == BigSmallMigrationState::BIGGEST_ARE_COLD){
        std::vector<int> bigCores = bigCores(availableCores);
        std::vector<int> smallCores = smallCores(availableCores);

        int curBigCore = 0;

        for(int c : smallCores){
            migration m;

            m.fromCore = c;
            m.toCore = bigCores.at(curBigCore);
            m.swap = false;
            migrations.push_back(m);
            curBigCore++ %= bigCores.size();
        }

        for(int c : bigCores){
            fCores.at(c) = maxFrequencyBig;
            std::cout << "[Scheduler][FunkyPolicy]: Core " << c << " set to " << maxFrequencyBig << " MHz" << std::endl;
        }

        bigSmallMigrationState = BigSmallMigrationState::DONE;
        state = State::INIT;
    }

    bigSmallMigrationFinal:
    return migrations;
}