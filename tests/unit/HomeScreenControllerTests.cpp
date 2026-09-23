#include <QtTest>

#include "../../src/presentation/home/HomeScreenController.h"

class FakeMediaRepository final : public IMediaRepository {
public:
    QList<Media> result;
    QString failure;

    QList<Media> ReadAll(QString &error) override {
        error = failure;
        return result;
    }

    bool Upsert(const QList<Media> &, QString &) override { return true; }
};

class HomeScreenControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void exposesReadyMedia();
    void exposesEmptyState();
    void exposesErrorState();
    void exposesInitializationErrorWithoutRepository();
};

void HomeScreenControllerTests::exposesReadyMedia() {
    FakeMediaRepository reader;
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
    FakeMediaRepository reader;
    HomeScreenController controller(reader);

    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("empty"));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::exposesErrorState() {
    FakeMediaRepository reader;
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
    QCOMPARE(controller.statusMessage(), QStringLiteral("A sincronização falhou."));
    QCOMPARE(controller.mediaCount(), 0);
}

QTEST_MAIN(HomeScreenControllerTests)
#include "HomeScreenControllerTests.moc"
