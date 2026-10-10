#include "AniListUpdateClient.h"

#include "AniListGraphQlClient.h"
#include "GraphQlQueryStore.h"

#include <QJsonObject>

#include <variant>

AniListUpdateClient::AniListUpdateClient(AniListGraphQlClient &graphQlClient,
                                         GraphQlQueryStore &updateQuery)
    : graphQlClient_(graphQlClient), updateQuery_(updateQuery) {
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
