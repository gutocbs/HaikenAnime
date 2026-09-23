#include "AniListGraphQlClient.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <utility>

AniListGraphQlClient::AniListGraphQlClient(QNetworkAccessManager &networkManager,
                                           IAniListAuthProvider *authProvider, QUrl endpoint,
                                           int timeoutMs)
    : networkManager_(networkManager), authProvider_(authProvider), endpoint_(std::move(endpoint)),
      timeoutMs_(timeoutMs) {
}

bool AniListGraphQlClient::execute(const QString &query, const QJsonObject &variables,
                                   AniListGraphQlResponse &response, QString &error) const {
    response = {};
    error.clear();
    QNetworkRequest request(endpoint_);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    if (authProvider_ != nullptr) {
        const auto token = authProvider_->credentials().token;
        if (!token.isEmpty()) {
            request.setRawHeader("Authorization", QByteArray("Bearer ") + token.toUtf8());
        }
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("query"), query);
    payload.insert(QStringLiteral("variables"), variables);

    QNetworkReply *reply = networkManager_.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop eventLoop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(timeoutMs_);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    timeout.start();
    eventLoop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        error = QStringLiteral("AniList request timed out.");
        reply->deleteLater();
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        error = reply->errorString();
        reply->deleteLater();
        return false;
    }

    const auto document = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();
    if (!document.isObject()) {
        error = QStringLiteral("AniList returned an invalid JSON response.");
        return false;
    }

    const auto root = document.object();
    if (root.value(QStringLiteral("data")).isObject()) {
        response.data = root.value(QStringLiteral("data")).toObject();
        response.dataWasPresent = true;
    }

    const auto errors = root.value(QStringLiteral("errors"));
    if (errors.isArray()) {
        for (const auto &errorValue : errors.toArray()) {
            if (!errorValue.isObject()) {
                continue;
            }

            const auto errorObject = errorValue.toObject();
            AniListGraphQlError graphQlError;
            graphQlError.message = errorObject.value(QStringLiteral("message")).toString();
            const auto path = errorObject.value(QStringLiteral("path"));
            if (path.isArray()) {
                for (const auto &pathValue : path.toArray()) {
                    graphQlError.path.append(pathValue.toVariant().toString());
                }
            }
            response.errors.append(graphQlError);
        }
    }

    if (!response.hasData() && !response.hasErrors()) {
        error = QStringLiteral("AniList response did not contain data or errors.");
        return false;
    }

    return true;
}

QUrl AniListGraphQlClient::endpoint() const {
    return endpoint_;
}
