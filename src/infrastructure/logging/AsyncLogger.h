#ifndef HAIKENANIME_ASYNCLOGGER_H
#define HAIKENANIME_ASYNCLOGGER_H

#include <QMutex>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QWaitCondition>

enum class LogLevel { Info, Warning, Error };
enum class LogCategory { Application, Configuration, Database, Migration, QueryConfiguration, QueryStore, Sync, Covers };

[[nodiscard]] QString toLogString(LogLevel level);
[[nodiscard]] QString toLogString(LogCategory category);

class AsyncLogger final {
public:
    explicit AsyncLogger(int retentionDays = 7);
    ~AsyncLogger();

    AsyncLogger(const AsyncLogger &) = delete;
    AsyncLogger &operator=(const AsyncLogger &) = delete;

    void start();
    void stop();
    void info(LogCategory category, const QString &message);
    void warning(LogCategory category, const QString &message);
    void error(LogCategory category, const QString &message);

private:
    struct Entry {
        LogLevel level;
        LogCategory category;
        QString message;
    };

    void enqueue(LogLevel level, LogCategory category, QString message);
    void run();
    void writeEntry(const Entry &entry);
    void writeSessionSeparator();
    void pruneOldFiles() const;

    QMutex mutex_;
    QWaitCondition condition_;
    QQueue<Entry> entries_;
    QThread thread_;
    QString logDirectory_;
    QString logFilePath_;
    int retentionDays_;
    bool running_ = false;
    bool stopping_ = false;
};

#endif // HAIKENANIME_ASYNCLOGGER_H
