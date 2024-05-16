
/**
 * This header implements a policy that maps new applications to the coldest
     core
 * and migrates threads from hot cores to the coldest cores.
 */
#ifndef __FUNCKYPOLICY_H
#define __FUNCKYPOLICY_H
#include <vector>
#include "mappingpolicy.h"
#include "migrationpolicy.h"
#include "performance_counters.h"
#include "dvfsAsymmetric.h"
#include "powermodel.h"

class FunkyPolicy : public MappingPolicy, public MigrationPolicy, public DVFSPolicy {
public:
    FunkyPolicy(
            const PerformanceCounters *performanceCounters,
            int coreColumns,
            int coreRows,
            float criticalTemperature,
            int minFrequencyBig,
            int maxFrequencyBig,
            int minFrequencySmall,
            int maxFrequencySmall);

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
    enum class State {
        BIG_BIG_MIGRATION,
        BIG_SMALL_MIGRATION,
        ESTIMATION,
        INIT,
        DVFS
    };

    enum class BigSmallMigrationState {
        START,
        SMALLEST_AT_MAX,
        DONE
    };

    enum class EstimationState {
        START,
        SMALLEST_AT_MAX,
        BIGGEST_AT_MAX,
        DVFS,
        DVFS_BIGGEST_AT_MAX,
        DONE
    };

    State state;
    BigSmallMigrationState bigSmallMigrationState;
    EstimationState estimationState;

    const PerformanceCounters *performanceCounters;

    unsigned int coreColumns;
    unsigned int coreRows;
    float criticalTemperature;

    int minFrequencyBig, maxFrequencyBig, minFrequencySmall, maxFrequencySmall;

    SubsecondTime tMigStart, tMig, tHighestSmall, tHighestBigStart, tHighestBig, tDVFSStart, tDVFSCycleStart, tLastEstimation;
    double fHighestBig, fHighestSmall, CPIHighestBig, CPIHighestSmall;
    double nMig, nDVFS;
    int measurements;

    static std::vector<int> fCores;
    std::vector<SubsecondTime> tBigFreqs;
    std::vector<double> CPIBig;
    std::vector<int> fBig;

    bool isBig(const uint core);
    std::vector<int> bigCores();
    std::vector<int> smallCores();

    std::vector<migration> estimation(SubsecondTime time, const std::vector<bool> &availableCores);
    std::vector<migration> bigSmallMigration(const std::vector<bool> &availableCores);
    migration bigBigMigration(const std::vector<bool> &availableCores);

    void logTemperatures(const std::vector<bool> &availableCores);

};
#endif