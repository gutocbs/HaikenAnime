#include "SyncTaskPolicy.h"

#include <algorithm>
#include <cmath>
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

CacheValidity cacheValidity(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                            const QDateTime now) {
    if (!state.lastSucceededAt.has_value()) return state.cacheValidity;
    if (state.lastSucceededAt.value().addMSecs(policy.normalInterval.count()) >= now) {
        return CacheValidity::Fresh;
    }
    if (state.lastSucceededAt.value().addMSecs(policy.staleProtectionTtl.count()) >= now) {
        return CacheValidity::Stale;
    }
    return CacheValidity::Expired;
}

std::chrono::milliseconds withDeterministicJitter(const std::chrono::milliseconds duration,
                                                  const SyncTaskKind kind,
                                                  const double jitterRatio) {
    if (duration.count() <= 0 || jitterRatio <= 0.0) return duration;
    constexpr qint64 taskKindCount = 8;
    const auto taskWeight = static_cast<qint64>(kind) + 1;
    const auto jitter = static_cast<qint64>(std::llround(
        static_cast<double>(duration.count()) * jitterRatio * taskWeight / taskKindCount));
    if (jitter <= 0 || duration.count() > std::numeric_limits<qint64>::max() - jitter) return duration;
    return duration + std::chrono::milliseconds(jitter);
}
}

SyncScheduleDecision SyncTaskPolicy::Evaluate(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                                              QDateTime now, std::optional<QDateTime> retryAfter) {
    SyncScheduleDecision decision;
    decision.cacheValidity = cacheValidity(state, policy, now);
    if (retryAfter.has_value() && retryAfter.value() > now) {
        decision.nextRunAt = retryAfter.value();
        return decision;
    }

    const auto lastAttempted = state.lastAttemptedAt.value_or(now);
    if (state.lastErrorCategory != AniListSyncErrorCategory::None) {
        if (IsRetryable(state.lastErrorCategory)) {
            if (state.consecutiveImmediateRetries < policy.maximumConsecutiveImmediateRetries) {
                decision.shouldRunNow = true;
                decision.nextRunAt = now;
                return decision;
            }
            const auto delay = std::min(withDeterministicJitter(retryDelay(state, policy), state.kind,
                                                                 policy.jitterRatio),
                                        policy.maximumRetryDelay);
            decision.nextRunAt = addDuration(lastAttempted, delay);
        } else {
            decision.nextRunAt = addDuration(lastAttempted,
                                             withDeterministicJitter(policy.cooldown, state.kind,
                                                                     policy.jitterRatio));
        }
        decision.shouldRunNow = decision.nextRunAt <= now;
        return decision;
    }

    if (decision.cacheValidity == CacheValidity::Fresh && state.lastSucceededAt.has_value()) {
        decision.nextRunAt = addDuration(state.lastSucceededAt.value(),
                                         withDeterministicJitter(policy.normalInterval, state.kind,
                                                                 policy.jitterRatio));
        decision.shouldRunNow = decision.nextRunAt <= now;
        return decision;
    }

    decision.shouldRunNow = true;
    decision.nextRunAt = now;
    return decision;
}
