#ifndef HAIKENANIME_SQLQUERYSTORE_H
#define HAIKENANIME_SQLQUERYSTORE_H

#include <QString>
class AsyncLogger;

/** Loads SQL statements from external files. */
class SqlQueryStore final {
public:
    /** Creates a query store for the supplied SQL file path. */
    explicit SqlQueryStore(QString filePath);
    void setLogger(AsyncLogger *logger);

    /** Reads the complete SQL statement and reports filesystem errors through error. */
    [[nodiscard]] bool load(QString &query, QString &error) const;

private:
    QString filePath_;
    AsyncLogger *logger_ = nullptr;
};

#endif // HAIKENANIME_SQLQUERYSTORE_H
