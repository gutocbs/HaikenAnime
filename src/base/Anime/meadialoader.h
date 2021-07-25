#ifndef MEADIALOADER_H
#define MEADIALOADER_H

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

class MeadiaLoader : public QObject
{
    Q_OBJECT
public:
    explicit MeadiaLoader(QObject *parent = nullptr);
    bool loadMediaFromFile();
    QString getQStringValueFromKey(QString objectName, QString key = "");
    QStringList getQStringListValuesFromKey(QString objectName, QString key = "");
    int getNumberValueFromKey(QString objectName, QString key = "");
    bool fchecaDatabaseReady();
    void finsereDataHashId(QString tipo, QString lista, QString id, QStringList nomes);
    void insereDataHashPosicao(Enums::mediaType type, QString id, int posicao);
    void fcarregaListaAnoEmThread();

private:
    bool finishedLoading;
    QJsonObject mediaObject;
    QPointer<IMediaListManager> mediaListManager;
    QPointer<IMediaSearchManager> mediaSearchManager;
    QPointer<IMediaManager> mediaManager;
};

#endif // MEADIALOADER_H
