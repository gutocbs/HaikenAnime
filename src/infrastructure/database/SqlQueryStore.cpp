#include "SqlQueryStore.h"

#include <QFile>

#include <utility>

SqlQueryStore::SqlQueryStore(QString filePath)
    : filePath_(std::move(filePath)) {
}

bool SqlQueryStore::load(QString &query, QString &error) const {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open SQL query file: %1").arg(file.errorString());
        return false;
    }

    query = QString::fromUtf8(file.readAll()).trimmed();
    if (query.isEmpty()) {
        error = QStringLiteral("SQL query file is empty.");
        return false;
    }
    return true;
}
