#include "PersonalListProgressWriter.h"

#include <utility>

PersonalListProgressWriter::PersonalListProgressWriter(const PersonalListChangeService &changes,
                                                       std::function<void(const Media &)> applied)
    : changes_(changes), applied_(std::move(applied)) {}

bool PersonalListProgressWriter::apply(const Media &previous, const Media &updated, QString &error) {
    if (!changes_.save(previous, updated, error)) return false;
    if (applied_) applied_(updated);
    return true;
}
