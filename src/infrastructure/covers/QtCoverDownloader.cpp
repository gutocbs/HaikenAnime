#include "QtCoverDownloader.h"
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryFile>
#include <QTimer>

QtCoverDownloader::QtCoverDownloader(QString temporaryDirectory, CoverSettings settings, QObject *parent)
    : QObject(parent), directory_(std::move(temporaryDirectory)), settings_(settings)
{
    QDir().mkpath(directory_);
}

quint64 QtCoverDownloader::Start(const CoverRequest &request, Completion completion)
{
    const quint64 id = nextId_++;
    auto temporary = new QTemporaryFile(QDir(directory_).filePath("cover-XXXXXX.tmp"), this);
    temporary->setAutoRemove(false);
    if (!temporary->open()) {
        CoverDownloadResult result; result.request = request; result.failure = CoverFailureCategory::FileSystem;
        result.error = temporary->errorString(); delete temporary; completion(std::move(result)); return id;
    }
    const QString path = temporary->fileName();
    auto reply = manager_.get(QNetworkRequest(request.remoteUrl));
    auto timer = new QTimer(reply); timer->setSingleShot(true); timer->start(settings_.timeoutMs);
    transfers_.insert(id, {request, std::move(completion), reply, temporary, timer, path});
    connect(reply, &QNetworkReply::readyRead, this, [this, id] {
        auto it = transfers_.find(id); if (it == transfers_.end()) return;
        const QByteArray chunk = it->reply->readAll();
        if (it->file->size() + chunk.size() > settings_.maxResponseBytes) { it->oversized = true; it->reply->abort(); return; }
        if (it->file->write(chunk) != chunk.size()) it->reply->abort();
    });
    connect(timer, &QTimer::timeout, this, [reply] { reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, id] { Finish(id); });
    return id;
}

void QtCoverDownloader::Cancel(quint64 requestId)
{
    auto it = transfers_.find(requestId);
    if (it == transfers_.end()) return;
    it->cancelled = true;
    it->reply->abort();
}

void QtCoverDownloader::Finish(quint64 id)
{
    auto it = transfers_.find(id); if (it == transfers_.end()) return;
    Transfer transfer = std::move(it.value()); transfers_.erase(it);
    transfer.timer->stop();
    if (transfer.reply->isOpen()) {
        const QByteArray tail = transfer.reply->readAll();
        if (transfer.file->size() + tail.size() > settings_.maxResponseBytes) transfer.oversized = true;
        else transfer.file->write(tail);
    }
    transfer.file->close();
    CoverDownloadResult result; result.request = transfer.request; result.temporaryPath = transfer.path;
    result.httpStatus = transfer.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.mimeType = transfer.reply->header(QNetworkRequest::ContentTypeHeader).toString().section(';', 0, 0);
    result.etag = QString::fromUtf8(transfer.reply->rawHeader("ETag"));
    result.lastModified = QString::fromUtf8(transfer.reply->rawHeader("Last-Modified"));
    bool retryOk = false;
    const int seconds = transfer.reply->rawHeader("Retry-After").toInt(&retryOk);
    result.retryAfterMs = retryOk ? seconds * 1000 : 0;
    if (transfer.cancelled) result.failure = CoverFailureCategory::Cancelled;
    else if (transfer.oversized) { result.failure = CoverFailureCategory::InvalidContent; result.error = "Response exceeds configured size."; }
    else if (result.httpStatus >= 200 && result.httpStatus < 300 && transfer.reply->error() == QNetworkReply::NoError) result.succeeded = true;
    else if (result.httpStatus == 408 || result.httpStatus == 429 || result.httpStatus >= 500) result.failure = CoverFailureCategory::HttpTemporary;
    else if (result.httpStatus >= 400) result.failure = CoverFailureCategory::HttpPermanent;
    else result.failure = CoverFailureCategory::Transport;
    if (!result.succeeded) {
        if (result.error.isEmpty()) result.error = transfer.reply->errorString();
        transfer.file->remove();
    }
    transfer.reply->deleteLater(); transfer.file->deleteLater();
    transfer.completion(std::move(result));
}
