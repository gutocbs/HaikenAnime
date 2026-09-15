#ifndef ANILIST_H
#define ANILIST_H

#include <QObject>
#include <QtNetwork>

#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>

#include <QDesktopServices>
#include <QSettings>

#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>

class anilist : public QObject
{
    Q_OBJECT
public:
    explicit anilist(QObject *parent = nullptr);
    ~anilist();
    bool fmudaLista(int, const QString &rNovaLista);
    bool fmudaNota(int, int);
    ///id, progresso
    bool fmudaProgresso(int, int);
    bool fexcluiAnime(int);
    bool fgetListasAnoSeason();

    QString fretornaAvatar();

    void fbaixaListaThread(QThread &cThread);
    void frecebeAutorizacao(const QString &ruser, QString);

public slots:
    bool fgetList();

signals:
    void sterminouDownload(bool);
private slots:
    bool fgetListaAno(const QString &rano);
private:
//    QNetworkReply *vreply = nullptr;
    QString vtoken;
    QString vusername;
    QString vavatar;
};

#endif // ANILIST_H
