#ifndef CRANKYPOLICY_H
#define CRANKYPOLICY_H

#include <vector>
#include "mappingpolicy.h"
#include "migrationpolicy.h"
#include "dvfspolicy.h"
#include "performance_counters.h"
#include "powermodel.h"
#include "neuralNetwork.h"


class CrankyPolicy : public MappingPolicy, public MigrationPolicy, public DVFSPolicy {
public:
    CrankyPolicy(
            const PerformanceCounters *performanceCounters,
            int coreColumns,
            int coreRows,
            int minFrequency,
            int maxFrequency,
            int maxTemperature);

    virtual std::vector<int> map(
            String taskName,
            int taskCoreRequirement,
            const std::vector<bool> &availableCores,
            const std::vector<bool> &activeCores);

    virtual std::vector<migration> migrate(
            SubsecondTime time,
            const std::vector<int> &taskIds,
            const std::vector<bool> &activeCores);

    virtual std::vector<int> getFrequencies(
        const std::vector<int> &oldFrequencies,
        const std::vector<bool> &activeCores);

private:
    void logTemperatures();

    const PerformanceCounters *performanceCounters;
    std::vector<Net*> neuralNets;
    int coreColumns, coreRows;
    int minFrequency, maxFrequency;
    int maxTemperature;
};

#endif