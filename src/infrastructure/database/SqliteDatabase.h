#ifndef HAIKENANIME_SQLITEDATABASE_H
#define HAIKENANIME_SQLITEDATABASE_H

#include <QSqlDatabase>
#include <QString>
class AsyncLogger;

class SqliteDatabase {
public:
    explicit SqliteDatabase(QString databasePath = {});
    void setLogger(AsyncLogger *logger);
    ~SqliteDatabase();

    bool open();
    bool migrate();
    void close();

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QString databasePath() const;
    [[nodiscard]] QSqlDatabase connection() const;

private:
    QString connectionName_;
    QString databasePath_;
    QString lastError_;
    QSqlDatabase database_;
    AsyncLogger *logger_ = nullptr;
};

#endif // HAIKENANIME_SQLITEDATABASE_H
