
/**
 * This header implements a policy that maps new applications to the coldest
     core
 * and migrates threads from hot cores to the coldest cores.
 */
#ifndef __COLDESTCORE_H
#define __COLDESTCORE_H
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
            int coreColumns, // flipped size and height
            int coreRows, // flipped size and height
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
        INIT
    };

    enum class BigSmallMigrationState {
        START,
        SMALLEST_AT_MAX,
        BIGGEST_ARE_COLD,
        DONE
    };

    enum class EstimationState {
        START,
        SMALLEST_AT_MAX,
        BIGGEST_ARE_COLD,
        BIGGEST_AT_MAX,
        DONE
    };

    State state;
    BigSmallMigrationState bigSmallMigrationState;
    EstimationState estimationState;

    const PerformanceCounters *performanceCounters;

    unsigned int coreColumns; // flipped size and height
    unsigned int coreRows; // flipped size and height
    float criticalTemperature;

    int minFrequencyBig, maxFrequencyBig, minFrequencySmall, maxFrequencySmall;

    SubsecondTime tMigStart;
    float fHighestBig, fHighestSmall, tHighestBig, tHighestSmall, CPIHighestBig, CPIHighestSmall;
    float tMig, nMig;

    std::vector<int> fCores;
    std::vector<int> tBigFreqs;
    std::vector<int> CPIBig;

    void estimation(SubsecondTime time, const std::vector<bool> &availableCores);
    void bigSmallMigration(const std::vector<bool> &availableCores);

    int getColdestCore(const std::vector<bool> &availableCores);
    void logTemperatures(const std::vector<bool> &availableCores);

    void bigBigMigration(const std::vector<bool> &availableCores)
};
#endif