#include "crankyPolicy.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

#define CRANKY_PRINT std::cout << "[Scheduler][CrankyPolicy]: "

CrankyPolicy::CrankyPolicy(
        const PerformanceCounters *performanceCounters,
        int coreColumns,
        int coreRows,
        int minFrequency,
        int maxFrequency,
        int maxTemperature)
    : performanceCounters(performanceCounters), coreColumns(coreColumns), coreRows(coreRows),
      minFrequency(minFrequency), maxFrequency(maxFrequency), maxTemperature(maxTemperature)
{
    std::cout << "CrankyPolicy: Initializing neural networks" << std::endl;
    for(int c = 0; c < coreColumns * coreRows; c++) {
        neuralNets.push_back(new Net({6, 10, 2}));
    }
}

std::vector<int> CrankyPolicy::map(
        String taskName,
        int taskCoreRequirement,
        const std::vector<bool> &availableCores,
        const std::vector<bool> &activeCores)
{
    std::vector<int> taskCoreMapping(taskCoreRequirement, -1);

    // Find the first available core
    for (int i = 0; i < taskCoreRequirement; i++) {
        taskCoreMapping.push_back(i);
    }

    return taskCoreMapping;
}

std::vector<migration> CrankyPolicy::migrate(
        SubsecondTime time,
        const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores)
{
    std::vector<migration> migrations;

    // No migrations
    return migrations;
}

std::vector<int> CrankyPolicy::getFrequencies(
    const std::vector<int> &oldFrequencies,
    const std::vector<bool> &activeCores)
{
    static bool first = true;
    static std::vector<double> oldTemps(coreColumns * coreRows, 0);
    static std::vector<double> oldTemps1(coreColumns * coreRows, 0);
    static std::vector<double> oldTemps2(coreColumns * coreRows, 0);
    static std::vector<bool> coolingDown(coreColumns * coreRows, 0);

    std::vector<int> newFrequencies(oldFrequencies.size(), -1);

    logTemperatures();

    for(int c = 0; c < coreColumns * coreRows; c++) {
        if(performanceCounters->getTemperatureOfCore(c) > maxTemperature + 20){
            coolingDown.at(c) = true;
            newFrequencies.at(c) = minFrequency;
        } else if(performanceCounters->getTemperatureOfCore(c) < maxTemperature - 10){
            coolingDown.at(c) = false;
        }
    }

    if(first) {
        first = false;
    } else {
        CRANKY_PRINT << "Doing backprop" << std::endl;

        for(int c = 0; c < coreColumns * coreRows; c++) {
            if(coolingDown.at(c)){
                continue;
            }

            double targetFrequency = ((double)oldFrequencies[c] - minFrequency) / (maxFrequency - minFrequency);

            // if(performanceCounters->getTemperatureOfCore(c) > maxTemperature){
                targetFrequency = maxTemperature / performanceCounters->getTemperatureOfCore(c) * targetFrequency;
            // } else {
            //     targetFrequency += (1.0 - targetFrequency) / 2;
            // }

            targetFrequency = std::min(std::max(targetFrequency, 0.1), 1.0);

            CRANKY_PRINT << "Core " << c << " Target frequency: " << targetFrequency << std::endl;
            
            std::vector<double> backPropVals = {targetFrequency, 0};

            neuralNets[c]->backProp(backPropVals);

            CRANKY_PRINT << "Error: " << neuralNets[c]->getRecentAverageError() << std::endl;
        }
    }


    for(int c = 0; c < coreColumns * coreRows; c++) {
        if(coolingDown.at(c)){
            continue;
        }

        std::vector<double> inputVals = {
            performanceCounters->getUtilizationOfCore(c),
            ((double)performanceCounters->getFreqOfCore(c) - minFrequency) / (maxFrequency - minFrequency),
            performanceCounters->getTemperatureOfCore(c) / maxTemperature,
            (performanceCounters->getTemperatureOfCore(c) - oldTemps.at(c)) / maxTemperature,
            (performanceCounters->getTemperatureOfCore(c) - oldTemps1.at(c)) / maxTemperature,
            (performanceCounters->getTemperatureOfCore(c) - oldTemps2.at(c)) / maxTemperature
        };

        CRANKY_PRINT << "Inputs: " << inputVals.at(0) << " " << inputVals.at(1) << " " << inputVals.at(2) << std::endl;

        neuralNets[c]->feedForward(inputVals);

        std::vector<double> resultVals;
        neuralNets[c]->getResults(resultVals);

        std::cout << "Core " << c << " result: " << resultVals.at(0) << std::endl;

        double newFreq = std::min(std::max(resultVals.at(0), 0.0), 1.0) * (maxFrequency - minFrequency) + minFrequency;

        if(std::abs(oldFrequencies.at(c) - newFreq) > 20){
            newFrequencies.at(c) = newFreq;
        } else {
            newFrequencies.at(c) = oldFrequencies.at(c);
        }
    }

    oldTemps2 = oldTemps1;
    oldTemps1 = oldTemps;

    for(int c = 0; c < coreColumns * coreRows; c++) {
        oldTemps.at(c) = performanceCounters->getTemperatureOfCore(c);
    }

    return newFrequencies;
}

void CrankyPolicy::logTemperatures() {
    CRANKY_PRINT << "Temperatures of available cores:" << endl;
    for (uint y = 0; y < coreRows; y++) {
        for (uint x = 0; x < coreColumns; x++) {
            int coreId = y * coreColumns + x;

            if (x > 0) {
                cout << " ";
            }

            float temperature = performanceCounters->getTemperatureOfCore(coreId);
            cout << " " << fixed << setprecision(1) << temperature << " " << setprecision(4);
        }
        cout << endl;
    }
}