#include "ProgressFormatter.h"

#include <algorithm>

QString FormatProgress(const int consumed, const int total) {
    const auto totalLabel = total > 0 ? QString::number(total) : QStringLiteral("?");
    return QStringLiteral("%1/%2").arg(std::max(consumed, 0)).arg(totalLabel);
}
