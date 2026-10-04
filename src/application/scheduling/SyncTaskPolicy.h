#ifndef HAIKENANIME_SYNCTASKPOLICY_H
#define HAIKENANIME_SYNCTASKPOLICY_H

#include "SyncTaskTypes.h"

enum class SyncTaskTrigger { Scheduled, ManualRun, LocalChange, CompletedToActive };

struct SyncTaskPolicyRequest final {
    SyncTaskTrigger trigger = SyncTaskTrigger::Scheduled;
    std::optional<QDateTime> retryAfter;
    std::chrono::milliseconds jitter = std::chrono::milliseconds::zero();
};

struct SyncTaskPolicyDecision final {
    SyncScheduleDecision schedule;
    SyncTaskState proposedState;
    bool resetFailureState = false;
    bool promoted = false;
};

class SyncTaskPolicy final {
public:
    [[nodiscard]] static int PriorityFor(SyncPartition partition);
    [[nodiscard]] static SyncTaskPolicyDecision Decide(const SyncTaskState &state,
                                                        const SyncSchedulePolicy &policy,
                                                        QDateTime now,
                                                        const SyncTaskPolicyRequest &request);
    [[nodiscard]] static SyncScheduleDecision Evaluate(const SyncTaskState &state,
                                                        const SyncSchedulePolicy &policy,
                                                        QDateTime now,
                                                        std::optional<QDateTime> retryAfter);
};

#endif // HAIKENANIME_SYNCTASKPOLICY_H
