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
#include "src/base/FileManager/filemanager.h"
#include "src/base/Media/mediacontroller.h"

class MediaLoader : public QObject
{
    Q_OBJECT
public:
    explicit MediaLoader(QObject *parent = nullptr);
    static bool loadMediaFromFile(bool mock = false);
private:
    static QString getFileName(bool mock = false);
    static QJsonArray getMediaListArray(QString fileName);
    static QPointer<Media> getMedia(QJsonArray mediaList, int index);
    static QString getQStringValueFromKey(QJsonObject mediaObject, QString objectName, QString key = "");
    static QStringList getQStringListValuesFromKey(QJsonObject mediaObject, QString objectName, QString key = "");
    static int getNumberValueFromKey(QJsonObject mediaObject, QString objectName, QString key = "");
    static int getNumberChapters(QJsonObject mediaObject, Enums::mediaType mediaType);
    static QDate getStartDate(QJsonObject mediaObject, QString startDateObject);
    static QString getNextEpisode(QJsonObject mediaObject, QString nextAiringEpisodeObject);
    static QString getNextEpisodeDate(QJsonObject mediaObject, QString objectName);
    static QString getDayOfTheWeek(QDateTime secondsSinceEpoch);
    static Enums::mediaType getMediaTypeFromKey(QJsonObject mediaObject, QString key);
    static Enums::mediaList getMediaListFromKey(QJsonObject mediaObject, QString key);
    static bool removeDeletedEntriesFromList();

    static inline QHash<int, Enums::mediaType> removedMediaId = QHash<int, Enums::mediaType>();
    static inline QHash<int, Enums::mediaType> mediaId = QHash<int, Enums::mediaType>();

    static bool fchecaDatabaseReady();
    static void finsereDataHashId(QString tipo, QString lista, QString id, QStringList nomes);
    static void insereDataHashPosicao(Enums::mediaType type, QString id, int posicao);
    static void fcarregaListaAnoEmThread();
};

#endif // MEDIALOADER_H