#ifndef HAIKENANIME_PERSONALLISTPROGRESSWRITER_H
#define HAIKENANIME_PERSONALLISTPROGRESSWRITER_H

#include <functional>

#include "../media/PersonalListChangeService.h"
#include "IRecognitionProgressWriter.h"

/// Routes automatic progress through the same local-save + AniList outbox path as manual edits.
class PersonalListProgressWriter final : public IRecognitionProgressWriter {
public:
    explicit PersonalListProgressWriter(const PersonalListChangeService &changes,
                                        std::function<void(const Media &)> applied = {});

    bool apply(const Media &previous, const Media &updated, QString &error) override;

private:
    const PersonalListChangeService &changes_;
    std::function<void(const Media &)> applied_;
};

#endif // HAIKENANIME_PERSONALLISTPROGRESSWRITER_H
