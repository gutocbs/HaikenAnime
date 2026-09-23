#ifndef HAIKENANIME_HOMESCREENCONTROLLER_H
#define HAIKENANIME_HOMESCREENCONTROLLER_H

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include "../../application/anilist/IMediaRepository.h"

class HomeMediaModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        ProgressRole,
        ScoreRole,
        StatusRole,
        CoverUrlRole
    };

    explicit HomeMediaModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMedia(QList<Media> media);

private:
    QList<Media> media_;
};

class HomeScreenController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(HomeMediaModel *mediaModel READ mediaModel CONSTANT)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int synchronizationProgress READ synchronizationProgress NOTIFY synchronizationProgressChanged)
    Q_PROPERTY(bool synchronizationProgressKnown READ synchronizationProgressKnown NOTIFY synchronizationProgressChanged)
    Q_PROPERTY(int mediaCount READ mediaCount NOTIFY mediaCountChanged)

public:
    explicit HomeScreenController(IMediaRepository &repository, QObject *parent = nullptr);
    explicit HomeScreenController(IMediaRepository *repository, QString initializationError = {}, QObject *parent = nullptr);

    HomeMediaModel *mediaModel();
    QString state() const;
    QString statusMessage() const;
    QString errorMessage() const;
    int synchronizationProgress() const;
    bool synchronizationProgressKnown() const;
    int mediaCount() const;

    Q_INVOKABLE void reload();
    void notifySynchronizationCompleted();
    void notifySynchronizationStarted();
    void notifySynchronizationProgress(int processedItems, int totalItems);
    void notifySynchronizationFailed(const QString &error);

signals:
    void stateChanged();
    void errorMessageChanged();
    void statusMessageChanged();
    void synchronizationProgressChanged();
    void mediaCountChanged();
    void mediaUpdated();
    void synchronizationCompleted();
    void synchronizationError();

private:
    void setState(QString state);
    void setStatusMessage(QString message);

    IMediaRepository *repository_ = nullptr;
    HomeMediaModel model_;
    QString state_ = QStringLiteral("idle");
    QString statusMessage_ = QStringLiteral("Aguardando sincronização.");
    QString errorMessage_;
    int synchronizationProgress_ = 0;
    bool synchronizationProgressKnown_ = false;
};

#endif // HAIKENANIME_HOMESCREENCONTROLLER_H
