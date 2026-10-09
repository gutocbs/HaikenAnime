#include "AniListViewerClient.h"
#include "AniListGraphQlClient.h"
#include "GraphQlQueryStore.h"

AniListViewerClient::AniListViewerClient(AniListGraphQlClient &client, GraphQlQueryStore &queryStore)
    : client_(client), queryStore_(queryStore) {}

bool AniListViewerClient::loadViewer(AniListViewer &viewer, QString &error) {
    viewer = {};
    QString query;
    if (!queryStore_.load(query, error)) return false;
    AniListGraphQlResponse response;
    if (!client_.execute(query, QJsonObject{}, response, error)) return false;
    if (response.hasErrors()) { error = response.errors.first().message; return false; }
    const auto object = response.data.value(QStringLiteral("Viewer")).toObject();
    if (object.isEmpty() || !object.value(QStringLiteral("id")).isDouble()
        || !object.value(QStringLiteral("name")).isString()) {
        error = QStringLiteral("AniList Viewer response is invalid."); return false;
    }
    viewer.id = object.value(QStringLiteral("id")).toInteger();
    viewer.username = object.value(QStringLiteral("name")).toString();
    if (viewer.id <= 0 || viewer.username.trimmed().isEmpty()) {
        error = QStringLiteral("AniList Viewer response is invalid."); return false;
    }
    return true;
}
