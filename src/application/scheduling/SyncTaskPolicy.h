#ifndef HAIKENANIME_SYNCTASKPOLICY_H
#define HAIKENANIME_SYNCTASKPOLICY_H

#include "SyncTaskTypes.h"

class SyncTaskPolicy final {
public:
    [[nodiscard]] static SyncScheduleDecision Evaluate(const SyncTaskState &state,
                                                        const SyncSchedulePolicy &policy,
                                                        QDateTime now,
                                                        std::optional<QDateTime> retryAfter);
};

#endif // HAIKENANIME_SYNCTASKPOLICY_H
