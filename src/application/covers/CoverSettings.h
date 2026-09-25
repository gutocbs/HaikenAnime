#ifndef HAIKENANIME_COVERSETTINGS_H
#define HAIKENANIME_COVERSETTINGS_H

#include "CoverQuality.h"

struct CoverSettings {
    CoverQuality quality = CoverQuality::Medium;
    int maxConcurrentDownloads = 3;
    int timeoutMs = 30000;
    int maxRetries = 2;
    int retryDelayMs = 1000;
    int maxResponseBytes = 5 * 1024 * 1024;
    int minDimension = 32;
    int maxDimension = 4096;
    int failureCooldownMs = 5 * 60 * 1000;
};

#endif // HAIKENANIME_COVERSETTINGS_H
