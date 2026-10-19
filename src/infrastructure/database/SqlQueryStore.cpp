#include "SqlQueryStore.h"

#include <QFile>
#include <QHash>
#include <QMutex>
#include "../logging/AsyncLogger.h"

#include <utility>

namespace {
// Qt resources never change while the app runs: each one is read once and shared by every thread.
QMutex cacheMutex;
QHash<QString, QString> resourceCache;

bool IsResource(const QString &source) {
    return source.startsWith(QStringLiteral(":/")) || source.startsWith(QStringLiteral("qrc:"));
}
}

SqlQueryStore::SqlQueryStore(QString filePath)
    : filePath_(std::move(filePath)) {
}

void SqlQueryStore::setLogger(AsyncLogger *logger) { logger_ = logger; }

bool SqlQueryStore::load(QString &query, QString &error) const {
    query.clear();
    error.clear();
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open SQL query file '%1': %2")
                    .arg(filePath_, file.errorString());
        if (logger_) logger_->error(LogCategory::QueryStore, error);
        return false;
    }

    query = QString::fromUtf8(file.readAll()).trimmed();
    if (query.isEmpty()) {
        error = QStringLiteral("SQL query file '%1' is empty.").arg(filePath_);
        if (logger_) logger_->error(LogCategory::QueryStore, error);
        return false;
    }
    if (logger_) logger_->info(LogCategory::QueryStore, QStringLiteral("Loaded SQL query file: %1").arg(filePath_));
    return true;
}

bool SqlQueryStore::loadSource(const QString &source, QString &query, QString &error,
                               AsyncLogger *logger) {
    if (IsResource(source)) {
        QMutexLocker locker(&cacheMutex);
        const auto cached = resourceCache.constFind(source);
        if (cached != resourceCache.cend()) {
            query = cached.value();
            error.clear();
            return true;
        }
    }
    SqlQueryStore store(source);
    store.setLogger(logger);
    if (!store.load(query, error)) return false;
    if (IsResource(source)) {
        QMutexLocker locker(&cacheMutex);
        resourceCache.insert(source, query);
    }
    return true;
}
