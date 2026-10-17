#ifndef HAIKENANIME_PERSONALLISTCHANGESERVICE_H
#define HAIKENANIME_PERSONALLISTCHANGESERVICE_H

#include <functional>

#include "IPersonalListChangeWriter.h"

/** Converts a local personal-list edit into durable local data and AniList outbox changes. */
class PersonalListChangeService final {
public:
    explicit PersonalListChangeService(IPersonalListChangeWriter &writer);

    void setPendingChangesNotifier(std::function<void()> notifier);

    /** Supplies the AniList account that owns edits made now (connected account, else last one). */
    void setOwnerProvider(std::function<qint64()> provider);

    [[nodiscard]] bool save(const Media &previous, const Media &updated, QString &error) const;

    /** Removes the media from the personal list locally and queues its confirmed AniList deletion. */
    [[nodiscard]] bool remove(const Media &media, QString &error) const;

private:
    IPersonalListChangeWriter &writer_;
    std::function<void()> pendingChangesNotifier_;
    std::function<qint64()> ownerProvider_;
};

#endif // HAIKENANIME_PERSONALLISTCHANGESERVICE_H
