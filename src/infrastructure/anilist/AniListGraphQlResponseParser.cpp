#include "AniListGraphQlResponseParser.h"

#include <QJsonArray>
#include <QJsonDocument>

bool AniListGraphQlResponseParser::parse(const QByteArray &payload,
                                         AniListGraphQlResponse &response, QString &error) {
    response = {};
    error.clear();
    AniListGraphQlResponse parsedResponse;

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("AniList returned an invalid JSON response.");
        return false;
    }

    const auto root = document.object();
    const auto data = root.value(QStringLiteral("data"));
    if (data.isObject()) {
        parsedResponse.data = data.toObject();
        parsedResponse.dataWasPresent = true;
    }

    const auto errors = root.value(QStringLiteral("errors"));
    if (!errors.isUndefined() && !errors.isArray()) {
        error = QStringLiteral("AniList returned an invalid GraphQL errors field.");
        return false;
    }

    for (const auto &errorValue : errors.toArray()) {
        if (!errorValue.isObject()) {
            error = QStringLiteral("AniList returned a malformed GraphQL error.");
            return false;
        }

        const auto errorObject = errorValue.toObject();
        const auto message = errorObject.value(QStringLiteral("message"));
        if (!message.isString() || message.toString().trimmed().isEmpty()) {
            error = QStringLiteral("AniList returned a GraphQL error without a message.");
            return false;
        }

        AniListGraphQlError graphQlError;
        graphQlError.message = message.toString();
        const auto path = errorObject.value(QStringLiteral("path"));
        if (path.isArray()) {
            for (const auto &pathValue : path.toArray()) {
                graphQlError.path.append(pathValue.toVariant().toString());
            }
        }
        parsedResponse.errors.append(graphQlError);
    }

    if (!parsedResponse.hasData() && !parsedResponse.hasErrors()) {
        error = QStringLiteral("AniList response did not contain data or errors.");
        return false;
    }

    response = std::move(parsedResponse);
    return true;
}
