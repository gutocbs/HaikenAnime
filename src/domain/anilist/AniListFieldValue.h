#ifndef HAIKENANIME_ANILISTFIELDVALUE_H
#define HAIKENANIME_ANILISTFIELDVALUE_H

#include <QString>

#include <variant>

/** Represents a value supported by the field-level AniList merge rules. */
using AniListFieldValue = std::variant<QString, int>;

#endif // HAIKENANIME_ANILISTFIELDVALUE_H
