#include "GraphQlQueryStore.h"

#include <QFile>

#include <utility>

GraphQlQueryStore::GraphQlQueryStore(QString filePath)
    : filePath_(std::move(filePath)) {
}

bool GraphQlQueryStore::load(QString &query, QString &error) const {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open GraphQL query file: %1").arg(file.errorString());
        return false;
    }

    query = QString::fromUtf8(file.readAll()).trimmed();
    if (query.isEmpty()) {
        error = QStringLiteral("GraphQL query file is empty.");
        return false;
    }

    return true;
}
