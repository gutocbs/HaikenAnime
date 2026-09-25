#ifndef HAIKENANIME_HOMESCREENCONTROLLER_H
#define HAIKENANIME_HOMESCREENCONTROLLER_H

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include "../../application/media/IMediaReader.h"
#include "../../application/covers/CoverDownloadCoordinator.h"

class HomeMediaModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        ProgressRole,
        ScoreRole,
        StatusLabelRole,
        RemoteCoverUrlRole,
        CoverSourceRole,
        CoverStateRole
    };

    explicit HomeMediaModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMedia(QList<Media> media);
    bool UpdateCover(int mediaId, QString source, CoverState state);
    QList<Media> media() const;
    void ClearCovers();

private:
    QList<Media> media_;
    QHash<int, QString> coverSources_;
    QHash<int, CoverState> coverStates_;
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
    explicit HomeScreenController(IMediaReader &reader, QObject *parent = nullptr);
    explicit HomeScreenController(IMediaReader *reader, QString initializationError = {}, QObject *parent = nullptr);
    HomeScreenController(IMediaReader *reader, CoverDownloadCoordinator *covers, CoverQuality quality,
                         QString initializationError = {}, QObject *parent = nullptr);

    HomeMediaModel *mediaModel();
    QString state() const;
    QString statusMessage() const;
    QString errorMessage() const;
    int synchronizationProgress() const;
    bool synchronizationProgressKnown() const;
    int mediaCount() const;

    void reload();
    void notifySynchronizationCompleted();
    void notifySynchronizationStarted();
    void notifySynchronizationProgress(int processedItems, int totalItems);
    void notifySynchronizationFailed(const QString &error);
    Q_INVOKABLE void RequestCoverWindow(int firstVisibleIndex, int lastVisibleIndex, int prefetchCount);
    Q_INVOKABLE void ReportCoverLoadFailure(int mediaId);
    Q_INVOKABLE void ClearCoverCache();

signals:
    void stateChanged();
    void errorMessageChanged();
    void statusMessageChanged();
    void synchronizationProgressChanged();
    void mediaCountChanged();

private:
    void setState(QString state);
    void setStatusMessage(QString message);

    IMediaReader *reader_ = nullptr;
    HomeMediaModel model_;
    CoverDownloadCoordinator *covers_ = nullptr;
    CoverQuality coverQuality_ = CoverQuality::Medium;
    QString state_ = QStringLiteral("idle");
    QString statusMessage_ = QStringLiteral("Aguardando sincronização.");
    QString errorMessage_;
    int synchronizationProgress_ = 0;
    bool synchronizationProgressKnown_ = false;
};

#endif // HAIKENANIME_HOMESCREENCONTROLLER_H
