#include "AniListUpdateClient.h"

#include "AniListGraphQlClient.h"
#include "GraphQlQueryStore.h"

#include "AniListGraphQlResponse.h"

#include <QJsonObject>

#include <variant>

AniListUpdateClient::AniListUpdateClient(AniListGraphQlClient &graphQlClient,
                                         GraphQlQueryStore &updateQuery,
                                         GraphQlQueryStore &entryIdQuery,
                                         GraphQlQueryStore &deleteMutation)
    : graphQlClient_(graphQlClient), updateQuery_(updateQuery),
      entryIdQuery_(entryIdQuery), deleteMutation_(deleteMutation) {
}

namespace {
/** AniList answers a missing list entry with GraphQL error "Not Found." (HTTP 404). */
bool IsNotFound(const AniListGraphQlResponse &response) {
    if (!response.hasErrors()) return false;
    for (const auto &graphQlError : response.errors) {
        if (!graphQlError.message.contains(QStringLiteral("not found"), Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}
}

bool AniListUpdateClient::Send(GraphQlQueryStore &queryStore, const QJsonObject &variables,
                               AniListGraphQlResponse &response, QString &error) const {
    QString query;
    if (!queryStore.load(query, error)) {
        return false;
    }
    return graphQlClient_.execute(query, variables, response, error);
}

bool AniListUpdateClient::deleteMediaListEntry(const int mediaId, const qint64 userId,
                                               QString &error) {
    error.clear();
    if (mediaId <= 0 || userId <= 0) {
        error = QStringLiteral("A list deletion requires a valid media id and AniList account id.");
        return false;
    }

    AniListGraphQlResponse entryResponse;
    const bool queried = Send(entryIdQuery_,
        QJsonObject{{QStringLiteral("userId"), static_cast<qint64>(userId)},
                    {QStringLiteral("mediaId"), mediaId}},
        entryResponse, error);
    if (IsNotFound(entryResponse)) {
        error.clear();
        return true;
    }
    if (!queried) return false;
    if (entryResponse.hasErrors()) {
        error = entryResponse.errors.first().message;
        return false;
    }
    const auto entry = entryResponse.data.value(QStringLiteral("MediaList"));
    if (entry.isNull() || entry.isUndefined()) {
        return true;
    }
    const auto entryId = entry.toObject().value(QStringLiteral("id")).toInt();
    if (entryId <= 0) {
        error = QStringLiteral("AniList returned an invalid list entry id.");
        return false;
    }

    AniListGraphQlResponse deleteResponse;
    const bool deleteSent = Send(deleteMutation_, QJsonObject{{QStringLiteral("id"), entryId}},
                                 deleteResponse, error);
    if (IsNotFound(deleteResponse)) {
        error.clear();
        return true;
    }
    if (!deleteSent) return false;
    if (deleteResponse.hasErrors()) {
        error = deleteResponse.errors.first().message;
        return false;
    }
    const auto deleted = deleteResponse.data.value(QStringLiteral("DeleteMediaListEntry"))
                             .toObject().value(QStringLiteral("deleted"));
    if (!deleted.toBool(false)) {
        error = QStringLiteral("AniList did not confirm the list entry deletion.");
        return false;
    }
    return true;
}

bool AniListUpdateClient::Execute(GraphQlQueryStore &queryStore, const QJsonObject &variables,
                                  QString &error) const {
    QString query;
    if (!queryStore.load(query, error)) {
        return false;
    }

    AniListGraphQlResponse response;
    if (!graphQlClient_.execute(query, variables, response, error)) {
        return false;
    }
    if (response.hasErrors()) {
        error = response.errors.first().message;
        return false;
    }
    return response.hasData();
}

bool AniListUpdateClient::updateMedia(const AniListMediaPendingChanges &changes, QString &error) {
    error.clear();
    if (changes.mediaId <= 0 || changes.changes.isEmpty()) {
        error = QStringLiteral("A media update must contain a valid media id and at least one change.");
        return false;
    }

    QJsonObject variables{{QStringLiteral("mediaId"), changes.mediaId}};
    for (const auto &change : changes.changes) {
        switch (change.field) {
        case AniListField::Progress:
            if (!std::holds_alternative<int>(change.newValue)) {
                error = QStringLiteral("AniList progress must be an integer.");
                return false;
            }
            variables.insert(QStringLiteral("progress"), std::get<int>(change.newValue));
            break;
        case AniListField::PersonalScore:
            if (!std::holds_alternative<int>(change.newValue)) {
                error = QStringLiteral("AniList score must be numeric.");
                return false;
            }
            variables.insert(QStringLiteral("score"), std::get<int>(change.newValue));
            break;
        case AniListField::ListStatus:
            if (!std::holds_alternative<QString>(change.newValue)) {
                error = QStringLiteral("AniList list status must be text.");
                return false;
            }
            variables.insert(QStringLiteral("status"), std::get<QString>(change.newValue));
            break;
        case AniListField::Deletion:
            error = QStringLiteral("Deletion requires a separate confirmed operation.");
            return false;
        default:
            error = QStringLiteral("The field does not have an update mutation.");
            return false;
        }
    }
    return Execute(updateQuery_, variables, error);
}
