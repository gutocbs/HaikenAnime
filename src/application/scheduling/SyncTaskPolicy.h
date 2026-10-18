#ifndef HAIKENANIME_SYNCTASKPOLICY_H
#define HAIKENANIME_SYNCTASKPOLICY_H

#include "SyncTaskTypes.h"

/** Why a task is evaluated: its own schedule, a user request, or a rerun requested while it was running. */
enum class SyncTaskTrigger { Scheduled, ManualRun, LocalChange };

struct SyncTaskPolicyRequest final {
    SyncTaskTrigger trigger = SyncTaskTrigger::Scheduled;
    std::chrono::milliseconds jitter = std::chrono::milliseconds::zero();
};

struct SyncTaskPolicyDecision final {
    SyncScheduleDecision schedule;
    SyncTaskState proposedState;
    bool resetFailureState = false;
};

class SyncTaskPolicy final {
public:
    [[nodiscard]] static int PriorityFor(SyncPartition partition);
    [[nodiscard]] static SyncTaskPolicyDecision Decide(const SyncTaskState &state,
                                                        const SyncSchedulePolicy &policy,
                                                        QDateTime now,
                                                        const SyncTaskPolicyRequest &request);
};

#endif // HAIKENANIME_SYNCTASKPOLICY_H
