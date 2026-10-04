#include "SyncTaskPolicy.h"

#include <algorithm>
#include <limits>

namespace {
QDateTime addDuration(QDateTime value, std::chrono::milliseconds duration) {
    return value.addMSecs(duration.count());
}

std::chrono::milliseconds retryDelay(const SyncTaskState &state, const SyncSchedulePolicy &policy) {
    const int exponent = std::clamp(state.consecutiveFailures - 1, 0, 30);
    const auto initial = policy.initialRetryDelay.count();
    const auto multiplier = qint64{1} << exponent;
    const auto bounded = initial > std::numeric_limits<qint64>::max() / multiplier
        ? std::numeric_limits<qint64>::max() : initial * multiplier;
    return std::min(std::chrono::milliseconds(bounded), policy.maximumRetryDelay);
}
}

SyncScheduleDecision SyncTaskPolicy::Evaluate(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                                              QDateTime now, std::optional<QDateTime> retryAfter) {
    SyncScheduleDecision decision;
    decision.cacheValidity = state.cacheValidity;
    if (retryAfter.has_value() && retryAfter.value() > now) {
        decision.nextRunAt = retryAfter.value();
        return decision;
    }

    const auto lastAttempted = state.lastAttemptedAt.value_or(now);
    if (state.lastErrorCategory != AniListSyncErrorCategory::None) {
        if (IsRetryable(state.lastErrorCategory)) {
            decision.nextRunAt = addDuration(lastAttempted, retryDelay(state, policy));
        } else {
            decision.nextRunAt = addDuration(lastAttempted, policy.cooldown);
        }
        decision.shouldRunNow = decision.nextRunAt <= now;
        return decision;
    }

    if (state.cacheValidity == CacheValidity::Fresh && state.lastSucceededAt.has_value()) {
        decision.nextRunAt = addDuration(state.lastSucceededAt.value(), policy.normalInterval);
        decision.shouldRunNow = decision.nextRunAt <= now;
        return decision;
    }

    decision.shouldRunNow = true;
    decision.nextRunAt = now;
    return decision;
}
