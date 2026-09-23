#ifndef HAIKENANIME_SQLQUERYSTORE_H
#define HAIKENANIME_SQLQUERYSTORE_H

#include <QString>

/** Loads SQL statements from external files. */
class SqlQueryStore final {
public:
    /** Creates a query store for the supplied SQL file path. */
    explicit SqlQueryStore(QString filePath);

    /** Reads the complete SQL statement and reports filesystem errors through error. */
    [[nodiscard]] bool load(QString &query, QString &error) const;

private:
    QString filePath_;
};

#endif // HAIKENANIME_SQLQUERYSTORE_H
