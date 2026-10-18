#include "RecognitionComposition.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QTimer>
#include <QUuid>

#include "../infrastructure/recognition/QtProcessLauncher.h"
#include "../infrastructure/recognition/mpv/QtLocalSocketMpvTransport.h"
#include "../infrastructure/secrets/WindowsPluginSecretStore.h"

#include <utility>

namespace {
const auto RecognitionSettingsId = QStringLiteral("recognition");
const auto ClientIdentifierKey = QStringLiteral("clientIdentifier");
constexpr int PlexHttpTimeoutMs = 15000;

QString Translate(const char *text) {
    return QCoreApplication::translate("RecognitionComposition", text);
}

PluginSettingsSchema RecognitionSchema() {
    return {.pluginId = RecognitionSettingsId,
            .displayName = Translate("Automatic recognition"),
            .fields = {{.key = QStringLiteral("minimumPlaybackSeconds"),
                        .type = PluginSettingType::Number,
                        .label = Translate("Minimum playback (seconds)"),
                        .description = Translate("Continuous playback needed before an unseen episode is marked watched."),
                        .defaultValue = RecognitionPolicy::DefaultMinimumPlaybackSeconds,
                        .minimum = 30,
                        .maximum = 3600}}};
}

/// Persistent per installation; kept in the secret store so it never appears in the settings UI.
QString PlexClientIdentifier(IPluginSecretStore &secrets, QStringList &warnings) {
    QString identifier;
    QString error;
    if (secrets.readSecret(PlexAuthentication::PluginId, ClientIdentifierKey, identifier, error) && !identifier.isEmpty()) {
        return identifier;
    }
    identifier = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!secrets.writeSecret(PlexAuthentication::PluginId, ClientIdentifierKey, identifier, error)) {
        warnings.append(QStringLiteral("The Plex client identifier could not be stored; a temporary one is used."));
    }
    return identifier;
}

QSet<QString> SplitUsers(const QString &value) {
    QSet<QString> users;
    for (const auto &user : value.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        if (!user.trimmed().isEmpty()) users.insert(user.trimmed());
    }
    return users;
}

void RegisterSchema(RecognitionContext &context, const PluginSettingsSchema &schema) {
    QString error;
    if (!context.settings->registerSchema(schema, error)) {
        // Unreadable settings must not block startup; the plugin keeps running with defaults.
        context.warnings.append(QStringLiteral("Settings of '%1' could not be loaded: %2").arg(schema.pluginId, error));
    }
}
}

std::unique_ptr<RecognitionContext> createRecognitionContext(RecognitionCompositionDependencies dependencies) {
    auto context = std::make_unique<RecognitionContext>();
    auto &ctx = *context;
    if (dependencies.mediaReader == nullptr || dependencies.changes == nullptr) {
        ctx.warnings.append(QStringLiteral("The local library is unavailable; automatic recognition is disabled."));
        return context;
    }
    if (!dependencies.connection.isValid() || !dependencies.connection.isOpen()) {
        ctx.warnings.append(QStringLiteral("The local database is unavailable; recognitions will not be saved."));
    }

    ctx.timerOwner = std::make_unique<QObject>();
    auto schedule = dependencies.schedule;
    if (!schedule) {
        schedule = [owner = ctx.timerOwner.get()](const int delayMs, std::function<void()> callback) {
            QTimer::singleShot(delayMs, owner, std::move(callback));
        };
    }
    auto openUrl = dependencies.openUrl;
    if (!openUrl) openUrl = [](const QUrl &url) { return QDesktopServices::openUrl(url); };

    ctx.secrets = dependencies.secretStore ? std::move(dependencies.secretStore)
                                           : std::make_unique<WindowsPluginSecretStore>();
    ctx.settingsRepository = std::make_unique<SqlitePluginSettingsRepository>(dependencies.connection);
    ctx.settings = std::make_unique<PluginSettingsService>(*ctx.settingsRepository, *ctx.secrets);
    ctx.historyRepository = std::make_unique<SqliteRecognitionHistoryRepository>(dependencies.connection);
    ctx.progressWriter = std::make_unique<PersonalListProgressWriter>(*dependencies.changes, dependencies.progressApplied);
    ctx.history = std::make_unique<RecognitionHistoryService>(*ctx.historyRepository, *ctx.progressWriter);
    // Before any plugin runs: records a crash left half-written must not stay "pending" forever.
    if (QString recoveryError; dependencies.connection.isOpen() && !ctx.history->recoverInterrupted(recoveryError)) {
        ctx.warnings.append(QStringLiteral("Interrupted recognitions could not be recovered: %1").arg(recoveryError));
    }
    // The decision reads the one media fresh from SQLite; only title matching uses a cached catalog.
    ctx.progressReader = std::make_unique<SqliteMediaProgressReader>(dependencies.connection);
    ctx.decisions = std::make_unique<ProgressDecisionService>(*ctx.progressReader, RecognitionPolicy());
    ctx.resolver = std::make_unique<CatalogEpisodeCandidateResolver>(*dependencies.mediaReader);

    ctx.processLauncher = dependencies.processLauncher ? std::move(dependencies.processLauncher)
                                                       : std::make_unique<QtProcessLauncher>();
    auto mpv = std::make_unique<MpvPlugin>(MpvPluginDependencies{
        .transportFactory = [] { return std::make_unique<QtLocalSocketMpvTransport>(); },
        .launcher = ctx.processLauncher.get(),
        .schedule = schedule,
        .log = dependencies.diagnostics});
    ctx.mpv = mpv.get();

    ctx.network = std::make_unique<QNetworkAccessManager>();
    ctx.plexHttp = std::make_unique<QtPlexHttp>(*ctx.network, PlexHttpTimeoutMs);
    const PlexClientIdentity identity{.clientIdentifier = PlexClientIdentifier(*ctx.secrets, ctx.warnings)};
    ctx.plexApi = std::make_unique<PlexApiClient>(*ctx.plexHttp, identity);
    ctx.plexAuthentication = std::make_unique<PlexAuthentication>(PlexAuthenticationDependencies{
        .http = ctx.plexHttp.get(), .secrets = ctx.secrets.get(), .identity = identity,
        .openUrl = openUrl, .schedule = schedule});
    auto plex = std::make_unique<PlexPlugin>(PlexPluginDependencies{
        .api = ctx.plexApi.get(), .authentication = ctx.plexAuthentication.get(), .schedule = schedule,
        .log = dependencies.info});
    ctx.plex = plex.get();

    RegisterSchema(ctx, RecognitionSchema());
    RegisterSchema(ctx, mpv->settingsSchema());
    RegisterSchema(ctx, plex->settingsSchema());
    ctx.registry.registerPlugin(std::move(mpv));
    ctx.registry.registerPlugin(std::move(plex));

    ctx.recognition = std::make_unique<EpisodeRecognitionService>(ctx.registry, *ctx.resolver, *ctx.decisions,
                                                                  *ctx.history, RecognitionPolicy(),
                                                                  dependencies.diagnostics);
    applyRecognitionSettings(ctx);
    ctx.recognition->start();
    return context;
}

void applyRecognitionSettings(RecognitionContext &context) {
    if (!context.settings || !context.recognition) return;
    const auto recognition = context.settings->values(RecognitionSettingsId);
    const auto seconds = recognition.value(QStringLiteral("minimumPlaybackSeconds")).toInt();
    context.recognition->setPolicy(RecognitionPolicy::create(seconds).value_or(RecognitionPolicy()));

    const auto mpv = context.settings->values(QStringLiteral("mpv"));
    context.mpvEnabled = mpv.value(QStringLiteral("enabled")).toBool();
    context.mpv->setExecutable(mpv.value(QStringLiteral("executablePath")).toString());

    const auto plex = context.settings->values(QStringLiteral("plex"));
    context.plex->setPollInterval(plex.value(QStringLiteral("pollSeconds")).toInt());
    context.plex->setSelectedUsers(SplitUsers(plex.value(QStringLiteral("selectedUsers")).toString()));
    const auto serverId = plex.value(QStringLiteral("serverId")).toString();
    if (!serverId.isEmpty() && context.plexAuthentication->hasToken()) {
        context.plex->selectServer(serverId, [&context](const bool selected, const QString &message) {
            if (!selected) {
                context.warnings.append(message);
                return;
            }
            // A saved server must be observed even if polling had stopped (e.g. after a rejected token).
            context.plex->resume();
        });
    }
}
