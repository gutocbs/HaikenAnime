#include "SqlQueryStore.h"

#include <QFile>
#include "../logging/AsyncLogger.h"

#include <utility>

SqlQueryStore::SqlQueryStore(QString filePath)
    : filePath_(std::move(filePath)) {
}

void SqlQueryStore::setLogger(AsyncLogger *logger) { logger_ = logger; }

bool SqlQueryStore::load(QString &query, QString &error) const {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open SQL query file: %1").arg(file.errorString());
        if (logger_) logger_->error(LogCategory::QueryStore, error);
        return false;
    }

    query = QString::fromUtf8(file.readAll()).trimmed();
    if (query.isEmpty()) {
        error = QStringLiteral("SQL query file is empty.");
        if (logger_) logger_->error(LogCategory::QueryStore, error);
        return false;
    }
    if (logger_) logger_->info(LogCategory::QueryStore, QStringLiteral("Loaded SQL query file: %1").arg(filePath_));
    return true;
}
