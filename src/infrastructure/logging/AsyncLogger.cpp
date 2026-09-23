#include "AsyncLogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QStandardPaths>
#include <QTextStream>

#include <utility>

QString toLogString(LogLevel level) {
    switch (level) {
    case LogLevel::Info: return QStringLiteral("INFO");
    case LogLevel::Warning: return QStringLiteral("WARN");
    case LogLevel::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("UNKNOWN");
}

QString toLogString(LogCategory category) {
    switch (category) {
    case LogCategory::Application: return QStringLiteral("Application");
    case LogCategory::Configuration: return QStringLiteral("Configuration");
    case LogCategory::Database: return QStringLiteral("Database");
    case LogCategory::Migration: return QStringLiteral("Migration");
    case LogCategory::QueryConfiguration: return QStringLiteral("QueryConfiguration");
    case LogCategory::QueryStore: return QStringLiteral("QueryStore");
    case LogCategory::Sync: return QStringLiteral("Sync");
    }
    return QStringLiteral("Unknown");
}

AsyncLogger::AsyncLogger(int retentionDays)
    : logDirectory_(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                        .filePath(QStringLiteral("logs"))),
      retentionDays_(qMax(1, retentionDays)) {
}

AsyncLogger::~AsyncLogger() {
    stop();
}

void AsyncLogger::start() {
    QMutexLocker locker(&mutex_);
    if (running_) {
        return;
    }
    if (!QDir().mkpath(logDirectory_)) {
        return;
    }
    logFilePath_ = QDir(logDirectory_).filePath(
        QStringLiteral("haikenanime-%1.log").arg(QDate::currentDate().toString(Qt::ISODate)));
    pruneOldFiles();
    running_ = true;
    stopping_ = false;
    QObject::connect(&thread_, &QThread::started, &thread_, [this]() { run(); }, Qt::DirectConnection);
    thread_.start();
}

void AsyncLogger::stop() {
    {
        QMutexLocker locker(&mutex_);
        if (!running_) {
            return;
        }
        stopping_ = true;
        condition_.wakeOne();
    }
    thread_.wait(2000);
    QMutexLocker locker(&mutex_);
    running_ = false;
}

void AsyncLogger::info(LogCategory category, const QString &message) {
    enqueue(LogLevel::Info, category, message);
}

void AsyncLogger::warning(LogCategory category, const QString &message) {
    enqueue(LogLevel::Warning, category, message);
}

void AsyncLogger::error(LogCategory category, const QString &message) {
    enqueue(LogLevel::Error, category, message);
}

void AsyncLogger::enqueue(LogLevel level, LogCategory category, QString message) {
    QMutexLocker locker(&mutex_);
    if (!running_ || stopping_) {
        return;
    }
    if (entries_.size() >= 4096) {
        entries_.dequeue();
    }
    entries_.enqueue({std::move(level), std::move(category), std::move(message)});
    condition_.wakeOne();
}

void AsyncLogger::run() {
    writeSessionSeparator();
    while (true) {
        Entry entry;
        {
            QMutexLocker locker(&mutex_);
            while (entries_.isEmpty() && !stopping_) {
                condition_.wait(&mutex_);
            }
            if (entries_.isEmpty() && stopping_) {
                break;
            }
            entry = entries_.dequeue();
        }
        writeEntry(entry);
    }
}

void AsyncLogger::writeEntry(const Entry &entry) {
    QFile file(logFilePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream stream(&file);
    stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " ["
           << toLogString(entry.level) << "] [" << toLogString(entry.category) << "] "
           << entry.message << Qt::endl;
}

void AsyncLogger::writeSessionSeparator() {
    writeEntry({LogLevel::Info, LogCategory::Application,
                QStringLiteral("===== application execution started =====")});
}

void AsyncLogger::pruneOldFiles() const {
    const auto files = QDir(logDirectory_).entryInfoList({QStringLiteral("haikenanime-*.log")},
                                                          QDir::Files | QDir::Readable);
    const auto cutoff = QDateTime::currentDateTime().addDays(-retentionDays_);
    for (const auto &file : files) {
        if (file.lastModified() < cutoff) {
            QFile::remove(file.absoluteFilePath());
        }
    }
}
