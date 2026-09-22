#ifndef HAIKENANIME_SQLITEDATABASE_H
#define HAIKENANIME_SQLITEDATABASE_H

#include <QSqlDatabase>
#include <QString>

class SqliteDatabase {
public:
    explicit SqliteDatabase(QString databasePath = {});
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
    QSqlDatabase database_;
};

#endif // HAIKENANIME_SQLITEDATABASE_H
