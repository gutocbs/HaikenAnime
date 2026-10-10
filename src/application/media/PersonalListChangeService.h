#ifndef HAIKENANIME_PERSONALLISTCHANGESERVICE_H
#define HAIKENANIME_PERSONALLISTCHANGESERVICE_H

#include <functional>

#include "IPersonalListChangeWriter.h"

/** Converts a local personal-list edit into durable local data and AniList outbox changes. */
class PersonalListChangeService final {
public:
    explicit PersonalListChangeService(IPersonalListChangeWriter &writer);

    void setPendingChangesNotifier(std::function<void()> notifier);

    [[nodiscard]] bool save(const Media &previous, const Media &updated, QString &error) const;

private:
    IPersonalListChangeWriter &writer_;
    std::function<void()> pendingChangesNotifier_;
};

#endif // HAIKENANIME_PERSONALLISTCHANGESERVICE_H
