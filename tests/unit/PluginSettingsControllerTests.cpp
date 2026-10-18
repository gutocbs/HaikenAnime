#include <QSignalSpy>
#include <QtTest>

#include "../../src/presentation/settings/PluginSettingsController.h"
#include "PlexTestSupport.h"

namespace {
const auto Token = QStringLiteral("plex-account-token-SECRET");
const auto ServerUri = QStringLiteral("https://10-0-0-20.server-abc.plex.direct:32400");

class MemorySettings final : public IPluginSettingsRepository {
public:
    bool read(const QString &pluginId, QVariantMap &values, QString &error) override {
        error.clear();
        values = stored.value(pluginId);
        return true;
    }
    bool replace(const QString &pluginId, const QVariantMap &values, QString &error) override {
        error.clear();
        stored.insert(pluginId, values);
        return true;
    }
    QHash<QString, QVariantMap> stored;
};

class PlayerPlugin final : public IPlaybackPlugin {
public:
    [[nodiscard]] QString id() const override { return QStringLiteral("mpv"); }
    [[nodiscard]] QString displayName() const override { return QStringLiteral("mpv"); }
    [[nodiscard]] PluginCapabilities capabilities() const override {
        return PluginCapability::DynamicSettings | PluginCapability::ConnectionTest;
    }
    [[nodiscard]] PluginSettingsSchema settingsSchema() const override {
        return {.pluginId = id(),
                .displayName = displayName(),
                .fields = {{.key = QStringLiteral("enabled"), .type = PluginSettingType::Boolean,
                            .label = QStringLiteral("Enabled"), .defaultValue = false},
                           {.key = QStringLiteral("executablePath"), .type = PluginSettingType::Text,
                            .label = QStringLiteral("Executable"), .required = true, .defaultValue = QStringLiteral("mpv")},
                           {.key = QStringLiteral("quality"), .type = PluginSettingType::Selection,
                            .label = QStringLiteral("Quality"), .defaultValue = QStringLiteral("auto"),
                            .options = {{QStringLiteral("auto"), QStringLiteral("Auto")}}},
                           {.key = QStringLiteral("seconds"), .type = PluginSettingType::Number,
                            .label = QStringLiteral("Seconds"), .defaultValue = 180, .minimum = 30, .maximum = 3600}}};
    }
    void observe(IPlaybackObserver &) override {}
    void testConnection(std::function<void(const ConnectionTestResult &)> completion) override {
        completion({.succeeded = false, .safeMessage = QStringLiteral("mpv executable was not found.")});
    }
    void stop() override {}
};

class NullObserver final : public IPlaybackObserver {
public:
    void onSessionStarted(const PlaybackSession &, const EpisodeCandidate &) override {}
    void onSessionUpdated(const PlaybackSessionId &, const PlaybackSnapshot &) override {}
    void onSessionStopped(const PlaybackSessionId &, StopReason) override {}
};

PlexHttpResponse json(const QByteArray &body, const int status = 200) {
    return {.status = status, .body = body};
}
}

class PluginSettingsControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void listsPluginsWithCapabilities();
    void selectedPluginExposesTypedFields();
    void secretFieldIsMaskedAndNeverEchoed();
    void invalidValueIsRejectedWithMessage();
    void saveReportsMissingRequiredFields();
    void saveAppliesSettingsAndClearsDirty();
    void discardRestoresSavedValues();
    void connectRunsBrowserLoginAndReportsConnected();
    void disconnectClearsLogin();
    void testConnectionShowsPluginResult();
    void selectingServerStoresItsId();
    void discardAfterSelectingServerKeepsTheSavedServer();
    void selectingUsersUpdatesObservedUsers();
    void messagesNeverContainTheToken();
    void reconnectingResumesPlexPolling();

private:
    std::unique_ptr<PluginSettingsController> makeController();

    MemorySettings repository;
    FakeSecretStore secrets;
    FakePlexHttp http;
    std::unique_ptr<PluginSettingsService> settings;
    std::unique_ptr<PluginRegistry> registry;
    std::unique_ptr<PlexApiClient> api;
    std::unique_ptr<PlexAuthentication> authentication;
    PlexPlugin *plex = nullptr;
    int applied = 0;
};

void PluginSettingsControllerTests::init() {
    repository = MemorySettings();
    secrets = FakeSecretStore();
    http = FakePlexHttp();
    applied = 0;
    settings = std::make_unique<PluginSettingsService>(repository, secrets);
    registry = std::make_unique<PluginRegistry>();
    api = std::make_unique<PlexApiClient>(http, PlexClientIdentity{.clientIdentifier = QStringLiteral("client-1")});
    authentication = std::make_unique<PlexAuthentication>(PlexAuthenticationDependencies{
        .http = &http, .secrets = &secrets, .identity = {.clientIdentifier = QStringLiteral("client-1")},
        .openUrl = [](const QUrl &) { return true; },
        .schedule = [](int, std::function<void()> callback) { callback(); }});
    auto plexPlugin = std::make_unique<PlexPlugin>(PlexPluginDependencies{.api = api.get(),
                                                                           .authentication = authentication.get()});
    plex = plexPlugin.get();
    auto player = std::make_unique<PlayerPlugin>();
    QString error;
    QVERIFY(settings->registerSchema(player->settingsSchema(), error));
    QVERIFY(settings->registerSchema(plex->settingsSchema(), error));
    QVERIFY(registry->registerPlugin(std::move(player)));
    QVERIFY(registry->registerPlugin(std::move(plexPlugin)));
}

std::unique_ptr<PluginSettingsController> PluginSettingsControllerTests::makeController() {
    return std::make_unique<PluginSettingsController>(
        *settings, *registry, PlexAccountServices{.authentication = authentication.get(), .api = api.get(), .plugin = plex},
        [this] { ++applied; });
}

void PluginSettingsControllerTests::listsPluginsWithCapabilities() {
    const auto controller = makeController();

    const auto plugins = controller->plugins();

    QCOMPARE(plugins.size(), 2);
    const auto plexEntry = plugins.at(1).toMap();
    QCOMPARE(plexEntry.value(QStringLiteral("id")).toString(), QStringLiteral("plex"));
    QVERIFY(plexEntry.value(QStringLiteral("browserAuthentication")).toBool());
    QVERIFY(plexEntry.value(QStringLiteral("userSelection")).toBool());
    QVERIFY(!plugins.at(0).toMap().value(QStringLiteral("browserAuthentication")).toBool());
    QCOMPARE(controller->selectedPluginId(), QStringLiteral("mpv"));
}

void PluginSettingsControllerTests::selectedPluginExposesTypedFields() {
    const auto controller = makeController();

    const auto fields = controller->fields();

    QCOMPARE(fields.size(), 4);
    const auto enabled = fields.at(0).toMap();
    QCOMPARE(enabled.value(QStringLiteral("type")).toString(), QStringLiteral("boolean"));
    QCOMPARE(enabled.value(QStringLiteral("value")).toBool(), false);
    const auto executable = fields.at(1).toMap();
    QCOMPARE(executable.value(QStringLiteral("type")).toString(), QStringLiteral("text"));
    QVERIFY(executable.value(QStringLiteral("required")).toBool());
    QCOMPARE(executable.value(QStringLiteral("value")).toString(), QStringLiteral("mpv"));
    const auto quality = fields.at(2).toMap();
    QCOMPARE(quality.value(QStringLiteral("type")).toString(), QStringLiteral("selection"));
    QCOMPARE(quality.value(QStringLiteral("options")).toList().size(), 1);
    const auto seconds = fields.at(3).toMap();
    QCOMPARE(seconds.value(QStringLiteral("type")).toString(), QStringLiteral("number"));
    QCOMPARE(seconds.value(QStringLiteral("minimum")).toDouble(), 30.0);
    QCOMPARE(seconds.value(QStringLiteral("maximum")).toDouble(), 3600.0);
}

void PluginSettingsControllerTests::secretFieldIsMaskedAndNeverEchoed() {
    secrets.secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = Token;
    QString error;
    PluginSettingsSchema schema = plex->settingsSchema();
    schema.pluginId = QStringLiteral("secretive");
    schema.fields = {{.key = QStringLiteral("apiKey"), .type = PluginSettingType::Secret, .label = QStringLiteral("Key")}};
    QVERIFY(settings->registerSchema(schema, error));
    const auto controller = makeController();
    controller->SelectPlugin(QStringLiteral("secretive"));

    QVERIFY(controller->SetValue(QStringLiteral("apiKey"), QStringLiteral("typed-secret")));

    const auto field = controller->fields().first().toMap();
    QCOMPARE(field.value(QStringLiteral("type")).toString(), QStringLiteral("secret"));
    QCOMPARE(field.value(QStringLiteral("value")).toString(), PluginSettingsService::secretMask());
    QVERIFY(field.value(QStringLiteral("hasSecret")).toBool());
    QVERIFY(!controller->fields().isEmpty());
    for (const auto &entry : controller->fields()) {
        for (const auto &value : entry.toMap()) QVERIFY(value.toString() != QStringLiteral("typed-secret"));
    }
}

void PluginSettingsControllerTests::invalidValueIsRejectedWithMessage() {
    const auto controller = makeController();

    QVERIFY(!controller->SetValue(QStringLiteral("seconds"), 5));

    QVERIFY(!controller->errorMessage().isEmpty());
    QVERIFY(!controller->dirty());
}

void PluginSettingsControllerTests::saveReportsMissingRequiredFields() {
    const auto controller = makeController();
    QVERIFY(controller->SetValue(QStringLiteral("executablePath"), QStringLiteral("  ")));

    QVERIFY(!controller->Save());

    QVERIFY(controller->errorMessage().contains(QStringLiteral("executablePath")));
    QVERIFY(controller->dirty());
    QCOMPARE(applied, 0);
}

void PluginSettingsControllerTests::saveAppliesSettingsAndClearsDirty() {
    const auto controller = makeController();
    QSignalSpy changed(controller.get(), &PluginSettingsController::changed);
    QVERIFY(controller->SetValue(QStringLiteral("enabled"), true));
    QVERIFY(controller->dirty());

    QVERIFY(controller->Save());

    QVERIFY(!controller->dirty());
    QCOMPARE(applied, 1);
    QVERIFY(repository.stored.value(QStringLiteral("mpv")).value(QStringLiteral("enabled")).toBool());
    QVERIFY(!controller->statusMessage().isEmpty());
    QVERIFY(changed.count() >= 2);
}

void PluginSettingsControllerTests::discardRestoresSavedValues() {
    const auto controller = makeController();
    QVERIFY(controller->SetValue(QStringLiteral("enabled"), true));

    controller->Discard();

    QVERIFY(!controller->dirty());
    QCOMPARE(controller->fields().at(0).toMap().value(QStringLiteral("value")).toBool(), false);
}

void PluginSettingsControllerTests::connectRunsBrowserLoginAndReportsConnected() {
    http.route(QStringLiteral("POST https://plex.tv/api/v2/pins"), json(R"({"id":7,"code":"c","expiresIn":60})", 201));
    http.route(QStringLiteral("GET https://plex.tv/api/v2/pins/7"),
               json(QStringLiteral(R"({"id":7,"authToken":"%1"})").arg(Token).toUtf8()));
    const auto controller = makeController();
    controller->SelectPlugin(QStringLiteral("plex"));
    QCOMPARE(controller->connectionState(), QStringLiteral("disconnected"));

    controller->Connect();

    QCOMPARE(controller->connectionState(), QStringLiteral("connected"));
    QVERIFY(controller->errorMessage().isEmpty());
}

void PluginSettingsControllerTests::disconnectClearsLogin() {
    secrets.secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = Token;
    const auto controller = makeController();
    controller->SelectPlugin(QStringLiteral("plex"));
    QCOMPARE(controller->connectionState(), QStringLiteral("connected"));

    controller->Disconnect();

    QCOMPARE(controller->connectionState(), QStringLiteral("disconnected"));
    QVERIFY(!secrets.hasSecret(QStringLiteral("plex"), QStringLiteral("token")));
}

void PluginSettingsControllerTests::testConnectionShowsPluginResult() {
    const auto controller = makeController();

    controller->TestConnection();

    QCOMPARE(controller->errorMessage(), QStringLiteral("mpv executable was not found."));
}

void PluginSettingsControllerTests::selectingServerStoresItsId() {
    secrets.secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = Token;
    http.route(QStringLiteral("GET https://clients.plex.tv/api/v2/resources"),
               json(PlexFixture(QStringLiteral("plex-resources.json"))));
    const auto controller = makeController();
    controller->SelectPlugin(QStringLiteral("plex"));

    controller->LoadServers();
    QCOMPARE(controller->servers().size(), 2);
    controller->SelectServer(QStringLiteral("server-abc"));

    // Only Save applies it: the plugin keeps observing the saved server until then.
    QVERIFY(!plex->server().has_value());
    QVERIFY(controller->dirty());
    QVERIFY(controller->Save());
    QCOMPARE(repository.stored.value(QStringLiteral("plex")).value(QStringLiteral("serverId")).toString(),
             QStringLiteral("server-abc"));
    QVERIFY(controller->servers().at(0).toMap().value(QStringLiteral("selected")).toBool());
}

void PluginSettingsControllerTests::discardAfterSelectingServerKeepsTheSavedServer() {
    secrets.secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = Token;
    http.route(QStringLiteral("GET https://clients.plex.tv/api/v2/resources"),
               json(PlexFixture(QStringLiteral("plex-resources.json"))));
    const auto controller = makeController();
    controller->SelectPlugin(QStringLiteral("plex"));
    controller->LoadServers();
    controller->SelectServer(QStringLiteral("server-abc"));

    controller->Discard();

    QVERIFY(!plex->server().has_value());
    QVERIFY(settings->values(QStringLiteral("plex")).value(QStringLiteral("serverId")).toString().isEmpty());
    for (const auto &server : controller->servers()) {
        QVERIFY(!server.toMap().value(QStringLiteral("selected")).toBool());
    }
}

void PluginSettingsControllerTests::selectingUsersUpdatesObservedUsers() {
    secrets.secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = Token;
    http.route(QStringLiteral("GET https://clients.plex.tv/api/v2/resources"),
               json(PlexFixture(QStringLiteral("plex-resources.json"))));
    http.route(QStringLiteral("GET ") + ServerUri + QStringLiteral("/accounts"), json(PlexFixture(QStringLiteral("plex-users.json"))));
    const auto controller = makeController();
    controller->SelectPlugin(QStringLiteral("plex"));
    controller->LoadServers();
    controller->SelectServer(QStringLiteral("server-abc"));

    controller->LoadUsers();
    QCOMPARE(controller->users().size(), 2);
    controller->SetUserSelected(QStringLiteral("7"), true);
    controller->SetUserSelected(QStringLiteral("1"), true);
    controller->SetUserSelected(QStringLiteral("1"), false);

    QCOMPARE(settings->values(QStringLiteral("plex")).value(QStringLiteral("selectedUsers")).toString(), QStringLiteral("7"));
    QVERIFY(!controller->users().at(0).toMap().value(QStringLiteral("selected")).toBool());
    QVERIFY(controller->users().at(1).toMap().value(QStringLiteral("selected")).toBool());
}

void PluginSettingsControllerTests::messagesNeverContainTheToken() {
    http.route(QStringLiteral("POST https://plex.tv/api/v2/pins"), json(R"({"id":7,"code":"c","expiresIn":60})", 201));
    http.route(QStringLiteral("GET https://plex.tv/api/v2/pins/7"),
               json(QStringLiteral(R"({"id":7,"authToken":"%1"})").arg(Token).toUtf8()));
    http.route(QStringLiteral("GET https://clients.plex.tv/api/v2/resources"), json({}, 401));
    LogCapture logs;
    const auto controller = makeController();
    controller->SelectPlugin(QStringLiteral("plex"));

    controller->Connect();
    controller->TestConnection();
    controller->LoadServers();

    QVERIFY(!controller->statusMessage().contains(Token));
    QVERIFY(!controller->errorMessage().contains(Token));
    for (const auto &field : controller->fields()) {
        for (const auto &value : field.toMap()) QVERIFY(!value.toString().contains(Token));
    }
    QVERIFY(!logs.contains(Token));
}

void PluginSettingsControllerTests::reconnectingResumesPlexPolling() {
    secrets.secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = Token;
    http.route(QStringLiteral("POST https://plex.tv/api/v2/pins"), json(R"({"id":7,"code":"c","expiresIn":60})", 201));
    http.route(QStringLiteral("GET https://plex.tv/api/v2/pins/7"),
               json(QStringLiteral(R"({"id":7,"authToken":"%1"})").arg(Token).toUtf8()));
    NullObserver observer;
    plex->observe(observer);
    const auto controller = makeController();
    controller->SelectPlugin(QStringLiteral("plex"));
    controller->Disconnect();
    QVERIFY(!plex->isPolling());

    controller->Connect();

    QCOMPARE(controller->connectionState(), QStringLiteral("connected"));
    QVERIFY(plex->isPolling());
}

QTEST_MAIN(PluginSettingsControllerTests)
#include "PluginSettingsControllerTests.moc"
