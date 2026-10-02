#ifndef HAIKENANIME_SEASONALCATALOGCONTROLLER_H
#define HAIKENANIME_SEASONALCATALOGCONTROLLER_H

#include <QAbstractListModel>
#include <QObject>
#include <QVariantList>

#include "../../application/covers/CoverQuality.h"
#include "../../application/media/MediaTitleResolver.h"

class SeasonalCatalogCoordinator;

class SeasonalCatalogMediaModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { MediaIdRole = Qt::UserRole + 1, TitleRole, StatusLabelRole, ProgressRole,
                ScoreRole, CoverSourceRole };

    explicit SeasonalCatalogMediaModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMedia(QList<Media> media);
    void configurePresentation(CoverQuality quality, QString preferredTitleKey);

private:
    QList<Media> media_;
    CoverQuality coverQuality_ = CoverQuality::Medium;
    QString preferredTitleKey_ = DefaultPreferredTitleKey();
};

/** Adapts the explicit seasonal coordinator state into the read-only QML presentation contract. */
class SeasonalCatalogController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(SeasonalCatalogMediaModel *mediaModel READ mediaModel CONSTANT)
    Q_PROPERTY(QVariantList availableYearOptions READ availableYearOptions CONSTANT)
    Q_PROPERTY(QVariantList availableSeasonOptions READ availableSeasonOptions CONSTANT)
    Q_PROPERTY(int selectedYear READ selectedYear NOTIFY filtersChanged)
    Q_PROPERTY(QString selectedSeasonKey READ selectedSeasonKey NOTIFY filtersChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(bool canLoadNextPage READ canLoadNextPage NOTIFY stateChanged)
    Q_PROPERTY(bool hasResults READ hasResults NOTIFY stateChanged)
    Q_PROPERTY(bool includeAdultContent READ includeAdultContent NOTIFY stateChanged)
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
    Q_PROPERTY(QStringList selectedAlternativeNames READ selectedAlternativeNames NOTIFY selectionChanged)

public:
    explicit SeasonalCatalogController(SeasonalCatalogCoordinator *coordinator,
                                       CoverQuality quality = CoverQuality::Medium,
                                       QString preferredTitleKey = DefaultPreferredTitleKey(),
                                       QObject *parent = nullptr);

    SeasonalCatalogMediaModel *mediaModel();
    QVariantList availableYearOptions() const;
    QVariantList availableSeasonOptions() const;
    int selectedYear() const;
    QString selectedSeasonKey() const;
    QString state() const;
    QString errorMessage() const;
    bool canLoadNextPage() const;
    bool hasResults() const;
    bool includeAdultContent() const;
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
    QStringList selectedAlternativeNames() const;

    void ConfigureCoverQuality(CoverQuality quality);
    void ConfigurePreferredTitle(QString key);
    void ConfigureIncludeAdultContent(bool enabled);

    Q_INVOKABLE void SetYear(int year);
    Q_INVOKABLE void SetSeason(const QString &seasonKey);
    Q_INVOKABLE void Retry();
    Q_INVOKABLE void LoadNextPage();
    Q_INVOKABLE void SelectMedia(int mediaId);

signals:
    void filtersChanged();
    void stateChanged();
    void selectionChanged();

private:
    void synchronizeFromCoordinator();
    void clearSelection();
    const Media *selectedMedia() const;

    SeasonalCatalogCoordinator *coordinator_ = nullptr;
    SeasonalCatalogMediaModel mediaModel_;
    QList<Media> media_;
    int selectedMediaId_ = 0;
    CoverQuality coverQuality_ = CoverQuality::Medium;
    QString preferredTitleKey_ = DefaultPreferredTitleKey();
};

#endif // HAIKENANIME_SEASONALCATALOGCONTROLLER_H
