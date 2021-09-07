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
    enum AnilistQuery{AnimeInfo, Avatar, YearLists, YearMediaList, MediaId};
    Q_ENUM(AnilistQuery)
    enum AnilistMutationType{MutationScore, MutationList, MutationProgress, MutationDelete};
    Q_ENUM(AnilistMutationType)
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
    bool fgetListaAno(int year);
private:
    QNetworkRequest getRequest(bool auth = false);
    bool getAvatar();
    bool getMediaList();
    QJsonDocument getMediaListObject();
    QJsonDocument getMediaYearListObject(int year);
    QString getAvatarURL();
    QString getQuery(AnilistQuery query);
    QString getMutationQuery(AnilistMutationType query, int id);
    QByteArray post(QString query, bool auth = false);
    int getMediaId(int anilistId);
    QString vtoken;
    QString vusername;
    QString vavatar;
    QString lastPage;
};

#endif // ANILIST_H
