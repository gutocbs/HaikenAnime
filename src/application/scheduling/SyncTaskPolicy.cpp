#include "SyncTaskPolicy.h"

#include <algorithm>
#include <limits>

namespace {
constexpr auto maximumFutureDelay = std::chrono::hours(24 * 366 * 100);

std::chrono::milliseconds nonNegativeDuration(const std::chrono::milliseconds duration) {
    return std::max(duration, std::chrono::milliseconds::zero());
}

std::chrono::milliseconds boundedFutureDuration(const std::chrono::milliseconds duration) {
    return std::min(nonNegativeDuration(duration),
                    std::chrono::duration_cast<std::chrono::milliseconds>(maximumFutureDelay));
}

QDateTime clampedFutureTime(QDateTime anchor, const std::chrono::milliseconds duration) {
    return anchor.addMSecs(boundedFutureDuration(duration).count());
}

QDateTime clampedDueTime(QDateTime anchor, const std::chrono::milliseconds duration, const QDateTime now) {
    const auto delay = boundedFutureDuration(duration);
    if (!anchor.isValid() || anchor > now) anchor = now;
    const auto due = anchor.addMSecs(delay.count());
    if (!due.isValid() || due < now) return now;
    return due;
}

std::chrono::milliseconds retryDelay(const SyncTaskState &state, const SyncSchedulePolicy &policy) {
    const int exponent = std::clamp(state.consecutiveFailures - 1, 0, 30);
    const auto initial = nonNegativeDuration(policy.initialRetryDelay).count();
    const auto maximum = nonNegativeDuration(policy.maximumRetryDelay);
    const auto multiplier = qint64{1} << exponent;
    const auto bounded = initial > std::numeric_limits<qint64>::max() / multiplier
        ? std::numeric_limits<qint64>::max() : initial * multiplier;
    return std::min(std::chrono::milliseconds(bounded), maximum);
}

CacheValidity cacheValidity(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                            const QDateTime now) {
    if (!state.lastSucceededAt.has_value() || !state.lastSucceededAt->isValid()) return state.cacheValidity;
    const auto succeededAt = state.lastSucceededAt->toUTC();
    if (clampedFutureTime(succeededAt, policy.normalInterval) > now) {
        return CacheValidity::Fresh;
    }
    if (clampedFutureTime(succeededAt, policy.staleProtectionTtl) > now) {
        return CacheValidity::Stale;
    }
    return CacheValidity::Expired;
}

qint64 saturatingAdd(const qint64 value, const qint64 offset) {
    if (offset > 0 && value > std::numeric_limits<qint64>::max() - offset) {
        return std::numeric_limits<qint64>::max();
    }
    if (offset < 0 && value < std::numeric_limits<qint64>::min() - offset) {
        return std::numeric_limits<qint64>::min();
    }
    return value + offset;
}

qint64 boundedJitter(const qint64 duration, const double jitterRatio) {
    if (duration <= 0 || jitterRatio <= 0.0) return 0;
    if (jitterRatio >= 1.0) return duration;
    const auto maximum = static_cast<long double>(duration) * static_cast<long double>(jitterRatio);
    if (maximum >= static_cast<long double>(std::numeric_limits<qint64>::max())) {
        return std::numeric_limits<qint64>::max();
    }
    return static_cast<qint64>(maximum);
}

std::chrono::milliseconds withInjectedJitter(const std::chrono::milliseconds duration,
                                             const std::chrono::milliseconds requestedJitter,
                                             const double jitterRatio) {
    const auto safeDuration = nonNegativeDuration(duration);
    const auto safeRatio = std::clamp(jitterRatio, 0.0, 1.0);
    const auto maximumJitter = boundedJitter(safeDuration.count(), safeRatio);
    const auto jitter = std::clamp(requestedJitter.count(), -maximumJitter, maximumJitter);
    return std::chrono::milliseconds(std::max<qint64>(saturatingAdd(safeDuration.count(), jitter), 0));
}

SyncScheduleDecision immediateDecision(const QDateTime now, const CacheValidity cacheValidity) {
    return {.shouldRunNow = true, .nextRunAt = now, .cacheValidity = cacheValidity};
}
}

int SyncTaskPolicy::PriorityFor(const SyncPartition partition) {
    switch (partition) {
    case SyncPartition::UserList: return 700;
    case SyncPartition::PendingChanges: return 600;
    case SyncPartition::ActiveCatalog: return 500;
    case SyncPartition::InactiveCatalog: return 300;
    case SyncPartition::CompletedCatalog: return 200;
    case SyncPartition::Covers: return 100;
    case SyncPartition::DerivedMetadata: return 50;
    }
    return 0;
}

SyncTaskPolicyDecision SyncTaskPolicy::Decide(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                                              QDateTime now, const SyncTaskPolicyRequest &request) {
    now = now.toUTC();
    SyncTaskPolicyDecision decision;
    decision.proposedState = state;
    decision.schedule.cacheValidity = cacheValidity(state, policy, now);
    decision.proposedState.cacheValidity = decision.schedule.cacheValidity;
    decision.proposedState.priority = PriorityFor(decision.proposedState.partition);

    if (state.status == SyncTaskStatus::Succeeded) {
        decision.resetFailureState = state.lastErrorCategory != AniListSyncErrorCategory::None
            || !state.safeErrorDetail.isEmpty() || state.consecutiveFailures != 0
            || state.consecutiveImmediateRetries != 0;
        decision.proposedState.lastErrorCategory = AniListSyncErrorCategory::None;
        decision.proposedState.safeErrorDetail.clear();
        decision.proposedState.consecutiveFailures = 0;
        decision.proposedState.consecutiveImmediateRetries = 0;
    }

    if (request.trigger == SyncTaskTrigger::CompletedToActive) {
        decision.proposedState.kind = SyncTaskKind::ActiveCatalog;
        decision.proposedState.partition = SyncPartition::ActiveCatalog;
        decision.proposedState.priority = PriorityFor(SyncPartition::ActiveCatalog);
        decision.promotion = SyncTaskPromotion::CompletedToActive;
    }

    if (request.trigger == SyncTaskTrigger::LocalChange) {
        decision.promotion = SyncTaskPromotion::LocalChange;
    }

    if (request.retryAfter.has_value() && request.retryAfter->isValid() && *request.retryAfter > now) {
        decision.schedule.nextRunAt = request.retryAfter->toUTC();
        return decision;
    }

    if (request.trigger == SyncTaskTrigger::ManualRun || request.trigger == SyncTaskTrigger::LocalChange
        || request.trigger == SyncTaskTrigger::CompletedToActive) {
        decision.schedule = immediateDecision(now, decision.schedule.cacheValidity);
        return decision;
    }

    const auto &effectiveState = decision.proposedState;
    const auto lastAttempted = effectiveState.lastAttemptedAt.value_or(now);
    if (effectiveState.lastErrorCategory != AniListSyncErrorCategory::None) {
        if (IsRetryable(effectiveState.lastErrorCategory)) {
            if (effectiveState.consecutiveImmediateRetries < policy.maximumConsecutiveImmediateRetries) {
                decision.schedule = immediateDecision(now, decision.schedule.cacheValidity);
                return decision;
            }
            const auto delay = std::min(withInjectedJitter(retryDelay(effectiveState, policy), request.jitter,
                                                            policy.jitterRatio),
                                        nonNegativeDuration(policy.maximumRetryDelay));
            decision.schedule.nextRunAt = clampedDueTime(lastAttempted, delay, now);
        } else {
            decision.schedule.nextRunAt = clampedDueTime(lastAttempted,
                                                          withInjectedJitter(policy.cooldown, request.jitter,
                                                                             policy.jitterRatio), now);
        }
        decision.schedule.shouldRunNow = decision.schedule.nextRunAt <= now;
        return decision;
    }

    if (decision.schedule.cacheValidity == CacheValidity::Fresh && effectiveState.lastSucceededAt.has_value()) {
        decision.schedule.nextRunAt = clampedDueTime(*effectiveState.lastSucceededAt,
                                                      withInjectedJitter(policy.normalInterval, request.jitter,
                                                                         policy.jitterRatio), now);
        decision.schedule.shouldRunNow = decision.schedule.nextRunAt <= now;
        return decision;
    }

    decision.schedule = immediateDecision(now, decision.schedule.cacheValidity);
    return decision;
}

SyncScheduleDecision SyncTaskPolicy::Evaluate(const SyncTaskState &state, const SyncSchedulePolicy &policy,
                                              QDateTime now, std::optional<QDateTime> retryAfter) {
    return Decide(state, policy, now, {.retryAfter = std::move(retryAfter)}).schedule;
}
