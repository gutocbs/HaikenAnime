#ifndef HAIKENANIME_ASYNCLOGGER_H
#define HAIKENANIME_ASYNCLOGGER_H

#include <QMutex>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QWaitCondition>

#include <functional>

enum class LogLevel { Info, Warning, Error };
enum class LogCategory { Application, Configuration, Database, Migration, QueryStore, Sync, Covers, LocalLibrary };

[[nodiscard]] QString toLogString(LogLevel level);
[[nodiscard]] QString toLogString(LogCategory category);

class AsyncLogger final {
public:
    explicit AsyncLogger(int retentionDays = 7);
    ~AsyncLogger();
    /// Folder that holds the daily log files.
    [[nodiscard]] QString logDirectory() const { return logDirectory_; }

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

/// Callback writing each message to logger at level, for components that take a plain logging function.
/// Does nothing when logger is null.
[[nodiscard]] std::function<void(const QString &)> LogSink(AsyncLogger *logger, LogCategory category,
                                                         LogLevel level = LogLevel::Info);

#endif // HAIKENANIME_ASYNCLOGGER_H
