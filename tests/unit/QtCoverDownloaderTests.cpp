#include "infrastructure/covers/QtCoverDownloader.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

class QtCoverDownloaderTests : public QObject {
    Q_OBJECT
private slots:
    void classifiesResponses();
    void abortsOversizedBody();
};
static QUrl Serve(QTcpServer &server, QByteArray response)
{
    if (!server.listen(QHostAddress::LocalHost)) return {};
    QObject::connect(&server,&QTcpServer::newConnection,&server,[&server,response]{
        auto socket=server.nextPendingConnection();
        QObject::connect(socket,&QTcpSocket::readyRead,socket,[socket,response]{socket->readAll();socket->write(response);socket->disconnectFromHost();});
    });
    return QUrl(QString("http://127.0.0.1:%1/cover").arg(server.serverPort()));
}
void QtCoverDownloaderTests::classifiesResponses()
{
    QTemporaryDir dir; CoverSettings s; s.timeoutMs=1000; QtCoverDownloader downloader(dir.path(),s);
    QTcpServer server; auto url=Serve(server,"HTTP/1.1 429 Too Many Requests\r\nRetry-After: 2\r\nContent-Length: 0\r\n\r\n");
    bool done=false; CoverDownloadResult result;
    downloader.Start({1,url},[&](auto value){result=value;done=true;});
    QTRY_VERIFY_WITH_TIMEOUT(done,2000);
    QCOMPARE(result.failure,CoverFailureCategory::HttpTemporary); QCOMPARE(result.retryAfterMs,2000);
}
void QtCoverDownloaderTests::abortsOversizedBody()
{
    QTemporaryDir dir; CoverSettings s; s.maxResponseBytes=4; s.timeoutMs=1000; QtCoverDownloader downloader(dir.path(),s);
    QTcpServer server; auto url=Serve(server,"HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: 8\r\n\r\n12345678");
    bool done=false; CoverDownloadResult result;
    downloader.Start({1,url},[&](auto value){result=value;done=true;});
    QTRY_VERIFY_WITH_TIMEOUT(done,2000);
    QCOMPARE(result.failure,CoverFailureCategory::InvalidContent); QVERIFY(!QFileInfo::exists(result.temporaryPath));
}
QTEST_MAIN(QtCoverDownloaderTests)
#include "QtCoverDownloaderTests.moc"
