#ifndef HAIKENANIME_ICOVERDOWNLOADER_H
#define HAIKENANIME_ICOVERDOWNLOADER_H
#include "CoverDownloadTypes.h"
#include <functional>
class ICoverDownloader {
public:
    using Completion = std::function<void(CoverDownloadResult)>;
    virtual ~ICoverDownloader() = default;
    virtual quint64 Start(const CoverRequest &request, Completion completion) = 0;
    virtual void Cancel(quint64 requestId) = 0;
};
#endif
