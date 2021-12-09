#ifndef ANILIST_H
#define ANILIST_H

#include <QObject>
#include <QtNetwork>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QFuture>
#include <QtConcurrent>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>

#include <QDesktopServices>
#include <QSettings>

#include <QJsonObject>
#include <QJsonDocument>
#include "src/clients/IClient.h"

class anilist : public IClient
{
    Q_OBJECT
public:
    enum AnilistQuery{AnimeInfo, Avatar, YearLists, YearMediaList, MediaId};
    Q_ENUM(AnilistQuery)
    enum AnilistMutationType{MutationScore, MutationList, MutationProgress, MutationDelete};
    Q_ENUM(AnilistMutationType)
    explicit anilist(QObject *parent = nullptr);
    ~anilist();
    bool updateList(int mediaId, Enums::mediaList mediaList);
    bool updateScore(int mediaId, int mediaScore);
    bool updateProgress(int mediaId, int mediaProgress);
    QString getAvatar();
    bool getMediaList();
    bool deleteMediaFromList(int mediaId);
    void setThread(QThread &cThread);
    bool getYearlyLists();
    bool getAvatarAndMediaList();

    void setAuthCode(const QString &ruser, QVariant);

signals:
    void downloadFinished(bool);
    void started();
public slots:

private slots:
    bool fgetListaAno(int year);
private:
    QNetworkRequest getRequest(bool auth = false);
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
