#ifndef ABACONFIG_H
#define ABACONFIG_H

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QDebug>
#include <QMetaEnum>
#include <QFile>
#include <QMap>
#include <QDir>
#include <QFile>

#include <QVariant>
#include <QVariantList>
#include <QVariantList>
#include <QJsonDocument>


#include "src/utilities/singleton.h"
class abaConfig : public QObject
{
    Q_OBJECT
    static abaConfig *createInstance();

public:
    explicit abaConfig(QObject *parent = nullptr);
    static abaConfig* instance();
    enum clients {ANILIST, MYANIMELIST, KITSU};
    enum configs {DIRECTORY, LOWQUALITY, TORRENT, AUTODOWNLOAD, SAVEFOLDER, DOWNLOADEDLISTS, SUB, QUALITY, FEED,
                  SPECIFICFEED
                 };
    Q_ENUM(clients)
    Q_ENUM(configs)

    void fsaveSettings();
    void floadSettings();

    QVariant fgetService();
    QVariant fgetAuthCode();
    QVariant fgetDirectory();
    QString fgetUsername();
    QString fgetPreferredTorrent();
    QString fgetPreferredTorrentPath();
    void fsaveAuthcode();
    void fsetAuthCode(QVariant);
    void fsetService(QVariant);
    void fsetUsername(QVariant);
    bool fsetDirectory(QVariant);
    bool fremoveDirectory(QVariant);

    void fsetHighQualityImages();
    void fsetDetection();
    void fsetPlayers();

    bool fgetHighQualityImages();
    QStringList fgetDetection();
    QStringList fgetPlayers();

    void fsetGeneralFeed();
    QString fgetGeneralFeed();
    void fsetSpecificFeed();
    QString fgetSpecificFeed();
    void fsetTorrentOptions();
    QStringList fgetDownloadLists();
    QString fgetPreferredSub();
    QString fgetPreferredQuality();
    QString fgetSaveFolder();
    bool fsetDefaultTorrentSoftware();
    bool fsetDeafultDownloadFolder();


private:

    QSettings vsettings;
    QMetaEnum vmetaEnumClient;
    QMetaEnum vmetaEnumConfig;

    clients vclientEscolhido;
    QString vuser;
    QVariant vauthCode;

    QStringList vanimesDirectory;

    //torrents
    //Filtros
    //Recognition
    bool vhighQualityImages;
    bool vautoDownloadTorrents;
    QMap<QString, QStringList> vdirConfigMap;
    QMap<QString, QStringList> vtorConfigMap;

};

#endif // ABACONFIG_H
