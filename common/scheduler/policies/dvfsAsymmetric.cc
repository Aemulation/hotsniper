#include "dvfsAsymmetric.h"
#include "powermodel.h"
#include <iomanip>
#include <iostream>

DVFSAsymmetric::DVFSAsymmetric(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize, String asymmetry)
    : performanceCounters(performanceCounters), coreRows(coreRows), coreColumns(coreColumns), minFrequency(minFrequency), maxFrequency(maxFrequency), frequencyStepSize(frequencyStepSize), asymmetry(asymmetry) {
    
}

std::vector<int> DVFSAsymmetric::getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) {
    std::vector<int> frequencies(coreRows * coreColumns);

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
        // if (activeCores.at(coreCounter)) {
        //     float power = performanceCounters->getPowerOfCore(coreCounter);
        //     float temperature = performanceCounters->getTemperatureOfCore(coreCounter);
        //     int frequency = oldFrequencies.at(coreCounter);
        //     float utilization = performanceCounters->getUtilizationOfCore(coreCounter);

        //     cout << "[Scheduler][DVFSAsymmetric]: Core " << setw(2) << coreCounter << ":";
        //     cout << " P=" << fixed << setprecision(3) << power << " W";
        //     cout << " (budget: " << fixed << setprecision(3) << perCorePowerBudget << " W)";
        //     cout << " f=" << frequency << " MHz";
        //     cout << " T=" << fixed << setprecision(1) << temperature << " °C";
        //     cout << " utilization=" << fixed << setprecision(3) << utilization << endl;

        //     int expectedGoodFrequency = PowerModel::getExpectedGoodFrequency(frequency, power, perCorePowerBudget, minFrequency, maxFrequency, frequencyStepSize);
        //     frequencies.at(coreCounter) = expectedGoodFrequency;
        // } else {
        //     frequencies.at(coreCounter) = minFrequency;
        // }

        if(asymmetry == "master"){
            frequencies.at(0) = minFrequency;

            for(int i = 1; i < coreRows * coreColumns; i++) {
                frequencies.at(i) = maxFrequency;
            }
        } else if(asymmetry == "slave"){
            frequencies.at(0) = maxFrequency;

            for(int i = 1; i < coreRows * coreColumns; i++) {
                frequencies.at(i) = minFrequency;
            }
        }
    }

    return frequencies;
}