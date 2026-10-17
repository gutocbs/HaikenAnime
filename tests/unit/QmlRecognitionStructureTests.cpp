#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QRegularExpression>
#include <QStringListModel>
#include <QtTest>

namespace {
class FakePluginSettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList plugins READ plugins CONSTANT)
    Q_PROPERTY(QString selectedPluginId READ selectedPluginId CONSTANT)
    Q_PROPERTY(QVariantList fields READ fields CONSTANT)
    Q_PROPERTY(bool dirty READ dirty CONSTANT)
    Q_PROPERTY(bool busy READ busy CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage CONSTANT)
    Q_PROPERTY(QString connectionState READ connectionState CONSTANT)
    Q_PROPERTY(QVariantList servers READ servers CONSTANT)
    Q_PROPERTY(QVariantList users READ users CONSTANT)

public:
    QVariantList plugins() const {
        return {QVariantMap{{"id", "mpv"}, {"displayName", "mpv"}, {"connectionTest", true}},
                QVariantMap{{"id", "plex"}, {"displayName", "Plex"}, {"browserAuthentication", true},
                            {"connectionTest", true}, {"userSelection", true}}};
    }
    QString selectedPluginId() const { return QStringLiteral("plex"); }
    QVariantList fields() const {
        return {QVariantMap{{"key", "enabled"}, {"type", "boolean"}, {"label", "Enabled"}, {"value", true}},
                QVariantMap{{"key", "path"}, {"type", "text"}, {"label", "Path"}, {"value", "mpv"}, {"required", true}},
                QVariantMap{{"key", "seconds"}, {"type", "number"}, {"label", "Seconds"}, {"value", 180},
                            {"minimum", 30}, {"maximum", 3600}},
                QVariantMap{{"key", "quality"}, {"type", "selection"}, {"label", "Quality"}, {"value", "auto"},
                            {"options", QVariantList{QVariantMap{{"value", "auto"}, {"label", "Auto"}}}}},
                QVariantMap{{"key", "apiKey"}, {"type", "secret"}, {"label", "Key"}, {"value", "********"},
                            {"hasSecret", true}}};
    }
    bool dirty() const { return true; }
    bool busy() const { return false; }
    QString errorMessage() const { return {}; }
    QString statusMessage() const { return QStringLiteral("ok"); }
    QString connectionState() const { return QStringLiteral("connected"); }
    QVariantList servers() const { return {QVariantMap{{"id", "s"}, {"name", "Server"}, {"selected", true}}}; }
    QVariantList users() const { return {QVariantMap{{"id", "1"}, {"name", "owner"}, {"selected", true}}}; }

    Q_INVOKABLE void SelectPlugin(const QString &) {}
    Q_INVOKABLE bool SetValue(const QString &, const QVariant &) { return true; }
    Q_INVOKABLE bool ClearSecret(const QString &) { return true; }
    Q_INVOKABLE bool Save() { return true; }
    Q_INVOKABLE void Discard() {}
    Q_INVOKABLE void Connect() {}
    Q_INVOKABLE void Disconnect() {}
    Q_INVOKABLE void TestConnection() {}
    Q_INVOKABLE void LoadServers() {}
    Q_INVOKABLE void SelectServer(const QString &) {}
    Q_INVOKABLE void LoadUsers() {}
    Q_INVOKABLE void SetUserSelected(const QString &, bool) {}
};

class FakeRecognitionController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *records READ records CONSTANT)
    Q_PROPERTY(bool hasMore READ hasMore CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(QVariantList mediaChoices READ mediaChoices CONSTANT)

public:
    QObject *records() { return &model_; }
    bool hasMore() const { return false; }
    QString errorMessage() const { return {}; }
    QVariantList mediaChoices() const { return {QVariantMap{{"id", 21}, {"title", "One Piece"}}}; }

    Q_INVOKABLE void Reload() {}
    Q_INVOKABLE void LoadMore() {}
    Q_INVOKABLE bool Correct(qint64, int, int) { return true; }
    Q_INVOKABLE bool Ignore(qint64) { return true; }

private:
    QStringListModel model_;
};

class FakeNowWatchingController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ active CONSTANT)
    Q_PROPERTY(QVariantList sessions READ sessions CONSTANT)
    Q_PROPERTY(QString selectedSessionKey READ selectedSessionKey CONSTANT)
    Q_PROPERTY(QVariantMap current READ current CONSTANT)

public:
    bool active() const { return true; }
    QVariantList sessions() const {
        return {QVariantMap{{"key", "plex:a"}, {"label", "One Piece · owner · Plex"}},
                QVariantMap{{"key", "mpv:b"}, {"label", "ep02.mkv · local · mpv"}}};
    }
    QString selectedSessionKey() const { return QStringLiteral("plex:a"); }
    QVariantMap current() const {
        return {{"associated", true}, {"animeTitle", "One Piece"}, {"progress", "5/12"}, {"listStatus", "current"},
                {"season", 1}, {"episode", 6}, {"episodeTitle", "Romance Dawn"}, {"summary", "Luffy sets sail."},
                {"duration", "24 min"}, {"airDate", "20/10/1999"}, {"source", "Plex"}, {"user", "owner"},
                {"device", "TV"}, {"sourceTitle", "One Piece"}};
    }
    Q_INVOKABLE void SelectSession(const QString &) {}
};

QString source(const QString &name) {
    QFile file(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml/") + name);
    return file.open(QIODevice::ReadOnly | QIODevice::Text) ? QString::fromUtf8(file.readAll()) : QString();
}

QUrl url(const QString &name) {
    return QUrl::fromLocalFile(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml/") + name);
}

const QStringList RecognitionFiles{QStringLiteral("PluginSettingsScreen.qml"), QStringLiteral("PluginSettingField.qml"),
                                   QStringLiteral("EpisodeRecognitionScreen.qml"), QStringLiteral("NowWatchingScreen.qml")};
}

class QmlRecognitionStructureTests final : public QObject {
    Q_OBJECT

private slots:
    void pluginSettingsScreenLoadsWithController();
    void recognitionScreenLoadsWithController();
    void qmlHasNoTimersNetworkOrPlayerParsing();
    void qmlNeverDisplaysSecretsOrUsesNativeButtons();
    void secretInputIsPasswordAndStartsEmpty();
    void recognitionScreenIsNotAConfirmationQueue();
    void settingsScreenHostsPluginsAndHistory();
    void nowWatchingScreenLoadsWithController();
    void nowWatchingScreenShowsEmptyMessageAndUnassociatedLabel();
    void homeButtonIsRightOfSettingsAndGuarded();
    void mainHasNowWatchingPage();
};

void QmlRecognitionStructureTests::pluginSettingsScreenLoadsWithController() {
    QQmlEngine engine;
    FakePluginSettingsController controller;
    engine.rootContext()->setContextProperty(QStringLiteral("pluginSettingsController"), &controller);
    QQmlComponent component(&engine, url(QStringLiteral("PluginSettingsScreen.qml")));

    std::unique_ptr<QObject> object(component.create());

    QVERIFY2(object, qPrintable(component.errorString()));
}

void QmlRecognitionStructureTests::recognitionScreenLoadsWithController() {
    QQmlEngine engine;
    FakeRecognitionController controller;
    engine.rootContext()->setContextProperty(QStringLiteral("recognitionController"), &controller);
    QQmlComponent component(&engine, url(QStringLiteral("EpisodeRecognitionScreen.qml")));

    std::unique_ptr<QObject> object(component.create());

    QVERIFY2(object, qPrintable(component.errorString()));
}

void QmlRecognitionStructureTests::qmlHasNoTimersNetworkOrPlayerParsing() {
    const QStringList forbidden{QStringLiteral("Timer"), QStringLiteral("XMLHttpRequest"), QStringLiteral("graphql"), QStringLiteral("anilist.co"),
                                QStringLiteral("time-pos"), QStringLiteral("status/sessions"), QStringLiteral("plex.tv"),
                                QStringLiteral("input-ipc-server"), QStringLiteral("viewOffset")};
    for (const auto &file : RecognitionFiles) {
        const auto text = source(file);
        QVERIFY2(!text.isEmpty(), qPrintable(file));
        for (const auto &word : forbidden) {
            QVERIFY2(!text.contains(word, Qt::CaseInsensitive), qPrintable(file + QStringLiteral(": ") + word));
        }
    }
}

void QmlRecognitionStructureTests::qmlNeverDisplaysSecretsOrUsesNativeButtons() {
    const QRegularExpression nativeButton(QStringLiteral(R"((?:^|[^\w.])(?:Basic\.)?Button\s*\{)"),
                                          QRegularExpression::MultilineOption);
    for (const auto &file : RecognitionFiles) {
        const auto text = source(file);
        QVERIFY2(!text.contains(QStringLiteral("authToken")), qPrintable(file));
        QVERIFY2(!text.contains(QStringLiteral("accessToken")), qPrintable(file));
        QVERIFY2(!nativeButton.match(text).hasMatch(), qPrintable(file));
    }
}

void QmlRecognitionStructureTests::secretInputIsPasswordAndStartsEmpty() {
    const auto text = source(QStringLiteral("PluginSettingField.qml"));
    const auto start = text.indexOf(QStringLiteral("id: secretInput"));
    QVERIFY(start >= 0);
    const auto block = text.mid(start, text.indexOf(QLatin1Char('}'), start) - start);

    QVERIFY(block.contains(QStringLiteral("echoMode: TextInput.Password")));
    QVERIFY(block.contains(QStringLiteral("text: \"\"")));
    QVERIFY(!block.contains(QStringLiteral("text: field.value")));
}

void QmlRecognitionStructureTests::recognitionScreenIsNotAConfirmationQueue() {
    const auto text = source(QStringLiteral("EpisodeRecognitionScreen.qml"));

    QVERIFY(!text.contains(QStringLiteral("Confirmar"), Qt::CaseInsensitive));
    QVERIFY(text.contains(QStringLiteral("controller.Correct(")));
    QVERIFY(text.contains(QStringLiteral("controller.Ignore(")));
}

void QmlRecognitionStructureTests::settingsScreenHostsPluginsAndHistory() {
    const auto text = source(QStringLiteral("SettingsScreen.qml"));

    QVERIFY(text.contains(QStringLiteral("key: \"plugins\"")));
    QVERIFY(text.contains(QStringLiteral("PluginSettingsScreen {")));
    QVERIFY(text.contains(QStringLiteral("EpisodeRecognitionScreen {")));
    QVERIFY(!text.contains(QStringLiteral("O histórico será preenchido em uma etapa posterior.")));
}

void QmlRecognitionStructureTests::nowWatchingScreenLoadsWithController() {
    QQmlEngine engine;
    FakeNowWatchingController controller;
    engine.rootContext()->setContextProperty(QStringLiteral("nowWatchingController"), &controller);
    QQmlComponent component(&engine, url(QStringLiteral("NowWatchingScreen.qml")));

    std::unique_ptr<QObject> object(component.create());

    QVERIFY2(object, qPrintable(component.errorString()));
}

void QmlRecognitionStructureTests::nowWatchingScreenShowsEmptyMessageAndUnassociatedLabel() {
    const auto text = source(QStringLiteral("NowWatchingScreen.qml"));

    QVERIFY(text.contains(QStringLiteral("Nada sendo reproduzido no momento")));
    QVERIFY(text.contains(QStringLiteral("não associado à biblioteca")));
    QVERIFY(text.contains(QStringLiteral("signal backRequested()")));
}

void QmlRecognitionStructureTests::homeButtonIsRightOfSettingsAndGuarded() {
    const auto text = source(QStringLiteral("Home.qml"));
    const auto settings = text.indexOf(QStringLiteral("qsTr(\"Configurações\")"));
    const auto watching = text.indexOf(QStringLiteral("qsTr(\"Assistindo agora\")"));

    QVERIFY(settings >= 0);
    QVERIFY(watching > settings);
    const auto block = text.mid(watching, text.indexOf(QLatin1Char('}'), watching) - watching);
    QVERIFY(block.contains(QStringLiteral("nowWatchingController !== null")));
    QVERIFY(block.contains(QStringLiteral("openNowWatchingRequested()")));
    QVERIFY(text.contains(QStringLiteral("signal openNowWatchingRequested()")));
}

void QmlRecognitionStructureTests::mainHasNowWatchingPage() {
    const auto text = source(QStringLiteral("Main.qml"));

    QVERIFY(text.contains(QStringLiteral("NowWatchingScreen {")));
    QVERIFY(text.contains(QStringLiteral("root.page === \"nowWatching\"")));
    QVERIFY(text.contains(QStringLiteral("onOpenNowWatchingRequested: root.page = \"nowWatching\"")));
}

QTEST_MAIN(QmlRecognitionStructureTests)
#include "QmlRecognitionStructureTests.moc"
