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