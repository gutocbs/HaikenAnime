#ifndef HAIKENANIME_ICACHECLEANUPPARTICIPANT_H
#define HAIKENANIME_ICACHECLEANUPPARTICIPANT_H

#include <QString>

#include <functional>

using CacheCleanupCancellationProbe = std::function<bool()>;

struct CacheCleanupParticipantResult final {
    int removedItems = 0;
    QString error;
};

class ICacheCleanupParticipant {
public:
    virtual ~ICacheCleanupParticipant() = default;
    [[nodiscard]] virtual QString Name() const = 0;
    virtual CacheCleanupParticipantResult Clear(
        const CacheCleanupCancellationProbe &isCancelled) = 0;
};

#endif // HAIKENANIME_ICACHECLEANUPPARTICIPANT_H
