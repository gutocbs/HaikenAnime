#ifndef HAIKENANIME_QTCOVERDOWNLOADER_H
#define HAIKENANIME_QTCOVERDOWNLOADER_H

#include "../../application/covers/CoverSettings.h"
#include "../../application/covers/ICoverDownloader.h"
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>

class QFile;
class QNetworkReply;
class QTimer;

class QtCoverDownloader final : public QObject, public ICoverDownloader {
public:
    QtCoverDownloader(QString temporaryDirectory, CoverSettings settings, QObject *parent = nullptr);
    quint64 Start(const CoverRequest &request, Completion completion) override;
    void Cancel(quint64 requestId) override;

private:
    struct Transfer {
        CoverRequest request;
        Completion completion;
        QNetworkReply *reply = nullptr;
        QFile *file = nullptr;
        QTimer *timer = nullptr;
        QString path;
        bool cancelled = false;
        bool oversized = false;
    };
    void Finish(quint64 id);
    QString directory_;
    CoverSettings settings_;
    QNetworkAccessManager manager_;
    QHash<quint64, Transfer> transfers_;
    quint64 nextId_ = 1;
};
#endif
