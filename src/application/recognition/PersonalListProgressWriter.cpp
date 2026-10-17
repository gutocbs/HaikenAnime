#include "PersonalListProgressWriter.h"

PersonalListProgressWriter::PersonalListProgressWriter(const PersonalListChangeService &changes)
    : changes_(changes) {}

bool PersonalListProgressWriter::apply(const Media &previous, const Media &updated, QString &error) {
    return changes_.save(previous, updated, error);
}
