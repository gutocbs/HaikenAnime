#include "SettingsController.h"

#include "../../application/cache/ClearLocalCacheUseCase.h"
#include "../../application/anilist/AniListAuthManager.h"
#include "../../application/configuration/UserPreferencesValidator.h"
#include "../../app/LocalLibraryScanCoordinator.h"

namespace {
QString authenticationStateName(const AniListAuthenticationState state) {
    switch (state) {
    case AniListAuthenticationState::Disconnected: return QStringLiteral("disconnected");
    case AniListAuthenticationState::Authorizing: return QStringLiteral("authorizing");
    case AniListAuthenticationState::AwaitingValidation: return QStringLiteral("validating");
    case AniListAuthenticationState::Authenticated: return QStringLiteral("authenticated");
    case AniListAuthenticationState::AuthenticationFailed: return QStringLiteral("failed");
    }
    return QStringLiteral("failed");
}
}

QString SettingsController::libraryRoot() const { return draft_.libraryRoot; }
QStringList SettingsController::availableScanExtensions() const {
    QStringList options = UserPreferences{}.scanExtensions;
    for (const auto &extension : persisted_.scanExtensions + draft_.scanExtensions)
        if (!options.contains(extension)) options.append(extension);
    return options;
}
QStringList SettingsController::selectedScanExtensions() const { return draft_.scanExtensions; }
bool SettingsController::scanRunning() const { return scanRunning_; }
bool SettingsController::synchronizationRunning() const { return synchronizationRunning_; }
qsizetype SettingsController::scanCandidateCount() const { return scanCandidateCount_; }
QString SettingsController::scanStatusMessage() const { return scanStatusMessage_; }
QString SettingsController::scanErrorMessage() const { return scanErrorMessage_; }
bool SettingsController::cacheCleanupRunning() const { return cacheCleanupRunning_; }
QString SettingsController::cacheCleanupStatusMessage() const { return cacheCleanupStatusMessage_; }
QString SettingsController::cacheCleanupErrorMessage() const { return cacheCleanupErrorMessage_; }
bool SettingsController::aniListConnectionAvailable() const {
    return aniListAuthManager_ && aniListOAuthConfiguration_.has_value()
        && aniListAuthorizationLauncher_ && aniListViewerClient_;
}
bool SettingsController::aniListAuthenticationInProgress() const {
    return aniListAuthenticationState_ == QStringLiteral("authorizing")
        || aniListAuthenticationState_ == QStringLiteral("validating");
}
QString SettingsController::aniListAuthenticationState() const { return aniListAuthenticationState_; }
QString SettingsController::aniListUsername() const { return aniListUsername_; }
QString SettingsController::aniListAuthenticationMessage() const { return aniListAuthenticationMessage_; }

void SettingsController::SetAniListAuthenticationServices(
    AniListAuthManager *authManager, AniListOAuthConfig configuration,
    AniListAuthorizationLauncher launcher, IAniListViewerClient *viewerClient,
    AniListOAuthReceiverStarter receiverStarter, AniListOAuthReceiverStopper receiverStopper) {
    aniListAuthManager_ = authManager;
    aniListOAuthConfiguration_ = std::move(configuration);
    aniListAuthorizationLauncher_ = std::move(launcher);
    aniListViewerClient_ = viewerClient;
    aniListOAuthReceiverStarter_ = std::move(receiverStarter);
    aniListOAuthReceiverStopper_ = std::move(receiverStopper);
    refreshAniListPresentation();
    emit changed();
}

void SettingsController::SetScanCoordinator(LocalLibraryScanCoordinator *coordinator) {
    if (scanCoordinator_ == coordinator) return;
    if (scanCoordinator_) disconnect(scanCoordinator_, nullptr, this, nullptr);
    scanCoordinator_ = coordinator;
    scanRunning_ = false;
    scanCandidateCount_ = 0;
    scanStatusMessage_ = tr("Nenhuma varredura iniciada.");
    scanErrorMessage_.clear();
    if (coordinator) {
        connect(coordinator, &LocalLibraryScanCoordinator::started, this, [this](const QString &root) {
            scanRunning_ = true;
            scanCandidateCount_ = 0;
            scanStatusMessage_ = tr("Escaneando %1").arg(root);
            scanErrorMessage_.clear();
            emit scanChanged();
        });
        connect(coordinator, &LocalLibraryScanCoordinator::progressChanged, this, [this](qsizetype count) {
            scanCandidateCount_ = count;
            emit scanChanged();
        });
        connect(coordinator, &LocalLibraryScanCoordinator::completed, this, [this](qsizetype count) {
            scanRunning_ = false;
            scanCandidateCount_ = count;
            scanStatusMessage_ = tr("Varredura concluída.");
            scanErrorMessage_.clear();
            emit scanChanged();
        });
        connect(coordinator, &LocalLibraryScanCoordinator::failed, this, [this](const QString &error) {
            scanRunning_ = false;
            scanStatusMessage_ = tr("A varredura não foi concluída.");
            scanErrorMessage_ = error;
            emit scanChanged();
        });
        connect(coordinator, &QObject::destroyed, this, [this] {
            scanRunning_ = false;
            scanStatusMessage_ = tr("Varredura indisponível.");
            scanErrorMessage_ = tr("O serviço de varredura não está disponível.");
            emit scanChanged();
        });
    }
    emit scanChanged();
}

void SettingsController::ApplyExternalHomeSortKey(QString key) {
    if (key.isEmpty() || (persisted_.homeSortKey == key && draft_.homeSortKey == key)) return;
    persisted_.homeSortKey = key;
    draft_.homeSortKey = std::move(key);
    emit changed();
}

void SettingsController::SetLibraryRoot(const QString &root) {
    draft_.libraryRoot = root;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetScanExtensionEnabled(const QString &extension, const bool enabled) {
    QString error;
    const auto normalized = NormalizeScanExtensions({extension}, error);
    extensionInputValid_ = !normalized.isEmpty();
    if (extensionInputValid_) {
        const auto &value = normalized.first();
        if (enabled && !draft_.scanExtensions.contains(value)) draft_.scanExtensions.append(value);
        if (!enabled) draft_.scanExtensions.removeAll(value);
    }
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::ScanNow() {
    if (dirty() || saving_ || !valid_) {
        scanErrorMessage_ = tr("Salve ou descarte as alterações antes de escanear.");
    } else if (scanRunning_) {
        scanErrorMessage_ = tr("Uma varredura já está em andamento.");
    } else if (!scanCoordinator_) {
        scanErrorMessage_ = tr("O serviço de varredura não está disponível.");
    } else {
        LocalLibraryScanRequest request;
        request.rootPath = persisted_.libraryRoot;
        request.allowedExtensions = persisted_.scanExtensions;
        if (scanCoordinator_->start(request)) {
            scanRunning_ = true;
            scanCandidateCount_ = 0;
            scanStatusMessage_ = tr("Escaneando %1").arg(request.rootPath);
            scanErrorMessage_.clear();
        } else {
            scanErrorMessage_ = tr("Não foi possível iniciar a varredura. O serviço pode estar ocupado ou encerrando.");
        }
    }
    emit scanChanged();
}

void SettingsController::SynchronizeNow() {
    if (synchronizationRunning_) return;
    emit synchronizationRequested();
}

void SettingsController::ConnectAniList() {
    if (!aniListConnectionAvailable() || aniListAuthenticationInProgress()) return;

    QString error;
    if (aniListOAuthReceiverStarter_
        && !aniListOAuthReceiverStarter_(error)) {
        aniListAuthenticationState_ = QStringLiteral("failed");
        aniListAuthenticationMessage_ = tr("Não foi possível preparar o retorno da autorização da AniList.");
        statusMessage_.clear();
        errorMessage_ = aniListAuthenticationMessage_;
        emit changed();
        return;
    }
    const QUrl authorizationUrl = aniListAuthManager_->beginAuthorization(*aniListOAuthConfiguration_);
    if (!aniListAuthorizationLauncher_(authorizationUrl, error)) {
        if (aniListOAuthReceiverStopper_) aniListOAuthReceiverStopper_();
        aniListAuthenticationState_ = QStringLiteral("failed");
        aniListAuthenticationMessage_ = tr("Não foi possível abrir a autorização da AniList.");
        statusMessage_.clear();
        errorMessage_ = aniListAuthenticationMessage_;
        emit changed();
        return;
    }
    refreshAniListPresentation();
    statusMessage_ = tr("Conectando conta AniList...");
    errorMessage_.clear();
    emit changed();
}

void SettingsController::HandleAniListOAuthCallback(const QUrl &callback) {
    if (aniListOAuthReceiverStopper_) aniListOAuthReceiverStopper_();
    if (!aniListConnectionAvailable()) return;

    QString error;
    if (!aniListAuthManager_->handleCallback(callback, error)) {
        aniListAuthenticationState_ = QStringLiteral("failed");
        aniListAuthenticationMessage_ = tr("Não foi possível concluir a autorização da AniList.");
        aniListUsername_.clear();
        statusMessage_.clear();
        errorMessage_ = aniListAuthenticationMessage_;
        emit changed();
        return;
    }
    if (!aniListAuthManager_->validateToken(*aniListViewerClient_, error)) {
        aniListAuthenticationState_ = QStringLiteral("failed");
        aniListAuthenticationMessage_ = tr("Não foi possível validar a conta AniList.");
        aniListUsername_.clear();
        statusMessage_.clear();
        errorMessage_ = aniListAuthenticationMessage_;
        emit changed();
        return;
    }
    refreshAniListPresentation();
    aniListAuthenticationMessage_ = tr("Conta AniList conectada.");
    statusMessage_ = aniListAuthenticationMessage_;
    errorMessage_.clear();
    emit changed();
}

void SettingsController::ClearLocalCache() {
    if (cacheCleanupRunning_) return;
    if (!cacheCleanup_) {
        cacheCleanupStatusMessage_.clear();
        cacheCleanupErrorMessage_ = tr("A limpeza do cache local não está disponível.");
        emit cacheCleanupFailed(cacheCleanupErrorMessage_);
        emit cacheCleanupChanged();
        return;
    }

    cacheCleanupRunning_ = true;
    cacheCleanupStatusMessage_ = tr("Limpando cache local...");
    cacheCleanupErrorMessage_.clear();
    emit cacheCleanupChanged();
    const bool started = cacheCleanup_->Start([this](ClearLocalCacheResult result) {
        cacheCleanupRunning_ = false;
        QStringList details;
        for (const auto &failure : result.failures)
            details.append(QStringLiteral("%1: %2").arg(failure.participant, failure.error));
        const QString failureDetails = details.join(QStringLiteral("; "));
        if (result.Succeeded()) {
            cacheCleanupStatusMessage_ = tr("Cache local limpo. %1 item(ns) removido(s).")
                .arg(result.TotalRemoved());
            cacheCleanupErrorMessage_.clear();
            emit cacheCleanupCompleted();
        } else if (result.TotalRemoved() > 0) {
            cacheCleanupStatusMessage_.clear();
            cacheCleanupErrorMessage_ = tr("A limpeza do cache local foi concluída parcialmente: %1")
                .arg(failureDetails);
            emit cacheCleanupPartialFailure(cacheCleanupErrorMessage_);
        } else {
            cacheCleanupStatusMessage_.clear();
            cacheCleanupErrorMessage_ = result.cancelled
                ? tr("A limpeza do cache local foi cancelada.")
                : tr("Não foi possível limpar o cache local: %1").arg(failureDetails);
            emit cacheCleanupFailed(cacheCleanupErrorMessage_);
        }
        emit cacheCleanupChanged();
    });
    if (!started && cacheCleanupRunning_) {
        cacheCleanupRunning_ = false;
        cacheCleanupStatusMessage_.clear();
        cacheCleanupErrorMessage_ = tr("A limpeza do cache local já está em andamento.");
        emit cacheCleanupFailed(cacheCleanupErrorMessage_);
        emit cacheCleanupChanged();
    }
}

void SettingsController::notifySynchronizationStarted() {
    synchronizationRunning_ = true;
    statusMessage_ = tr("Sincronizando dados em segundo plano...");
    errorMessage_.clear();
    emit synchronizationChanged();
    emit changed();
}

void SettingsController::notifySynchronizationCompleted() {
    synchronizationRunning_ = false;
    statusMessage_ = tr("Sincronização concluída.");
    emit synchronizationChanged();
    emit changed();
}

void SettingsController::notifySynchronizationFailed(const QString &error) {
    synchronizationRunning_ = false;
    errorMessage_ = error;
    statusMessage_.clear();
    emit synchronizationChanged();
    emit changed();
}

namespace {
QVariantMap option(const QString &key, const QString &label) {
    return {{QStringLiteral("key"), key}, {QStringLiteral("label"), label}};
}
QVariantMap intervalOption(const int value, const QString &label) {
    return {{QStringLiteral("value"), value}, {QStringLiteral("label"), label}};
}
QVariantMap scoreScaleOption(const double minimum, const double maximum, const double step,
                             const QString &label) {
    return {{QStringLiteral("label"), label},
            {QStringLiteral("minimum"), minimum},
            {QStringLiteral("maximum"), maximum},
            {QStringLiteral("step"), step}};
}
}

SettingsController::SettingsController(IUserPreferencesRepository *repository,
                                       UserPreferences initial,
                                       ClearLocalCacheUseCase *cacheCleanup,
                                       QObject *parent)
    : QObject(parent), repository_(repository), persisted_(initial), draft_(initial),
      coverQualityKey_(CoverQualityName(initial.coverQuality)),
      cardStatusPresentationKey_(CardStatusPresentationKey(initial.cardStatusPresentation)),
      languageKey_(NormalizeLanguageKey(initial.languageKey)),
      appliedLanguageKey_(NormalizeLanguageKey(initial.languageKey)),
      preferredTitleKey_(NormalizePreferredTitleKey(initial.preferredTitleKey)),
      cacheCleanup_(cacheCleanup) {
    qRegisterMetaType<UserPreferences>();
    scanStatusMessage_ = tr("Nenhuma varredura iniciada.");
    refreshValidation();
}

double SettingsController::scoreMinimum() const { return draft_.scoreMinimum; }
double SettingsController::scoreMaximum() const { return draft_.scoreMaximum; }
double SettingsController::scoreStep() const { return draft_.scoreStep; }
QString SettingsController::coverQualityKey() const { return coverQualityKey_; }
bool SettingsController::synchronizationEnabled() const { return draft_.synchronizationEnabled; }
int SettingsController::synchronizationIntervalMs() const { return draft_.synchronizationIntervalMs; }
QString SettingsController::cardStatusPresentationKey() const { return cardStatusPresentationKey_; }
QString SettingsController::languageKey() const { return languageKey_; }
QString SettingsController::preferredTitleKey() const { return preferredTitleKey_; }
bool SettingsController::includeAdultContent() const { return draft_.includeAdultContent; }
bool SettingsController::automaticLocalFileRecognition() const { return draft_.automaticLocalFileRecognition; }
QStringList SettingsController::enabledUserLists() const { return draft_.enabledUserLists; }
bool SettingsController::dirty() const { return !extensionInputValid_ || !(draft_ == persisted_); }
bool SettingsController::valid() const { return valid_; }
bool SettingsController::saving() const { return saving_; }
QString SettingsController::statusMessage() const { return statusMessage_; }
QString SettingsController::errorMessage() const { return errorMessage_; }

QVariantList SettingsController::coverQualityOptions() const {
    return {option(QStringLiteral("medium"), tr("Média")),
            option(QStringLiteral("large"), tr("Grande")),
            option(QStringLiteral("extraLarge"), tr("Extra grande"))};
}

QVariantList SettingsController::scoreScaleOptions() const {
    return {scoreScaleOption(0.0, 10.0, 1.0, tr("0–10 · intervalo 1")),
            scoreScaleOption(0.0, 100.0, 1.0, tr("0–100 · intervalo 1")),
            scoreScaleOption(0.0, 100.0, 5.0, tr("0–100 · intervalo 5"))};
}

QVariantList SettingsController::synchronizationIntervalOptions() const {
    return {intervalOption(900000, tr("15 minutos")), intervalOption(1800000, tr("30 minutos")),
            intervalOption(3600000, tr("1 hora")), intervalOption(10800000, tr("3 horas")),
            intervalOption(21600000, tr("6 horas")), intervalOption(43200000, tr("12 horas")),
            intervalOption(86400000, tr("24 horas"))};
}

QVariantList SettingsController::cardStatusPresentationOptions() const {
    return {option(QStringLiteral("personal-list-status"), tr("Status da minha lista")),
            option(QStringLiteral("media-release-status"), tr("Status de exibição"))};
}

QVariantList SettingsController::languageOptions() const {
    return {option(QStringLiteral("pt-BR"), tr("Português (Brasil)")),
            option(QStringLiteral("en"), tr("English"))};
}

QVariantList SettingsController::preferredTitleOptions() const {
    return {option(QStringLiteral("romaji"), tr("Romaji")),
            option(QStringLiteral("english"), tr("Inglês")),
            option(QStringLiteral("native"), tr("Nativo"))};
}

QString SettingsController::restartRequiredMessage() const {
    return languageKey_ == appliedLanguageKey_ ? QString{}
                                              : tr("Reinicie o aplicativo para aplicar o idioma selecionado.");
}

void SettingsController::SetScoreScale(const double minimum, const double maximum,
                                       const double step) {
    draft_.scoreMinimum = minimum;
    draft_.scoreMaximum = maximum;
    draft_.scoreStep = step;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetCoverQuality(const QString &key) {
    coverQualityKey_ = key;
    const auto quality = ParseCoverQuality(key);
    qualityKeyValid_ = quality.has_value();
    if (quality) draft_.coverQuality = quality.value();
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetSynchronizationEnabled(const bool enabled) {
    draft_.synchronizationEnabled = enabled;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetSynchronizationInterval(const int intervalMs) {
    draft_.synchronizationIntervalMs = intervalMs;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetCardStatusPresentation(const QString &key) {
    cardStatusPresentationKey_ = key;
    const auto presentation = ParseCardStatusPresentation(key);
    cardStatusPresentationKeyValid_ = presentation.has_value();
    if (presentation) draft_.cardStatusPresentation = presentation.value();
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetLanguage(const QString &key) {
    languageKey_ = key;
    languageKeyValid_ = IsSupportedLanguageKey(key);
    if (languageKeyValid_) draft_.languageKey = key;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetPreferredTitle(const QString &key) {
    preferredTitleKey_ = key;
    preferredTitleKeyValid_ = IsSupportedPreferredTitleKey(key);
    if (preferredTitleKeyValid_) draft_.preferredTitleKey = key;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetIncludeAdultContent(const bool enabled) {
    if (draft_.includeAdultContent == enabled) return;
    draft_.includeAdultContent = enabled;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}
void SettingsController::SetAutomaticLocalFileRecognition(const bool enabled) {
    draft_.automaticLocalFileRecognition = enabled;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::SetUserListEnabled(const QString &key, const bool enabled) {
    static const QStringList supported = {QStringLiteral("current"), QStringLiteral("planning"),
                                          QStringLiteral("on_hold"), QStringLiteral("dropped"),
                                          QStringLiteral("completed")};
    if (!supported.contains(key)) return;
    if (enabled && !draft_.enabledUserLists.contains(key)) draft_.enabledUserLists.append(key);
    if (!enabled) draft_.enabledUserLists.removeAll(key);
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::Save() {
    if (saving_) return;
    refreshValidation();
    if (!valid_) { if (errorMessage_.isEmpty()) errorMessage_ = tr("Revise os valores informados."); emit changed(); return; }
    if (!repository_) { errorMessage_ = tr("As configurações não estão disponíveis."); emit changed(); return; }
    saving_ = true; statusMessage_.clear(); errorMessage_.clear(); emit changed();
    QString error;
    if (!repository_->replace(draft_, error)) {
        saving_ = false; errorMessage_ = error; emit changed(); return;
    }
    persisted_ = draft_;
    saving_ = false;
    statusMessage_ = tr("Alterações salvas.");
    emit preferencesApplied(persisted_);
    emit changed();
}

void SettingsController::Discard() {
    draft_ = persisted_;
    coverQualityKey_ = CoverQualityName(draft_.coverQuality);
    qualityKeyValid_ = true;
    cardStatusPresentationKey_ = CardStatusPresentationKey(draft_.cardStatusPresentation);
    cardStatusPresentationKeyValid_ = true;
    languageKey_ = draft_.languageKey;
    languageKeyValid_ = true;
    preferredTitleKey_ = draft_.preferredTitleKey;
    preferredTitleKeyValid_ = true;
    extensionInputValid_ = true;
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::refreshValidation() {
    const auto result = ValidateUserPreferences(draft_);
    valid_ = qualityKeyValid_ && cardStatusPresentationKeyValid_ && languageKeyValid_
        && preferredTitleKeyValid_
        && extensionInputValid_ && result.valid;
    if (!qualityKeyValid_) errorMessage_ = tr("Qualidade de capa inválida.");
    else if (!cardStatusPresentationKeyValid_) errorMessage_ = tr("Apresentação de status inválida.");
    else if (!languageKeyValid_) errorMessage_ = tr("Idioma inválido.");
    else if (!preferredTitleKeyValid_) errorMessage_ = tr("Título preferido inválido.");
    else if (!extensionInputValid_) errorMessage_ = tr("Extensão de arquivo inválida.");
    else if (!result.valid) errorMessage_ = result.error;
}

void SettingsController::refreshAniListPresentation() {
    if (!aniListConnectionAvailable()) {
        aniListAuthenticationState_ = QStringLiteral("unavailable");
        aniListUsername_.clear();
        aniListAuthenticationMessage_.clear();
        return;
    }
    aniListAuthenticationState_ = authenticationStateName(aniListAuthManager_->state());
    aniListUsername_ = aniListAuthManager_->credentials().username;
    aniListAuthenticationMessage_.clear();
}
