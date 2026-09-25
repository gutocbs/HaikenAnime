#include "AniListGraphQlClient.h"
#include "AniListGraphQlResponseParser.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QThread>

#include <utility>

AniListGraphQlClient::AniListGraphQlClient(QNetworkAccessManager &networkManager,
                                           IAniListAuthProvider *authProvider, QUrl endpoint,
                                           int timeoutMs, int maxRetries, int retryDelayMs)
    : networkManager_(networkManager), authProvider_(authProvider), endpoint_(std::move(endpoint)),
      timeoutMs_(timeoutMs), maxRetries_(maxRetries), retryDelayMs_(retryDelayMs) {
}

bool AniListGraphQlClient::execute(const QString &query, const QJsonObject &variables,
                                   AniListGraphQlResponse &response, QString &error) const {
    return ExecuteWithAttempt(query, variables, response, error, 0);
}

bool AniListGraphQlClient::ExecuteWithAttempt(const QString &query, const QJsonObject &variables,
                                             AniListGraphQlResponse &response, QString &error,
                                             const int attempt) const {
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
        if (attempt < maxRetries_) {
            QThread::msleep(static_cast<unsigned long>(retryDelayMs_));
            return ExecuteWithAttempt(query, variables, response, error, attempt + 1);
        }
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        error = reply->errorString();
        reply->deleteLater();
        if (attempt < maxRetries_) {
            QThread::msleep(static_cast<unsigned long>(retryDelayMs_));
            return ExecuteWithAttempt(query, variables, response, error, attempt + 1);
        }
        return false;
    }

    const auto responsePayload = reply->readAll();
    reply->deleteLater();
    return AniListGraphQlResponseParser::parse(responsePayload, response, error);
}

QUrl AniListGraphQlClient::endpoint() const {
    return endpoint_;
}
