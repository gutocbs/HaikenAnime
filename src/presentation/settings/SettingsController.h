#ifndef HAIKENANIME_SETTINGSCONTROLLER_H
#define HAIKENANIME_SETTINGSCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QPointer>

#include <functional>
#include <optional>

#include "../../application/configuration/IUserPreferencesRepository.h"
#include "../../application/anilist/AniListOAuthConfig.h"

class LocalLibraryScanCoordinator;
class ClearLocalCacheUseCase;
class AniListAuthManager;
class IAniListViewerClient;

class SettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(double scoreMinimum READ scoreMinimum NOTIFY changed)
    Q_PROPERTY(double scoreMaximum READ scoreMaximum NOTIFY changed)
    Q_PROPERTY(double scoreStep READ scoreStep NOTIFY changed)
    Q_PROPERTY(QString coverQualityKey READ coverQualityKey NOTIFY changed)
    Q_PROPERTY(bool synchronizationEnabled READ synchronizationEnabled NOTIFY changed)
    Q_PROPERTY(int synchronizationIntervalMs READ synchronizationIntervalMs NOTIFY changed)
    Q_PROPERTY(QString cardStatusPresentationKey READ cardStatusPresentationKey NOTIFY changed)
    Q_PROPERTY(QString languageKey READ languageKey NOTIFY changed)
    Q_PROPERTY(QString preferredTitleKey READ preferredTitleKey NOTIFY changed)
    Q_PROPERTY(bool includeAdultContent READ includeAdultContent NOTIFY changed)
    Q_PROPERTY(bool automaticLocalFileRecognition READ automaticLocalFileRecognition NOTIFY changed)
    Q_PROPERTY(QStringList enabledUserLists READ enabledUserLists NOTIFY changed)
    Q_PROPERTY(QVariantList coverQualityOptions READ coverQualityOptions CONSTANT)
    Q_PROPERTY(QVariantList scoreScaleOptions READ scoreScaleOptions CONSTANT)
    Q_PROPERTY(QVariantList synchronizationIntervalOptions READ synchronizationIntervalOptions CONSTANT)
    Q_PROPERTY(QVariantList cardStatusPresentationOptions READ cardStatusPresentationOptions CONSTANT)
    Q_PROPERTY(QVariantList languageOptions READ languageOptions CONSTANT)
    Q_PROPERTY(QVariantList preferredTitleOptions READ preferredTitleOptions CONSTANT)
    Q_PROPERTY(QString restartRequiredMessage READ restartRequiredMessage NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(bool valid READ valid NOTIFY changed)
    Q_PROPERTY(bool saving READ saving NOTIFY changed)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY changed)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY changed)
    Q_PROPERTY(QString libraryRoot READ libraryRoot NOTIFY changed)
    Q_PROPERTY(QStringList availableScanExtensions READ availableScanExtensions NOTIFY changed)
    Q_PROPERTY(QStringList selectedScanExtensions READ selectedScanExtensions NOTIFY changed)
    Q_PROPERTY(bool scanRunning READ scanRunning NOTIFY scanChanged)
    Q_PROPERTY(bool synchronizationRunning READ synchronizationRunning NOTIFY synchronizationChanged)
    Q_PROPERTY(qsizetype scanCandidateCount READ scanCandidateCount NOTIFY scanChanged)
    Q_PROPERTY(QString scanStatusMessage READ scanStatusMessage NOTIFY scanChanged)
    Q_PROPERTY(QString scanErrorMessage READ scanErrorMessage NOTIFY scanChanged)
    Q_PROPERTY(bool cacheCleanupRunning READ cacheCleanupRunning NOTIFY cacheCleanupChanged)
    Q_PROPERTY(QString cacheCleanupStatusMessage READ cacheCleanupStatusMessage NOTIFY cacheCleanupChanged)
    Q_PROPERTY(QString cacheCleanupErrorMessage READ cacheCleanupErrorMessage NOTIFY cacheCleanupChanged)
    Q_PROPERTY(bool aniListConnectionAvailable READ aniListConnectionAvailable NOTIFY changed)
    Q_PROPERTY(bool aniListAuthenticationInProgress READ aniListAuthenticationInProgress NOTIFY changed)
    Q_PROPERTY(QString aniListAuthenticationState READ aniListAuthenticationState NOTIFY changed)
    Q_PROPERTY(QString aniListUsername READ aniListUsername NOTIFY changed)
    Q_PROPERTY(QString aniListAuthenticationMessage READ aniListAuthenticationMessage NOTIFY changed)
public:
    using AniListAuthorizationLauncher = std::function<bool(const QUrl &, QString &)>;
    using AniListOAuthReceiverStarter = std::function<bool(QString &)>;
    using AniListOAuthReceiverStopper = std::function<void()>;

    explicit SettingsController(IUserPreferencesRepository *repository,
                                UserPreferences initial,
                                ClearLocalCacheUseCase *cacheCleanup = nullptr,
                                QObject *parent = nullptr);
    double scoreMinimum() const;
    double scoreMaximum() const;
    double scoreStep() const;
    QString coverQualityKey() const;
    bool synchronizationEnabled() const;
    int synchronizationIntervalMs() const;
    QString cardStatusPresentationKey() const;
    QString languageKey() const;
    QString preferredTitleKey() const;
    bool includeAdultContent() const;
    bool automaticLocalFileRecognition() const;
    QStringList enabledUserLists() const;
    QVariantList coverQualityOptions() const;
    QVariantList scoreScaleOptions() const;
    QVariantList synchronizationIntervalOptions() const;
    QVariantList cardStatusPresentationOptions() const;
    QVariantList languageOptions() const;
    QVariantList preferredTitleOptions() const;
    QString restartRequiredMessage() const;
    bool dirty() const;
    bool valid() const;
    bool saving() const;
    QString statusMessage() const;
    QString errorMessage() const;
    QString libraryRoot() const;
    QStringList availableScanExtensions() const;
    QStringList selectedScanExtensions() const;
    bool scanRunning() const;
    bool synchronizationRunning() const;
    qsizetype scanCandidateCount() const;
    QString scanStatusMessage() const;
    QString scanErrorMessage() const;
    bool cacheCleanupRunning() const;
    QString cacheCleanupStatusMessage() const;
    QString cacheCleanupErrorMessage() const;
    bool aniListConnectionAvailable() const;
    bool aniListAuthenticationInProgress() const;
    QString aniListAuthenticationState() const;
    QString aniListUsername() const;
    QString aniListAuthenticationMessage() const;
    void SetScanCoordinator(LocalLibraryScanCoordinator *coordinator);
    void SetAniListAuthenticationServices(AniListAuthManager *authManager,
                                           AniListOAuthConfig configuration,
                                           AniListAuthorizationLauncher launcher,
                                           IAniListViewerClient *viewerClient,
                                           AniListOAuthReceiverStarter receiverStarter = {},
                                           AniListOAuthReceiverStopper receiverStopper = {});
    void ApplyExternalHomeSortKey(QString key);

    Q_INVOKABLE void SetScoreScale(double minimum, double maximum, double step);
    Q_INVOKABLE void SetCoverQuality(const QString &key);
    Q_INVOKABLE void SetSynchronizationEnabled(bool enabled);
    Q_INVOKABLE void SetSynchronizationInterval(int intervalMs);
    Q_INVOKABLE void SetCardStatusPresentation(const QString &key);
    Q_INVOKABLE void SetLanguage(const QString &key);
    Q_INVOKABLE void SetPreferredTitle(const QString &key);
    Q_INVOKABLE void SetIncludeAdultContent(bool enabled);
    Q_INVOKABLE void SetAutomaticLocalFileRecognition(bool enabled);
    Q_INVOKABLE void SetUserListEnabled(const QString &key, bool enabled);
    Q_INVOKABLE void Save();
    Q_INVOKABLE void Discard();
    Q_INVOKABLE void SetLibraryRoot(const QString &root);
    Q_INVOKABLE void SetScanExtensionEnabled(const QString &extension, bool enabled);
    Q_INVOKABLE void ScanNow();
    Q_INVOKABLE void SynchronizeNow();
    Q_INVOKABLE void ClearLocalCache();
    Q_INVOKABLE void ConnectAniList();
    Q_INVOKABLE void HandleAniListOAuthCallback(const QUrl &callback);
    void notifySynchronizationStarted();
    void notifySynchronizationCompleted();
    void notifySynchronizationFailed(const QString &error);
signals:
    void changed();
    void scanChanged();
    void cacheCleanupChanged();
    void cacheCleanupCompleted();
    void cacheCleanupPartialFailure(QString error);
    void cacheCleanupFailed(QString error);
    void synchronizationChanged();
    void synchronizationRequested();
    void preferencesApplied(UserPreferences preferences);
private:
    void refreshValidation();
    void refreshAniListPresentation();
    IUserPreferencesRepository *repository_ = nullptr;
    UserPreferences persisted_;
    UserPreferences draft_;
    QString coverQualityKey_;
    bool qualityKeyValid_ = true;
    QString cardStatusPresentationKey_;
    bool cardStatusPresentationKeyValid_ = true;
    QString languageKey_;
    QString appliedLanguageKey_;
    bool languageKeyValid_ = true;
    QString preferredTitleKey_;
    bool preferredTitleKeyValid_ = true;
    bool valid_ = true;
    bool saving_ = false;
    QString statusMessage_;
    QString errorMessage_;
    QPointer<LocalLibraryScanCoordinator> scanCoordinator_;
    bool extensionInputValid_ = true;
    bool scanRunning_ = false;
    bool synchronizationRunning_ = false;
    qsizetype scanCandidateCount_ = 0;
    QString scanStatusMessage_;
    QString scanErrorMessage_;
    ClearLocalCacheUseCase *cacheCleanup_ = nullptr;
    bool cacheCleanupRunning_ = false;
    QString cacheCleanupStatusMessage_;
    QString cacheCleanupErrorMessage_;
    AniListAuthManager *aniListAuthManager_ = nullptr;
    std::optional<AniListOAuthConfig> aniListOAuthConfiguration_;
    AniListAuthorizationLauncher aniListAuthorizationLauncher_;
    IAniListViewerClient *aniListViewerClient_ = nullptr;
    AniListOAuthReceiverStarter aniListOAuthReceiverStarter_;
    AniListOAuthReceiverStopper aniListOAuthReceiverStopper_;
    QString aniListAuthenticationState_ = QStringLiteral("unavailable");
    QString aniListUsername_;
    QString aniListAuthenticationMessage_;
};

#endif
