#ifndef HAIKENANIME_INITIALSYNCCOORDINATOR_H
#define HAIKENANIME_INITIALSYNCCOORDINATOR_H

#include <QObject>

class AsyncLogger;

class InitialSyncCoordinator final : public QObject {
    Q_OBJECT
public:
    explicit InitialSyncCoordinator(QString databasePath, QString fixturePath,
                                    QString upsertQueryPath, QString readQueryPath,
                                    QObject *parent = nullptr);
    void setLogger(AsyncLogger *logger);
public slots:
    void start();
signals:
    void started();
    void completed();
    void failed(const QString &error);
private:
    QString databasePath_;
    QString fixturePath_;
    QString upsertQueryPath_;
    QString readQueryPath_;
    AsyncLogger *logger_ = nullptr;
};

#endif
