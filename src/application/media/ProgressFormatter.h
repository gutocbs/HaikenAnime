#ifndef HAIKENANIME_PROGRESSFORMATTER_H
#define HAIKENANIME_PROGRESSFORMATTER_H

#include <QString>

// Formats "consumed/total"; an unknown total (<= 0) is shown as "?".
[[nodiscard]] QString FormatProgress(int consumed, int total);

#endif
