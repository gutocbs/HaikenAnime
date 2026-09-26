#ifndef HAIKENANIME_SETTINGSCONTROLLER_H
#define HAIKENANIME_SETTINGSCONTROLLER_H

#include <QObject>
#include <QVariantList>

#include "../../application/configuration/IUserPreferencesRepository.h"

class SettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(double scoreMinimum READ scoreMinimum NOTIFY changed)
    Q_PROPERTY(double scoreMaximum READ scoreMaximum NOTIFY changed)
    Q_PROPERTY(double scoreStep READ scoreStep NOTIFY changed)
    Q_PROPERTY(QString coverQualityKey READ coverQualityKey NOTIFY changed)
    Q_PROPERTY(bool synchronizationEnabled READ synchronizationEnabled NOTIFY changed)
    Q_PROPERTY(int synchronizationIntervalMs READ synchronizationIntervalMs NOTIFY changed)
    Q_PROPERTY(QVariantList coverQualityOptions READ coverQualityOptions CONSTANT)
    Q_PROPERTY(QVariantList scoreScaleOptions READ scoreScaleOptions CONSTANT)
    Q_PROPERTY(QVariantList synchronizationIntervalOptions READ synchronizationIntervalOptions CONSTANT)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(bool valid READ valid NOTIFY changed)
    Q_PROPERTY(bool saving READ saving NOTIFY changed)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY changed)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY changed)
public:
    explicit SettingsController(IUserPreferencesRepository *repository,
                                UserPreferences initial, QObject *parent = nullptr);
    double scoreMinimum() const;
    double scoreMaximum() const;
    double scoreStep() const;
    QString coverQualityKey() const;
    bool synchronizationEnabled() const;
    int synchronizationIntervalMs() const;
    QVariantList coverQualityOptions() const;
    QVariantList scoreScaleOptions() const;
    QVariantList synchronizationIntervalOptions() const;
    bool dirty() const;
    bool valid() const;
    bool saving() const;
    QString statusMessage() const;
    QString errorMessage() const;

    Q_INVOKABLE void SetScoreScale(double minimum, double maximum, double step);
    Q_INVOKABLE void SetCoverQuality(const QString &key);
    Q_INVOKABLE void SetSynchronizationEnabled(bool enabled);
    Q_INVOKABLE void SetSynchronizationInterval(int intervalMs);
    Q_INVOKABLE void Save();
    Q_INVOKABLE void Discard();
signals:
    void changed();
    void preferencesApplied(UserPreferences preferences);
private:
    void refreshValidation();
    IUserPreferencesRepository *repository_ = nullptr;
    UserPreferences persisted_;
    UserPreferences draft_;
    QString coverQualityKey_;
    bool qualityKeyValid_ = true;
    bool valid_ = true;
    bool saving_ = false;
    QString statusMessage_;
    QString errorMessage_;
};

#endif
