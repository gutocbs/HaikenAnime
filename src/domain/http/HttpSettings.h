#ifndef HAIKENANIME_HTTPSETTINGS_H
#define HAIKENANIME_HTTPSETTINGS_H

/** Runtime settings used to manage HTTP requests. */
struct HttpSettings {
    int timeoutMs = 30000;
    int maxRetries = 2;
    int retryDelayMs = 1000;
};

#endif // HAIKENANIME_HTTPSETTINGS_H
