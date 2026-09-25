#include <QtTest>

#include "../../src/presentation/home/HomeScreenController.h"

class FakeMediaReader final : public IMediaReader {
public:
    QList<Media> result;
    QString failure;

    bool readAll(QList<Media> &media, QString &error) override {
        error = failure;
        media = result;
        return failure.isEmpty();
    }

};

class HomeScreenControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void exposesReadyMedia();
    void exposesEmptyState();
    void exposesErrorState();
    void exposesInitializationErrorWithoutRepository();
    void synchronizationCompletionReloadsMedia();
    void exposesPresentationReadyStatus_data();
    void exposesPresentationReadyStatus();
};

void HomeScreenControllerTests::exposesReadyMedia() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.TotalChapters = 28;
    media.ConsumedChapters = 12;
    media.PersonalScore = 9;
    reader.result.append(media);

    HomeScreenController controller(reader);
    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("ready"));
    QCOMPARE(controller.statusMessage(), QStringLiteral("Dados locais carregados."));
    QCOMPARE(controller.mediaCount(), 1);
    const auto index = controller.mediaModel()->index(0, 0);
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::TitleRole).toString(),
             QStringLiteral("Frieren"));
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::ProgressRole).toString(),
             QStringLiteral("12/28"));
}

void HomeScreenControllerTests::exposesEmptyState() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);

    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("empty"));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::exposesErrorState() {
    FakeMediaReader reader;
    reader.failure = QStringLiteral("Database unavailable");
    HomeScreenController controller(reader);

    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("error"));
    QCOMPARE(controller.errorMessage(), QStringLiteral("Database unavailable"));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::exposesInitializationErrorWithoutRepository() {
    HomeScreenController controller(nullptr, QStringLiteral("Database initialization failed"));

    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("error"));
    QCOMPARE(controller.errorMessage(), QStringLiteral("Database initialization failed"));
    QCOMPARE(controller.statusMessage(), QStringLiteral("Não foi possível carregar os dados locais."));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::synchronizationCompletionReloadsMedia() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    reader.result.append(media);
    HomeScreenController controller(reader);

    controller.notifySynchronizationCompleted();

    QCOMPARE(controller.state(), QStringLiteral("ready"));
    QCOMPARE(controller.mediaCount(), 1);
    QCOMPARE(controller.statusMessage(), QStringLiteral("Dados locais carregados."));
}

void HomeScreenControllerTests::exposesPresentationReadyStatus_data() {
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("expectedLabel");

    QTest::newRow("unknown") << static_cast<int>(MediaStatus::Unknown)
                              << QStringLiteral("Desconhecido");
    QTest::newRow("not released") << static_cast<int>(MediaStatus::NotReleased)
                                   << QStringLiteral("Ainda não lançado");
    QTest::newRow("releasing") << static_cast<int>(MediaStatus::Releasing)
                                << QStringLiteral("Em lançamento");
    QTest::newRow("released") << static_cast<int>(MediaStatus::Released)
                               << QStringLiteral("Concluído");
}

void HomeScreenControllerTests::exposesPresentationReadyStatus() {
    QFETCH(int, status);
    QFETCH(QString, expectedLabel);
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Status = static_cast<MediaStatus>(status);
    reader.result.append(media);
    HomeScreenController controller(reader);

    controller.reload();

    const auto index = controller.mediaModel()->index(0, 0);
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::StatusLabelRole).toString(),
             expectedLabel);
}

QTEST_MAIN(HomeScreenControllerTests)
#include "HomeScreenControllerTests.moc"
