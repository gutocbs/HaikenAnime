#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/app/MpvAwareLocalFileOpener.h"
#include "../../src/app/RecognitionComposition.h"
#include "../../src/infrastructure/database/SqliteDatabase.h"
#include "PlexTestSupport.h"

namespace {
class FakeMediaReader final : public IMediaReader {
public:
    bool readAll(QList<Media> &media, QString &error) override {
        error.clear();
        media.clear();
        return true;
    }
};

class FakeChangeWriter final : public IPersonalListChangeWriter {
public:
    bool remove(const Media &, const AniListPendingChange &, QString &) override { return true; }
    bool save(const Media &, const QList<AniListPendingChange> &, QString &) override { return true; }
};

class FakeLauncher final : public IProcessLauncher {
public:
    bool start(const QString &startedProgram, const QStringList &startedArguments, QString &error) override {
        if (fail) {
            error = QStringLiteral("cannot start");
            return false;
        }
        program = startedProgram;
        arguments = startedArguments;
        return true;
    }
    [[nodiscard]] bool canStart(const QString &) const override { return !fail; }

    bool fail = false;
    QString program;
    QStringList arguments;
};

class FakeOpener final : public ILocalFileOpener {
public:
    bool open(const QString &path, QString &error) override {
        error.clear();
        opened.append(path);
        return true;
    }
    QStringList opened;
};

/// Secret store shared across contexts so persistence of the client identifier can be observed.
class SharedSecrets final : public IPluginSecretStore {
public:
    explicit SharedSecrets(FakeSecretStore &store) : store_(store) {}
    bool hasSecret(const QString &p, const QString &k) const override { return store_.hasSecret(p, k); }
    bool readSecret(const QString &p, const QString &k, QString &v, QString &e) override { return store_.readSecret(p, k, v, e); }
    bool writeSecret(const QString &p, const QString &k, const QString &v, QString &e) override {
        return store_.writeSecret(p, k, v, e);
    }
    bool clearSecret(const QString &p, const QString &k, QString &e) override { return store_.clearSecret(p, k, e); }

private:
    FakeSecretStore &store_;
};
}

class RecognitionCompositionTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void startsWithoutPlexServerOrMpv();
    void invalidStoredSettingsDoNotBlockStartup();
    void missingDatabaseStillStartsWithWarnings();
    void startupRecoversRecordsInterruptedByACrash();
    void plexClientIdentifierIsPersistent();
    void openerUsesDefaultAppWhileMpvIsDisabled();
    void openerLaunchesConfiguredMpvWithIpcWhenEnabled();
    void openerFallsBackWhenMpvCannotStart();
    void stopAndDestructionShutDownCleanly();

private:
    RecognitionCompositionDependencies dependencies(QSqlDatabase connection);

    std::unique_ptr<QTemporaryDir> directory;
    std::unique_ptr<SqliteDatabase> database;
    FakeMediaReader reader;
    FakeChangeWriter changeWriter;
    std::unique_ptr<PersonalListChangeService> changes;
    FakeSecretStore secrets;
    FakeLauncher *launcher = nullptr;
    bool launcherFails = false;
};

void RecognitionCompositionTests::init() {
    directory = std::make_unique<QTemporaryDir>();
    database = std::make_unique<SqliteDatabase>(directory->filePath(QStringLiteral("app.sqlite")));
    QVERIFY(database->open());
    QVERIFY2(database->migrate(), qPrintable(database->lastError()));
    changes = std::make_unique<PersonalListChangeService>(changeWriter);
    secrets = FakeSecretStore();
    launcher = nullptr;
    launcherFails = false;
}

void RecognitionCompositionTests::cleanup() {
    database.reset();
    directory.reset();
}

RecognitionCompositionDependencies RecognitionCompositionTests::dependencies(QSqlDatabase connection) {
    auto fakeLauncher = std::make_unique<FakeLauncher>();
    fakeLauncher->fail = launcherFails;
    launcher = fakeLauncher.get();
    return {.connection = std::move(connection),
            .mediaReader = &reader,
            .changes = changes.get(),
            .secretStore = std::make_unique<SharedSecrets>(secrets),
            .processLauncher = std::move(fakeLauncher),
            .openUrl = [](const QUrl &) { return false; },
            .schedule = [](int, std::function<void()>) {}};
}

void RecognitionCompositionTests::startsWithoutPlexServerOrMpv() {
    const auto context = createRecognitionContext(dependencies(database->connection()));

    QVERIFY(context != nullptr);
    QVERIFY2(context->warnings.isEmpty(), qPrintable(context->warnings.join(QStringLiteral("; "))));
    QVERIFY(context->registry.plugin(QStringLiteral("mpv")) != nullptr);
    QVERIFY(context->registry.plugin(QStringLiteral("plex")) != nullptr);
    QCOMPARE(context->settings->schemas().size(), 3);
    QVERIFY(context->recognition->isRunning());
    QVERIFY(!context->isMpvEnabled());
}

void RecognitionCompositionTests::invalidStoredSettingsDoNotBlockStartup() {
    QSqlQuery query(database->connection());
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO plugin_setting (plugin_id, setting_key, setting_value) VALUES ('plex', 'pollSeconds', 'not json')")));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO plugin_setting (plugin_id, setting_key, setting_value) VALUES ('mpv', 'enabled', '[\"yes\"]')")));

    const auto context = createRecognitionContext(dependencies(database->connection()));

    QVERIFY(context->recognition->isRunning());
    QVERIFY(!context->warnings.isEmpty());
    QVERIFY(!context->isMpvEnabled());
}

void RecognitionCompositionTests::startupRecoversRecordsInterruptedByACrash() {
    QSqlQuery query(database->connection());
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO recognition_record (plugin_id, session_key, user_id, device_id, deduplication_key, title, "
        "media_id, season, episode, confidence, status, diagnostic, sync_state, sync_error, created_at, updated_at) "
        "VALUES ('mpv', 'abc', '', '', 'mpv|abc|21|6', 'One Piece', 21, NULL, 6, 1.0, 'pending', '', "
        "'not-required', '', '2026-10-17T00:00:00Z', '2026-10-17T00:00:00Z')")));

    const auto context = createRecognitionContext(dependencies(database->connection()));

    QVERIFY(query.exec(QStringLiteral("SELECT status, diagnostic FROM recognition_record")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("uncertain"));
    QCOMPARE(query.value(1).toString(), QStringLiteral("interrupted"));
}

void RecognitionCompositionTests::missingDatabaseStillStartsWithWarnings() {
    const auto context = createRecognitionContext(dependencies(QSqlDatabase()));

    QVERIFY(context != nullptr);
    QVERIFY(context->recognition->isRunning());
    QVERIFY(!context->warnings.isEmpty());
}

void RecognitionCompositionTests::plexClientIdentifierIsPersistent() {
    QString first;
    {
        const auto context = createRecognitionContext(dependencies(database->connection()));
        QString error;
        QVERIFY(secrets.readSecret(QStringLiteral("plex"), QStringLiteral("clientIdentifier"), first, error));
        QVERIFY(!first.isEmpty());
    }
    const auto context = createRecognitionContext(dependencies(database->connection()));
    QString second;
    QString error;
    QVERIFY(secrets.readSecret(QStringLiteral("plex"), QStringLiteral("clientIdentifier"), second, error));

    QCOMPARE(second, first);
}

void RecognitionCompositionTests::openerUsesDefaultAppWhileMpvIsDisabled() {
    const auto context = createRecognitionContext(dependencies(database->connection()));
    FakeOpener fallback;
    MpvAwareLocalFileOpener opener(fallback, *context);
    QString error;

    QVERIFY(opener.openEpisode({.mediaId = 21, .episode = 3, .path = QStringLiteral("D:/Anime/03.mkv")}, error));

    QCOMPARE(fallback.opened, QStringList({QStringLiteral("D:/Anime/03.mkv")}));
    QVERIFY(launcher->program.isEmpty());
}

void RecognitionCompositionTests::openerLaunchesConfiguredMpvWithIpcWhenEnabled() {
    const auto context = createRecognitionContext(dependencies(database->connection()));
    QString error;
    QVERIFY(context->settings->setValue(QStringLiteral("mpv"), QStringLiteral("enabled"), true, error));
    QVERIFY(context->settings->setValue(QStringLiteral("mpv"), QStringLiteral("executablePath"),
                                        QStringLiteral("C:/Tools/mpv/mpv.exe"), error));
    QVERIFY2(context->settings->save(QStringLiteral("mpv"), error), qPrintable(error));
    applyRecognitionSettings(*context);
    FakeOpener fallback;
    MpvAwareLocalFileOpener opener(fallback, *context);

    QVERIFY2(opener.openEpisode({.mediaId = 21, .episode = 3, .path = QStringLiteral("D:/Anime/03.mkv")}, error),
             qPrintable(error));

    QVERIFY(fallback.opened.isEmpty());
    QCOMPARE(launcher->program, QStringLiteral("C:/Tools/mpv/mpv.exe"));
    QVERIFY(launcher->arguments.first().startsWith(QStringLiteral(R"(--input-ipc-server=\\.\pipe\haikenanime-mpv-)")));
    QCOMPARE(launcher->arguments.last(), QStringLiteral("D:/Anime/03.mkv"));
}

void RecognitionCompositionTests::openerFallsBackWhenMpvCannotStart() {
    launcherFails = true;
    QStringList reported;
    auto withDiagnostics = dependencies(database->connection());
    withDiagnostics.diagnostics = [&reported](const QString &message) { reported.append(message); };
    const auto context = createRecognitionContext(std::move(withDiagnostics));
    QString error;
    QVERIFY(context->settings->setValue(QStringLiteral("mpv"), QStringLiteral("enabled"), true, error));
    QVERIFY(context->settings->save(QStringLiteral("mpv"), error));
    applyRecognitionSettings(*context);
    FakeOpener fallback;
    MpvAwareLocalFileOpener opener(fallback, *context);

    QVERIFY(opener.openEpisode({.mediaId = 21, .episode = 3, .path = QStringLiteral("D:/Anime/03.mkv")}, error));

    QCOMPARE(fallback.opened, QStringList({QStringLiteral("D:/Anime/03.mkv")}));
    // Raised long after startup: it must reach the log, not just a list nobody reads anymore.
    QVERIFY2(reported.join(QLatin1Char(';')).contains(QStringLiteral("mpv could not be started")),
             qPrintable(reported.join(QLatin1Char(';'))));
}

void RecognitionCompositionTests::stopAndDestructionShutDownCleanly() {
    auto context = createRecognitionContext(dependencies(database->connection()));
    context->recognition->stop();
    QVERIFY(!context->recognition->isRunning());
    context->recognition->start();

    context.reset();

    QVERIFY(context == nullptr);
}

QTEST_MAIN(RecognitionCompositionTests)
#include "RecognitionCompositionTests.moc"
