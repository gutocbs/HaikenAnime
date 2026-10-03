#ifndef HAIKENANIME_HOMESCREENCONTROLLER_H
#define HAIKENANIME_HOMESCREENCONTROLLER_H

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVariantList>

#include "../../application/media/IMediaReader.h"
#include "../../application/covers/CoverDownloadCoordinator.h"
#include "../../application/configuration/CardStatusPresentation.h"
#include "../../application/media/MediaTitleResolver.h"

class ILocalEpisodeReader;
class ILocalFileOpener;

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
    void ConfigureCardPresentation(CardStatusPresentation presentation, double scoreMaximum);
    void ConfigurePreferredTitle(QString key);

private:
    QList<Media> media_;
    QHash<int, QString> coverSources_;
    QHash<int, CoverState> coverStates_;
    CardStatusPresentation cardStatusPresentation_ = CardStatusPresentation::PersonalListStatus;
    double scoreMaximum_ = 10.0;
    QString preferredTitleKey_ = DefaultPreferredTitleKey();
};

class HomeScreenController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(HomeMediaModel *mediaModel READ mediaModel CONSTANT)
    Q_PROPERTY(HomeMediaModel *fullMediaModel READ fullMediaModel CONSTANT)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int synchronizationProgress READ synchronizationProgress NOTIFY synchronizationProgressChanged)
    Q_PROPERTY(bool synchronizationProgressKnown READ synchronizationProgressKnown NOTIFY synchronizationProgressChanged)
    Q_PROPERTY(int mediaCount READ mediaCount NOTIFY mediaCountChanged)
    Q_PROPERTY(int filteredMediaCount READ filteredMediaCount NOTIFY mediaCountChanged)
    Q_PROPERTY(QString activeMediaType READ activeMediaType NOTIFY activeMediaTypeChanged)
    Q_PROPERTY(QVariantList availableMediaTypeOptions READ availableMediaTypeOptions NOTIFY browseOptionsChanged)
    Q_PROPERTY(QVariantList availableListOptions READ availableListOptions NOTIFY browseOptionsChanged)
    Q_PROPERTY(QVariantList availableSortOptions READ availableSortOptions NOTIFY browseOptionsChanged)
    Q_PROPERTY(QString activeListFilter READ activeListFilter NOTIFY browseCriteriaChanged)
    Q_PROPERTY(QString activeSort READ activeSort NOTIFY browseCriteriaChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery NOTIFY browseCriteriaChanged)
    Q_PROPERTY(bool browseCriteriaActive READ browseCriteriaActive NOTIFY browseCriteriaChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(int selectedMediaId READ selectedMediaId NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSynopsis READ selectedSynopsis NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedTypeLabel READ selectedTypeLabel NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedStatusLabel READ selectedStatusLabel NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedProgress READ selectedProgress NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedScore READ selectedScore NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedAverageScore READ selectedAverageScore NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSeasonLabel READ selectedSeasonLabel NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedNextAiringLabel READ selectedNextAiringLabel NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedMediaLinks READ selectedMediaLinks NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCoverSource READ selectedCoverSource NOTIFY selectionChanged)
    Q_PROPERTY(int selectedProgressValue READ selectedProgressValue NOTIFY selectionChanged)
    Q_PROPERTY(int selectedProgressMaximum READ selectedProgressMaximum NOTIFY selectionChanged)
    Q_PROPERTY(double selectedScoreValue READ selectedScoreValue NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedListStatusKey READ selectedListStatusKey NOTIFY selectionChanged)
    Q_PROPERTY(QStringList selectedAlternativeNames READ selectedAlternativeNames NOTIFY selectionChanged)
    Q_PROPERTY(bool canWatch READ canWatch NOTIFY selectionChanged)
    Q_PROPERTY(int nextLocalEpisode READ nextLocalEpisode NOTIFY selectionChanged)
    Q_PROPERTY(QString localLibraryStatusMessage READ localLibraryStatusMessage NOTIFY selectionChanged)
    Q_PROPERTY(QString localLibraryErrorMessage READ localLibraryErrorMessage NOTIFY selectionChanged)
    Q_PROPERTY(double scoreMinimum READ scoreMinimum NOTIFY editingOptionsChanged)
    Q_PROPERTY(double scoreMaximum READ scoreMaximum NOTIFY editingOptionsChanged)
    Q_PROPERTY(double scoreStep READ scoreStep NOTIFY editingOptionsChanged)

public:
    explicit HomeScreenController(IMediaReader &reader, QObject *parent = nullptr);
    explicit HomeScreenController(IMediaReader *reader, QString initializationError = {}, QObject *parent = nullptr);
    HomeScreenController(IMediaReader *reader, CoverDownloadCoordinator *covers, CoverQuality quality,
                         QString initializationError = {}, QObject *parent = nullptr);

    HomeMediaModel *mediaModel();
    HomeMediaModel *fullMediaModel();
    QString state() const;
    QString statusMessage() const;
    QString errorMessage() const;
    int synchronizationProgress() const;
    bool synchronizationProgressKnown() const;
    int mediaCount() const;
    int filteredMediaCount() const;
    QString activeMediaType() const;
    QVariantList availableMediaTypeOptions() const;
    QVariantList availableListOptions() const;
    QVariantList availableSortOptions() const;
    QString activeListFilter() const;
    QString activeSort() const;
    QString searchQuery() const;
    bool browseCriteriaActive() const;
    bool hasSelection() const;
    int selectedMediaId() const;
    QString selectedTitle() const;
    QString selectedSynopsis() const;
    QString selectedTypeLabel() const;
    QString selectedStatusLabel() const;
    QString selectedProgress() const;
    QString selectedScore() const;
    QString selectedAverageScore() const;
    QString selectedSeasonLabel() const;
    QString selectedNextAiringLabel() const;
    QVariantList selectedMediaLinks() const;
    QString selectedCoverSource() const;
    int selectedProgressValue() const;
    int selectedProgressMaximum() const;
    double selectedScoreValue() const;
    QString selectedListStatusKey() const;
    QStringList selectedAlternativeNames() const;
    bool canWatch() const;
    int nextLocalEpisode() const;
    QString localLibraryStatusMessage() const;
    QString localLibraryErrorMessage() const;
    double scoreMinimum() const;
    double scoreMaximum() const;
    double scoreStep() const;

    void ConfigureBrowseOptions(QVariantList mediaTypeOptions, QVariantList listOptions,
                                QVariantList sortOptions);
    void ConfigureInitialSort(QString key);
    void ConfigureScoreScale(double minimum, double maximum, double step);
    void ConfigureCoverQuality(CoverQuality quality);
    void ConfigureCardStatusPresentation(CardStatusPresentation presentation);
    void ConfigurePreferredTitle(QString key);
    void SetLocalEpisodeServices(ILocalEpisodeReader *episodeReader, ILocalFileOpener *fileOpener);
    void RefreshLocalEpisode();

    void reload();
    void notifySynchronizationCompleted();
    void notifySynchronizationStarted();
    void notifySynchronizationProgress(int processedItems, int totalItems);
    void notifySynchronizationFailed(const QString &error);
    Q_INVOKABLE void SetMediaType(const QString &typeKey);
    Q_INVOKABLE void SetListFilter(const QString &filterKey);
    Q_INVOKABLE void SetSort(const QString &sortKey);
    Q_INVOKABLE void SetSearchQuery(const QString &query);
    Q_INVOKABLE void ClearBrowseCriteria();
    Q_INVOKABLE void SelectMedia(int mediaId);
    Q_INVOKABLE void WatchNext();
    Q_INVOKABLE QVariantMap PreviewCardMetadata(int progress, const QString &listStatusKey,
                                                double score) const;
    Q_INVOKABLE QVariantMap PreviewCompactDetailMetadata(int progress,
                                                         const QString &listStatusKey,
                                                         double score) const;
    Q_INVOKABLE void RequestCoverWindow(const QString &scope, int firstVisibleIndex,
                                        int lastVisibleIndex, int prefetchCount);
    Q_INVOKABLE void ReportCoverLoadFailure(int mediaId);
    Q_INVOKABLE void ClearCoverCache();

signals:
    void stateChanged();
    void errorMessageChanged();
    void statusMessageChanged();
    void synchronizationProgressChanged();
    void mediaCountChanged();
    void activeMediaTypeChanged();
    void browseOptionsChanged();
    void browseCriteriaChanged();
    void sortPreferenceChanged(QString key);
    void editingOptionsChanged();
    void selectionChanged();

private:
    void setState(QString state);
    void setStatusMessage(QString message);
    void rebuildMediaModels();
    void clearSelection();
    void refreshNextLocalEpisode();

    IMediaReader *reader_ = nullptr;
    HomeMediaModel model_;
    HomeMediaModel fullModel_;
    QList<Media> allMedia_;
    CoverDownloadCoordinator *covers_ = nullptr;
    CoverQuality coverQuality_ = CoverQuality::Medium;
    QString state_ = QStringLiteral("idle");
    QString statusMessage_ = QStringLiteral("Aguardando sincronização.");
    QString errorMessage_;
    int synchronizationProgress_ = 0;
    bool synchronizationProgressKnown_ = false;
    QString activeMediaType_ = QStringLiteral("anime");
    QString activeListFilter_ = QStringLiteral("all");
    QString activeSort_ = QStringLiteral("title_asc");
    QString searchQuery_;
    QVariantList listOptions_;
    QVariantList sortOptions_;
    QVariantList mediaTypeOptions_;
    bool usesDefaultBrowseOptions_ = true;
    Media selectedMedia_;
    bool hasSelection_ = false;
    ILocalEpisodeReader *episodeReader_ = nullptr;
    ILocalFileOpener *fileOpener_ = nullptr;
    int nextLocalEpisode_ = 0;
    QString nextLocalEpisodePath_;
    QString localLibraryStatusMessage_;
    QString localLibraryErrorMessage_;
    QString selectedCoverSource_ = QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg");
    double scoreMinimum_ = 0.0;
    double scoreMaximum_ = 10.0;
    double scoreStep_ = 1.0;
    CardStatusPresentation cardStatusPresentation_ = CardStatusPresentation::PersonalListStatus;
    QString preferredTitleKey_ = DefaultPreferredTitleKey();
};

#endif // HAIKENANIME_HOMESCREENCONTROLLER_H
