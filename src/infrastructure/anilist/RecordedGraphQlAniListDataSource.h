#ifndef HAIKENANIME_RECORDEDGRAPHQLANILISTDATASOURCE_H
#define HAIKENANIME_RECORDEDGRAPHQLANILISTDATASOURCE_H

#include "../../application/media/IMediaDataSource.h"

class RecordedGraphQlAniListDataSource final : public IMediaDataSource {
public:
    explicit RecordedGraphQlAniListDataSource(QString filePath);
    bool fetchPage(const MediaSyncFilter &filter, MediaPage &result, QString &error) override;

private:
    QString filePath_;
};

#endif
