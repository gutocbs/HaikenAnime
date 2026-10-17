#include <QtTest>

#include "../../src/application/recognition/PluginRegistry.h"

namespace {
class NamedPlugin final : public IPlaybackPlugin {
public:
    explicit NamedPlugin(QString pluginId) : pluginId_(std::move(pluginId)) {}
    [[nodiscard]] QString id() const override { return pluginId_; }
    [[nodiscard]] QString displayName() const override { return pluginId_; }
    [[nodiscard]] PluginCapabilities capabilities() const override { return {}; }
    [[nodiscard]] PluginSettingsSchema settingsSchema() const override { return {.pluginId = pluginId_}; }
    void observe(IPlaybackObserver &) override {}
    void testConnection(std::function<void(const ConnectionTestResult &)> completion) override { completion({}); }
    void stop() override {}

private:
    QString pluginId_;
};
}

class PluginRegistryTests final : public QObject {
    Q_OBJECT

private slots:
    void registersPluginsInOrder();
    void findsPluginById();
    void rejectsDuplicateIds();
    void rejectsNullAndBlankIds();
};

void PluginRegistryTests::registersPluginsInOrder() {
    PluginRegistry registry;

    QVERIFY(registry.registerPlugin(std::make_unique<NamedPlugin>(QStringLiteral("mpv"))));
    QVERIFY(registry.registerPlugin(std::make_unique<NamedPlugin>(QStringLiteral("plex"))));

    QCOMPARE(registry.plugins().size(), 2);
    QCOMPARE(registry.plugins().at(0)->id(), QStringLiteral("mpv"));
    QCOMPARE(registry.plugins().at(1)->id(), QStringLiteral("plex"));
}

void PluginRegistryTests::findsPluginById() {
    PluginRegistry registry;
    QVERIFY(registry.registerPlugin(std::make_unique<NamedPlugin>(QStringLiteral("plex"))));

    QVERIFY(registry.plugin(QStringLiteral("plex")) != nullptr);
    QCOMPARE(registry.plugin(QStringLiteral("plex"))->id(), QStringLiteral("plex"));
    QVERIFY(registry.plugin(QStringLiteral("crunchyroll")) == nullptr);
}

void PluginRegistryTests::rejectsDuplicateIds() {
    PluginRegistry registry;
    QVERIFY(registry.registerPlugin(std::make_unique<NamedPlugin>(QStringLiteral("plex"))));

    QVERIFY(!registry.registerPlugin(std::make_unique<NamedPlugin>(QStringLiteral("plex"))));

    QCOMPARE(registry.plugins().size(), 1);
}

void PluginRegistryTests::rejectsNullAndBlankIds() {
    PluginRegistry registry;

    QVERIFY(!registry.registerPlugin(nullptr));
    QVERIFY(!registry.registerPlugin(std::make_unique<NamedPlugin>(QStringLiteral(" "))));

    QVERIFY(registry.plugins().isEmpty());
}

QTEST_MAIN(PluginRegistryTests)
#include "PluginRegistryTests.moc"
