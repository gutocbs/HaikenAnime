#ifndef HAIKENANIME_PERSONALLISTCHANGESERVICE_H
#define HAIKENANIME_PERSONALLISTCHANGESERVICE_H

#include "IPersonalListChangeWriter.h"

/** Converts a local personal-list edit into durable local data and AniList outbox changes. */
class PersonalListChangeService final {
public:
    explicit PersonalListChangeService(IPersonalListChangeWriter &writer);

    [[nodiscard]] bool save(const Media &previous, const Media &updated, QString &error) const;

private:
    IPersonalListChangeWriter &writer_;
};

#endif // HAIKENANIME_PERSONALLISTCHANGESERVICE_H
