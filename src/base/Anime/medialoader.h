#ifndef MEDIALOADER_H
#define MEDIALOADER_H

#include <QObject>
#include <QDebug>
#include <QFile> //Abre arquivo do anilist
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMap>
#include <QHash>
#include <QVariantList>
#include <QPointer>
#include "src/utilities/Enums.h"
#include "src/utilities/singleton.h"
#include "src/DTO/media.h"
#include "src/base/Anime/animelistmanager.h"
#include "src/base/Anime/mediasearchmanager.h"
#include "src/base/Anime/animemanager.h"
#include "src/base/FileManager/filemanager.h"

//TODO - Fazer singleton
//TODO - Funções de ler arquivos secundários (nomes alternativos, animes por ano etc)
class MediaLoader : public QObject
{
    Q_OBJECT
public:
    explicit MediaLoader(QObject *parent = nullptr, IMediaListManager *mediaListManager = nullptr);
    bool loadMediaFromFile(bool mock = false);
    QString getFileName(bool mock = false);
    QJsonArray getMediaListArray(QString fileName);
    QPointer<Media> getMedia();
    QString getQStringValueFromKey(QString objectName, QString key = "");
    QStringList getQStringListValuesFromKey(QString objectName, QString key = "");
    int getNumberValueFromKey(QString objectName, QString key = "");
    int getNumberChapters(Enums::mediaType mediaType);
    QDate getStartDate(QString startDateObject);
    QString getNextEpisode(QString nextAiringEpisodeObject);
    QString getNextEpisodeDate(QString objectName);
    QString getDayOfTheWeek(QDateTime secondsSinceEpoch);
    bool fchecaDatabaseReady();
    void finsereDataHashId(QString tipo, QString lista, QString id, QStringList nomes);
    void insereDataHashPosicao(Enums::mediaType type, QString id, int posicao);
    void fcarregaListaAnoEmThread();

private:
    bool finishedLoading;
    int listSize;
    QJsonObject mediaObject;
    QHash<QString, QStringList> hashMediaNamesById;
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaManager> mediaManager;
};

#endif // MEDIALOADER_H
