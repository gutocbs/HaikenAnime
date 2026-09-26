#include "SettingsController.h"

#include "../../application/configuration/UserPreferencesValidator.h"

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
                                       UserPreferences initial, QObject *parent)
    : QObject(parent), repository_(repository), persisted_(initial), draft_(initial),
      coverQualityKey_(CoverQualityName(initial.coverQuality)) {
    qRegisterMetaType<UserPreferences>();
    refreshValidation();
}

double SettingsController::scoreMinimum() const { return draft_.scoreMinimum; }
double SettingsController::scoreMaximum() const { return draft_.scoreMaximum; }
double SettingsController::scoreStep() const { return draft_.scoreStep; }
QString SettingsController::coverQualityKey() const { return coverQualityKey_; }
bool SettingsController::synchronizationEnabled() const { return draft_.synchronizationEnabled; }
int SettingsController::synchronizationIntervalMs() const { return draft_.synchronizationIntervalMs; }
bool SettingsController::dirty() const { return !(draft_ == persisted_); }
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
    statusMessage_.clear(); errorMessage_.clear(); refreshValidation(); emit changed();
}

void SettingsController::refreshValidation() {
    const auto result = ValidateUserPreferences(draft_);
    valid_ = qualityKeyValid_ && result.valid;
    if (!valid_) errorMessage_ = qualityKeyValid_ ? result.error : tr("Qualidade de capa inválida.");
}
