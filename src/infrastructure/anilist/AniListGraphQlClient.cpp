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
                                           int timeoutMs, int maxRetries, int retryDelayMs,
                                           Diagnostics diagnostics)
    : networkManager_(networkManager), authProvider_(authProvider), endpoint_(std::move(endpoint)),
      timeoutMs_(timeoutMs), maxRetries_(maxRetries), retryDelayMs_(retryDelayMs),
      diagnostics_(std::move(diagnostics)) {
}

bool AniListGraphQlClient::execute(const QString &query, const QJsonObject &variables,
                                   AniListGraphQlResponse &response, QString &error) const {
    return ExecuteWithAttempt(query, variables, response, error, 0);
}

bool AniListGraphQlClient::execute(const QString &query, const AniListDataSourceRequest &request,
                                   AniListGraphQlResponse &response, QString &error) const {
    return execute(query, request.variables, response, error);
}

bool AniListGraphQlClient::ExecuteWithAttempt(const QString &query, const QJsonObject &variables,
                                             AniListGraphQlResponse &response, QString &error,
                                             const int attempt) const {
    response = {};
    error.clear();
    Log(QStringLiteral("AniList GraphQL request started (attempt=%1, host=%2).")
            .arg(attempt + 1)
            .arg(endpoint_.host()));
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
        Log(QStringLiteral("AniList GraphQL request timed out (attempt=%1).").arg(attempt + 1));
        reply->deleteLater();
        if (attempt < maxRetries_) {
            QThread::msleep(static_cast<unsigned long>(retryDelayMs_));
            return ExecuteWithAttempt(query, variables, response, error, attempt + 1);
        }
        return false;
    }

    const auto responsePayload = reply->readAll();
    const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto networkError = reply->error();
    const auto networkErrorDetail = reply->errorString();
    reply->deleteLater();
    Log(QStringLiteral("AniList GraphQL request received HTTP %1 (%2 bytes).")
            .arg(statusCode).arg(responsePayload.size()));

    if (networkError != QNetworkReply::NoError) {
        AniListGraphQlResponse errorResponse;
        QString responseParseError;
        QStringList graphQlErrorMessages;
        if (AniListGraphQlResponseParser::parse(
                responsePayload, errorResponse, responseParseError)
            && errorResponse.hasErrors()) {
            response = std::move(errorResponse);
            for (const auto &graphQlError : response.errors) {
                graphQlErrorMessages.append(graphQlError.message);
            }
            error = graphQlErrorMessages.join(QStringLiteral(" | "));
        } else {
            error = networkErrorDetail;
        }
        Log(QStringLiteral("AniList GraphQL request failed (attempt=%1, HTTP=%2, networkError=%3).")
                .arg(attempt + 1)
                .arg(statusCode)
                .arg(static_cast<int>(networkError)));
        if (attempt < maxRetries_) {
            QThread::msleep(static_cast<unsigned long>(retryDelayMs_));
            return ExecuteWithAttempt(query, variables, response, error, attempt + 1);
        }
        return false;
    }

    const bool parsed = AniListGraphQlResponseParser::parse(responsePayload, response, error);
    if (!parsed) {
        Log(QStringLiteral("AniList GraphQL response parsing failed."));
    } else if (response.hasErrors()) {
        Log(QStringLiteral("AniList GraphQL response contains %1 GraphQL error(s).").arg(response.errors.size()));
    } else {
        Log(QStringLiteral("AniList GraphQL request succeeded."));
    }
    return parsed;
}

void AniListGraphQlClient::Log(QString message) const {
    if (diagnostics_.log) diagnostics_.log(std::move(message));
}

QUrl AniListGraphQlClient::endpoint() const {
    return endpoint_;
}
