#ifndef ANILIST_H
#define ANILIST_H

#include <QObject>
#include <QtNetwork>
#include <QNetworkRequest>
#include <QScopedPointer>

#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>

#include <QDesktopServices>
#include <QSettings>

#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>

#include "src/utilities/Enums.h"
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
    void frecebeAutorizacao(const QString &ruser, QVariant);

public slots:
    bool fgetList();

signals:
    void sterminouDownload(bool);
private slots:
    bool fgetListaAno(const QString &rano);
private:
    QNetworkRequest getRequest();
    bool getAvatar();
    bool getMediaList();
    QJsonDocument getMediaListObject();
    QString getQuery(Enums::AnilistQuery query);
    QByteArray post(QString query);
    QString vtoken;
    QString vusername;
    QString vavatar;
    QString lastPage;
};

#endif // ANILIST_H
