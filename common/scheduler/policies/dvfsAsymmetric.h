/**
 * This header implements the fixed power DVFS policy
 */

#ifndef __DVFS_ASYMMETRIC_H
#define __DVFS_ASYMMETRIC_H

#include <vector>
#include "dvfspolicy.h"

#include "scheduler_open.h"
#include "config.hpp"
#include "core_manager.h"
#include "magic_server.h"
#include "performance_model.h"
#include "thread.h"
#include "thread_manager.h"

class DVFSAsymmetric : public DVFSPolicy {
public:
    DVFSAsymmetric(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize, String asymmetry);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);

private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int minFrequency;
    int maxFrequency;
    int frequencyStepSize;

    String asymmetry;
};

#endif
