#ifndef HAIKENANIME_IANILISTDATASOURCE_H
#define HAIKENANIME_IANILISTDATASOURCE_H

#include <QJsonObject>
#include <QString>

#include "../media/MediaPage.h"
#include "../media/MediaSyncFilter.h"

struct AniListDataSourceRequest final {
    MediaSyncFilter filter;
    QJsonObject variables;
    QString queryIdentity;
    bool refresh = false;

    [[nodiscard]] static AniListDataSourceRequest ForPartition(const SyncPartition partition) {
        AniListDataSourceRequest request;
        request.filter.partition = partition;
        request.filter.type = QStringLiteral("ANIME");
        request.queryIdentity = partition == SyncPartition::UserList
            ? QStringLiteral("user-list:v1") : QStringLiteral("catalog:v1");
        request.variables.insert(QStringLiteral("type"), request.filter.type);
        request.variables.insert(QStringLiteral("status"), QJsonValue(QJsonValue::Null));
        request.variables.insert(QStringLiteral("list"), QJsonValue(QJsonValue::Null));
        request.setPage(request.filter.startingPage);

        switch (partition) {
        case SyncPartition::ActiveCatalog:
            request.filter.status = QStringLiteral("RELEASING");
            break;
        case SyncPartition::InactiveCatalog:
            request.filter.status = QStringLiteral("NOT_YET_RELEASED");
            break;
        case SyncPartition::CompletedCatalog:
            request.filter.status = QStringLiteral("FINISHED");
            break;
        default:
            break;
        }
        if (!request.filter.status.isEmpty()) {
            request.variables.insert(QStringLiteral("status"), request.filter.status);
        }
        return request;
    }

    void setPage(const int page) {
        filter.startingPage = page;
        variables.insert(QStringLiteral("page"), page);
        variables.insert(QStringLiteral("perPage"), filter.perPage);
    }
};

struct AniListDataSourceResult final {
    MediaPage page;
    SyncPartition completedPartition = SyncPartition::UserList;
    bool isCompleteAuthoritativeSnapshot = false;
};

/** Reads one AniList partition and reports whether its result is a complete snapshot. */
class IAniListDataSource {
public:
    virtual ~IAniListDataSource() = default;

    [[nodiscard]] virtual bool fetchPage(const AniListDataSourceRequest &request,
                                         AniListDataSourceResult &result, QString &error) = 0;
};

#endif // HAIKENANIME_IANILISTDATASOURCE_H
