#include <QtTest>

#include "../../src/application/recognition/IPluginSecretStore.h"
#include "../../src/application/recognition/IPluginSettingsRepository.h"
#include "../../src/application/recognition/PluginSettingsService.h"

#include <map>
#include <utility>

namespace {
const auto PlexToken = QStringLiteral("plex-token-value-123");

class FakePluginSettingsRepository final : public IPluginSettingsRepository {
public:
    bool read(const QString &pluginId, QVariantMap &values, QString &error) override {
        error.clear();
        values = stored.value(pluginId);
        return true;
    }

    bool replace(const QString &pluginId, const QVariantMap &values, QString &error) override {
        if (failWrites) {
            error = QStringLiteral("disk full");
            return false;
        }
        error.clear();
        stored.insert(pluginId, values);
        return true;
    }

    QHash<QString, QVariantMap> stored;
    bool failWrites = false;
};

class FakePluginSecretStore final : public IPluginSecretStore {
public:
    bool hasSecret(const QString &pluginId, const QString &key) const override {
        return secrets.contains({pluginId, key});
    }

    bool readSecret(const QString &pluginId, const QString &key, QString &value, QString &error) override {
        error.clear();
        const auto found = secrets.find({pluginId, key});
        if (found == secrets.end()) {
            error = QStringLiteral("missing");
            return false;
        }
        value = found->second;
        return true;
    }

    bool writeSecret(const QString &pluginId, const QString &key, const QString &value, QString &error) override {
        if (failWrites) {
            error = QStringLiteral("credential manager rejected %1").arg(value);
            return false;
        }
        error.clear();
        secrets[{pluginId, key}] = value;
        return true;
    }

    bool clearSecret(const QString &pluginId, const QString &key, QString &error) override {
        error.clear();
        secrets.erase({pluginId, key});
        return true;
    }

    std::map<std::pair<QString, QString>, QString> secrets;
    bool failWrites = false;
};

PluginSettingsSchema plexSchema() {
    PluginSettingsSchema schema;
    schema.pluginId = QStringLiteral("plex");
    schema.displayName = QStringLiteral("Plex");
    schema.fields = {
        {.key = QStringLiteral("serverUrl"), .type = PluginSettingType::Text, .label = QStringLiteral("Server"),
         .required = true},
        {.key = QStringLiteral("pollSeconds"), .type = PluginSettingType::Number, .label = QStringLiteral("Poll"),
         .defaultValue = 15, .minimum = 5, .maximum = 300},
        {.key = QStringLiteral("enabled"), .type = PluginSettingType::Boolean, .label = QStringLiteral("Enabled"),
         .defaultValue = true},
        {.key = QStringLiteral("quality"), .type = PluginSettingType::Selection, .label = QStringLiteral("Quality"),
         .defaultValue = QStringLiteral("auto"),
         .options = {{QStringLiteral("auto"), QStringLiteral("Auto")}, {QStringLiteral("high"), QStringLiteral("High")}}},
        {.key = QStringLiteral("token"), .type = PluginSettingType::Secret, .label = QStringLiteral("Token"),
         .required = true},
    };
    return schema;
}
}

class PluginSettingsServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void exposesRegisteredSchemas();
    void rejectsDuplicateSchemaRegistration();
    void valuesFallBackToSchemaDefaults();
    void valuesPreferStoredValuesOverDefaults();
    void setValueRejectsUnknownPlugin();
    void setValueRejectsUnknownKey();
    void setValueRejectsWrongType();
    void setValueRejectsNumberOutsideBounds();
    void setValueRejectsSelectionOutsideOptions();
    void saveRejectsMissingRequiredField();
    void savePersistsNonSecretValuesWithoutSecrets();
    void saveWritesSecretsToSecretStore();
    void valuesMaskStoredSecret();
    void valuesNeverExposeUnsavedSecret();
    void storedSecretSatisfiesRequiredField();
    void clearSecretRemovesStoredSecret();
    void saveFailureDoesNotLeakSecretValue();
    void failedSaveKeepsPreviouslyStoredValues();
    void discardDropsDraftsAndPendingSecrets();
    void reportsUnsavedChanges();

private:
    std::unique_ptr<FakePluginSettingsRepository> repository;
    std::unique_ptr<FakePluginSecretStore> secrets;
    std::unique_ptr<PluginSettingsService> service;

    void registerPlex() {
        QString error;
        QVERIFY2(service->registerSchema(plexSchema(), error), qPrintable(error));
    }
};

void PluginSettingsServiceTests::init() {
    repository = std::make_unique<FakePluginSettingsRepository>();
    secrets = std::make_unique<FakePluginSecretStore>();
    service = std::make_unique<PluginSettingsService>(*repository, *secrets);
}

void PluginSettingsServiceTests::exposesRegisteredSchemas() {
    registerPlex();

    const auto schemas = service->schemas();

    QCOMPARE(schemas.size(), 1);
    QCOMPARE(schemas.first().pluginId, QStringLiteral("plex"));
    QCOMPARE(schemas.first().fields.size(), 5);
}

void PluginSettingsServiceTests::rejectsDuplicateSchemaRegistration() {
    registerPlex();
    QString error;

    QVERIFY(!service->registerSchema(plexSchema(), error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(service->schemas().size(), 1);
}

void PluginSettingsServiceTests::valuesFallBackToSchemaDefaults() {
    registerPlex();

    const auto values = service->values(QStringLiteral("plex"));

    QCOMPARE(values.value(QStringLiteral("pollSeconds")).toInt(), 15);
    QCOMPARE(values.value(QStringLiteral("enabled")).toBool(), true);
    QCOMPARE(values.value(QStringLiteral("quality")).toString(), QStringLiteral("auto"));
    QCOMPARE(values.value(QStringLiteral("serverUrl")).toString(), QString());
}

void PluginSettingsServiceTests::valuesPreferStoredValuesOverDefaults() {
    repository->stored.insert(QStringLiteral("plex"), {{QStringLiteral("pollSeconds"), 30},
                                                       {QStringLiteral("serverUrl"), QStringLiteral("http://nas:32400")}});
    registerPlex();

    const auto values = service->values(QStringLiteral("plex"));

    QCOMPARE(values.value(QStringLiteral("pollSeconds")).toInt(), 30);
    QCOMPARE(values.value(QStringLiteral("serverUrl")).toString(), QStringLiteral("http://nas:32400"));
}

void PluginSettingsServiceTests::setValueRejectsUnknownPlugin() {
    registerPlex();
    QString error;

    QVERIFY(!service->setValue(QStringLiteral("crunchyroll"), QStringLiteral("serverUrl"), QStringLiteral("x"), error));
    QVERIFY(!error.isEmpty());
}

void PluginSettingsServiceTests::setValueRejectsUnknownKey() {
    registerPlex();
    QString error;

    QVERIFY(!service->setValue(QStringLiteral("plex"), QStringLiteral("unknown"), QStringLiteral("x"), error));
    QVERIFY(!error.isEmpty());
}

void PluginSettingsServiceTests::setValueRejectsWrongType() {
    registerPlex();
    QString error;

    QVERIFY(!service->setValue(QStringLiteral("plex"), QStringLiteral("pollSeconds"), QStringLiteral("abc"), error));
    QVERIFY(!service->setValue(QStringLiteral("plex"), QStringLiteral("enabled"), QStringLiteral("yes"), error));
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("pollSeconds"), 60, error));
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("enabled"), false, error));
    QCOMPARE(service->values(QStringLiteral("plex")).value(QStringLiteral("pollSeconds")).toInt(), 60);
}

void PluginSettingsServiceTests::setValueRejectsNumberOutsideBounds() {
    registerPlex();
    QString error;

    QVERIFY(!service->setValue(QStringLiteral("plex"), QStringLiteral("pollSeconds"), 4, error));
    QVERIFY(!service->setValue(QStringLiteral("plex"), QStringLiteral("pollSeconds"), 301, error));
    QCOMPARE(service->values(QStringLiteral("plex")).value(QStringLiteral("pollSeconds")).toInt(), 15);
}

void PluginSettingsServiceTests::setValueRejectsSelectionOutsideOptions() {
    registerPlex();
    QString error;

    QVERIFY(!service->setValue(QStringLiteral("plex"), QStringLiteral("quality"), QStringLiteral("ultra"), error));
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("quality"), QStringLiteral("high"), error));
}

void PluginSettingsServiceTests::saveRejectsMissingRequiredField() {
    registerPlex();
    QString error;
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("token"), PlexToken, error));

    QVERIFY(!service->save(QStringLiteral("plex"), error));
    QVERIFY(error.contains(QStringLiteral("serverUrl")));
    QVERIFY(!repository->stored.contains(QStringLiteral("plex")));
}

void PluginSettingsServiceTests::savePersistsNonSecretValuesWithoutSecrets() {
    registerPlex();
    QString error;
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("serverUrl"), QStringLiteral("http://nas:32400"), error));
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("token"), PlexToken, error));

    QVERIFY2(service->save(QStringLiteral("plex"), error), qPrintable(error));

    const auto stored = repository->stored.value(QStringLiteral("plex"));
    QCOMPARE(stored.value(QStringLiteral("serverUrl")).toString(), QStringLiteral("http://nas:32400"));
    QVERIFY(!stored.contains(QStringLiteral("token")));
    for (const auto &value : stored) QVERIFY(value.toString() != PlexToken);
}

void PluginSettingsServiceTests::saveWritesSecretsToSecretStore() {
    registerPlex();
    QString error;
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("serverUrl"), QStringLiteral("http://nas:32400"), error));
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("token"), PlexToken, error));

    QVERIFY2(service->save(QStringLiteral("plex"), error), qPrintable(error));

    QString stored;
    QVERIFY(secrets->readSecret(QStringLiteral("plex"), QStringLiteral("token"), stored, error));
    QCOMPARE(stored, PlexToken);
}

void PluginSettingsServiceTests::valuesMaskStoredSecret() {
    secrets->secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = PlexToken;
    registerPlex();

    const auto values = service->values(QStringLiteral("plex"));

    QCOMPARE(values.value(QStringLiteral("token")).toString(), PluginSettingsService::secretMask());
    QVERIFY(service->hasSecret(QStringLiteral("plex"), QStringLiteral("token")));
}

void PluginSettingsServiceTests::valuesNeverExposeUnsavedSecret() {
    registerPlex();
    QString error;
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("token"), PlexToken, error));

    const auto values = service->values(QStringLiteral("plex"));

    QCOMPARE(values.value(QStringLiteral("token")).toString(), PluginSettingsService::secretMask());
    for (const auto &value : values) QVERIFY(value.toString() != PlexToken);
}

void PluginSettingsServiceTests::storedSecretSatisfiesRequiredField() {
    secrets->secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = PlexToken;
    registerPlex();
    QString error;
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("serverUrl"), QStringLiteral("http://nas:32400"), error));

    QVERIFY2(service->save(QStringLiteral("plex"), error), qPrintable(error));
}

void PluginSettingsServiceTests::clearSecretRemovesStoredSecret() {
    secrets->secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = PlexToken;
    registerPlex();
    QString error;

    QVERIFY2(service->clearSecret(QStringLiteral("plex"), QStringLiteral("token"), error), qPrintable(error));

    QVERIFY(!service->hasSecret(QStringLiteral("plex"), QStringLiteral("token")));
    QCOMPARE(service->values(QStringLiteral("plex")).value(QStringLiteral("token")).toString(), QString());
    QVERIFY(!service->clearSecret(QStringLiteral("plex"), QStringLiteral("serverUrl"), error));
}

void PluginSettingsServiceTests::saveFailureDoesNotLeakSecretValue() {
    registerPlex();
    secrets->failWrites = true;
    QString error;
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("serverUrl"), QStringLiteral("http://nas:32400"), error));
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("token"), PlexToken, error));

    QVERIFY(!service->save(QStringLiteral("plex"), error));

    QVERIFY(!error.isEmpty());
    QVERIFY(!error.contains(PlexToken));
}

void PluginSettingsServiceTests::failedSaveKeepsPreviouslyStoredValues() {
    repository->stored.insert(QStringLiteral("plex"), {{QStringLiteral("serverUrl"), QStringLiteral("http://old:32400")}});
    secrets->secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = PlexToken;
    registerPlex();
    repository->failWrites = true;
    QString error;
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("serverUrl"), QStringLiteral("http://new:32400"), error));

    QVERIFY(!service->save(QStringLiteral("plex"), error));

    QCOMPARE(repository->stored.value(QStringLiteral("plex")).value(QStringLiteral("serverUrl")).toString(),
             QStringLiteral("http://old:32400"));
}

void PluginSettingsServiceTests::discardDropsDraftsAndPendingSecrets() {
    repository->stored.insert(QStringLiteral("plex"), {{QStringLiteral("pollSeconds"), 30}});
    registerPlex();
    QString error;
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("pollSeconds"), 60, error));
    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("token"), PlexToken, error));

    service->discard(QStringLiteral("plex"));

    QCOMPARE(service->values(QStringLiteral("plex")).value(QStringLiteral("pollSeconds")).toInt(), 30);
    QVERIFY(!service->hasSecret(QStringLiteral("plex"), QStringLiteral("token")));
    QVERIFY(!service->hasUnsavedChanges(QStringLiteral("plex")));
}

void PluginSettingsServiceTests::reportsUnsavedChanges() {
    registerPlex();
    QString error;
    QVERIFY(!service->hasUnsavedChanges(QStringLiteral("plex")));

    QVERIFY(service->setValue(QStringLiteral("plex"), QStringLiteral("serverUrl"), QStringLiteral("http://nas"), error));

    QVERIFY(service->hasUnsavedChanges(QStringLiteral("plex")));
}

QTEST_MAIN(PluginSettingsServiceTests)
#include "PluginSettingsServiceTests.moc"
