#include "AniListUpdateClient.h"

#include "AniListGraphQlClient.h"
#include "GraphQlQueryStore.h"

#include <QJsonObject>

#include <variant>

AniListUpdateClient::AniListUpdateClient(AniListGraphQlClient &graphQlClient,
                                         GraphQlQueryStore &progressQuery,
                                         GraphQlQueryStore &scoreQuery,
                                         GraphQlQueryStore &listStatusQuery,
                                         GraphQlQueryStore &deleteQuery)
    : graphQlClient_(graphQlClient), progressQuery_(progressQuery), scoreQuery_(scoreQuery),
      listStatusQuery_(listStatusQuery), deleteQuery_(deleteQuery) {
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

bool AniListUpdateClient::UpdateMedia(const AniListMediaPendingChanges &changes, QString &error) {
    if (changes.mediaId <= 0 || changes.changes.isEmpty()) {
        error = QStringLiteral("A media update must contain a valid media id and at least one change.");
        return false;
    }

    // The grouped contract is established here. The mutation payload will be consolidated
    // into one SaveMediaListEntry query when the real AniList transport is enabled.
    for (const auto &change : changes.changes) {
        switch (change.field) {
        case AniListField::Progress:
            if (!std::holds_alternative<int>(change.newValue) ||
                !UpdateProgress(changes.mediaId, std::get<int>(change.newValue), error)) {
                return false;
            }
            break;
        case AniListField::PersonalScore:
            if (!std::holds_alternative<int>(change.newValue) ||
                !UpdateScore(changes.mediaId, std::get<int>(change.newValue), error)) {
                return false;
            }
            break;
        case AniListField::ListStatus:
            if (!std::holds_alternative<QString>(change.newValue) ||
                !UpdateListStatus(changes.mediaId, std::get<QString>(change.newValue), error)) {
                return false;
            }
            break;
        case AniListField::Deletion:
            if (change.status != AniListPendingChangeStatus::RequiresConfirmation ||
                !DeleteListEntry(changes.mediaId, error)) {
                if (error.isEmpty()) {
                    error = QStringLiteral("Deletion requires user confirmation before sending.");
                }
                return false;
            }
            break;
        default:
            error = QStringLiteral("The field does not have an update mutation.");
            return false;
        }
    }
    return true;
}

bool AniListUpdateClient::UpdateProgress(const int mediaId, const int progress, QString &error) {
    QJsonObject variables{{QStringLiteral("mediaId"), mediaId},
                          {QStringLiteral("progress"), progress}};
    return Execute(progressQuery_, variables, error);
}

bool AniListUpdateClient::UpdateScore(const int mediaId, const double score, QString &error) {
    QJsonObject variables{{QStringLiteral("mediaId"), mediaId},
                          {QStringLiteral("score"), score}};
    return Execute(scoreQuery_, variables, error);
}

bool AniListUpdateClient::UpdateListStatus(const int mediaId, const QString &status,
                                           QString &error) {
    QJsonObject variables{{QStringLiteral("mediaId"), mediaId},
                          {QStringLiteral("status"), status}};
    return Execute(listStatusQuery_, variables, error);
}

bool AniListUpdateClient::DeleteListEntry(const int mediaId, QString &error) {
    QJsonObject variables{{QStringLiteral("mediaId"), mediaId}};
    return Execute(deleteQuery_, variables, error);
}
