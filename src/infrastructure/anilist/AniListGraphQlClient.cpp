#include "AniListGraphQlClient.h"
#include "AniListGraphQlResponseParser.h"

#include <QEventLoop>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QThread>
#include <QUuid>

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
    Log(QStringLiteral("AniList GraphQL request started (attempt=%1, endpoint=%2, variables=%3).")
            .arg(attempt + 1)
            .arg(endpoint_.toString(), QString::fromUtf8(QJsonDocument(variables).toJson(QJsonDocument::Compact))));
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

    if (reply->error() != QNetworkReply::NoError) {
        error = reply->errorString();
        Log(QStringLiteral("AniList GraphQL request failed (attempt=%1, networkError=%2, detail=%3).")
                .arg(attempt + 1)
                .arg(static_cast<int>(reply->error()))
                .arg(error));
        reply->deleteLater();
        if (attempt < maxRetries_) {
            QThread::msleep(static_cast<unsigned long>(retryDelayMs_));
            return ExecuteWithAttempt(query, variables, response, error, attempt + 1);
        }
        return false;
    }

    const auto responsePayload = reply->readAll();
    const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();
    Log(QStringLiteral("AniList GraphQL request received HTTP %1 (%2 bytes).")
            .arg(statusCode).arg(responsePayload.size()));
    CaptureResponse(responsePayload);
    const bool parsed = AniListGraphQlResponseParser::parse(responsePayload, response, error);
    if (!parsed) {
        Log(QStringLiteral("AniList GraphQL response parsing failed: %1").arg(error));
    } else if (response.hasErrors()) {
        QStringList messages;
        for (const auto &graphQlError : response.errors) messages.append(graphQlError.message);
        Log(QStringLiteral("AniList GraphQL response contains GraphQL errors: %1")
                .arg(messages.join(QStringLiteral(" | "))));
    } else {
        Log(QStringLiteral("AniList GraphQL response parsed successfully."));
    }
    return parsed;
}

void AniListGraphQlClient::Log(QString message) const {
    if (diagnostics_.log) diagnostics_.log(std::move(message));
}

void AniListGraphQlClient::CaptureResponse(const QByteArray &payload) const {
    if (diagnostics_.responseCaptureDirectory.isEmpty()) return;
    QDir directory(diagnostics_.responseCaptureDirectory);
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        Log(QStringLiteral("AniList GraphQL response capture failed: unable to create directory."));
        return;
    }
    const auto fileName = QStringLiteral("anilist-response-%1-%2.json")
                              .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmsszzz")),
                                   QUuid::createUuid().toString(QUuid::WithoutBraces));
    QFile file(directory.filePath(fileName));
    if (!file.open(QIODevice::WriteOnly)) {
        Log(QStringLiteral("AniList GraphQL response capture failed: unable to open output file."));
        return;
    }
    if (file.write(payload) != payload.size()) {
        Log(QStringLiteral("AniList GraphQL response capture failed: unable to write complete response."));
        return;
    }
    Log(QStringLiteral("AniList GraphQL response captured to %1.").arg(file.fileName()));
}

QUrl AniListGraphQlClient::endpoint() const {
    return endpoint_;
}
