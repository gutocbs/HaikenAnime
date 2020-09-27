#include "abaconfig.h"

abaConfig *abaConfig::createInstance()
{
    return new abaConfig();
}

abaConfig *abaConfig::instance()
{
    return  Singleton<abaConfig>::instance(abaConfig::createInstance);
}

abaConfig::abaConfig(QObject *parent) : QObject(parent)
{
    vmetaEnumClient = QMetaEnum::fromType<clients>();
    vmetaEnumConfig = QMetaEnum::fromType<configs>();

    vauthCode = vsettings.value("authCode");
    vuser = vsettings.value("username").toString();
    vhighQualityImages = false;

    floadSettings();
}

QVariant abaConfig::fgetAuthCode()
{
    return vauthCode;
}

QVariant abaConfig::fgetDirectory()
{
    return QVariant(vanimesDirectory);
}

void abaConfig::fsaveSettings()
{
    fsetHighQualityImages();


    QVariantMap map;
    QVariantList directories;
    vanimesDirectory.sort();
    foreach(QString dir, vanimesDirectory){
        directories.append(dir);
    }
    map[QString(vmetaEnumConfig.valueToKey(configs::DIRECTORY))] = QVariant(directories);
    QJsonDocument document = QJsonDocument::fromVariant(map);

    QFile larquivo("Configurações/userConfig.txt");
    if(larquivo.open(QIODevice::WriteOnly)){
        larquivo.write(document.toJson());
        larquivo.close();
    }
    else
        qCritical() << "Error - " << larquivo.errorString();
}

void abaConfig::floadSettings()
{
    QByteArray data;
    QFile larquivo("Configurações/userConfig.txt");
    if(larquivo.size() == 0)
        return;
    if(larquivo.open(QIODevice::ReadOnly)){
        data = larquivo.readAll();
        larquivo.close();
    }
    else
        qCritical() << "Error - " << larquivo.errorString();

    QJsonDocument document = QJsonDocument::fromJson(data);
    QVariantList vlist = qvariant_cast<QVariantList>(document[QString(vmetaEnumConfig.valueToKey(configs::DIRECTORY))]);
    foreach(QVariant key, vlist) {
        vanimesDirectory.append(key.toString());
    }

    fsetGeneralFeed();
    fsetSpecificFeed();
    fsetTorrentOptions();
    fsetPlayers();
}

void abaConfig::fsetService(QVariant client)
{
    QString lclientEscolhido = client.toString();
    if(lclientEscolhido.compare("Anilist", Qt::CaseInsensitive) == 0)
        vclientEscolhido = abaConfig::clients::ANILIST;
    else if(lclientEscolhido.compare("MyAnimeList", Qt::CaseInsensitive) == 0)
        vclientEscolhido = abaConfig::clients::MYANIMELIST;
    if(lclientEscolhido.compare("Kitsu", Qt::CaseInsensitive) == 0)
        vclientEscolhido = abaConfig::clients::KITSU;
    vsettings.setValue("service", QString(vmetaEnumClient.valueToKey(vclientEscolhido)));
}

void abaConfig::fsetUsername(QVariant username)
{
    vuser = username.toString();
    vsettings.setValue("username", vuser);
}

bool abaConfig::fsetDirectory(QVariant dirPath)
{
    if(!vanimesDirectory.contains(dirPath.toString())){
        vanimesDirectory.append(dirPath.toString());
        return true;
    }
    return false;
}

bool abaConfig::fremoveDirectory(QVariant dirPath)
{
    if(vanimesDirectory.contains(dirPath.toString())){
        vanimesDirectory.removeOne(dirPath.toString());
        return true;
    }
    return false;
}

void abaConfig::fsetHighQualityImages()
{
    vhighQualityImages = vsettings.value("confDir/imagemQualityLow").toBool();
}

void abaConfig::fsetDetection()
{
    QStringList det = vsettings.value("confDir/detection").toStringList();
    if(!vdirConfigMap.contains("detection") || (vdirConfigMap.contains("detection") && vdirConfigMap["detection"] != det)){
        vdirConfigMap.insert("detection", det);
    }
}

void abaConfig::fsetPlayers()
{
    QStringList players = vsettings.value("confDir/players").toStringList();
    players.removeDuplicates();
    players.removeAll("true");
    players.removeAll("false");
    players.removeAll("");
    if(!vdirConfigMap.contains("players") || (vdirConfigMap.contains("players") && vdirConfigMap["players"] != players)){
        vdirConfigMap.insert("players", players);
    }
}

bool abaConfig::fgetHighQualityImages()
{
    return vhighQualityImages;
}

QStringList abaConfig::fgetDetection()
{
    return vdirConfigMap["detection"];
}

QStringList abaConfig::fgetPlayers()
{
    return vdirConfigMap["players"];
}

void abaConfig::fsetGeneralFeed()
{
    QStringList generalFeed;
    generalFeed.append(vsettings.value("confTorrent/generalFeed").toString());
    if(!vtorConfigMap.contains("generalFeed") || (vtorConfigMap.contains("generalFeed") &&
                                                  vtorConfigMap["generalFeed"] != generalFeed)){
        vtorConfigMap.insert("generalFeed", generalFeed);
    }
}

QString abaConfig::fgetGeneralFeed()
{
    return vtorConfigMap["generalFeed"].first();
}

void abaConfig::fsetSpecificFeed()
{
    QStringList specificFeed;
    specificFeed.append(vsettings.value("confTorrent/specificFeed").toString());
    if(!vtorConfigMap.contains("specificFeed") || (vtorConfigMap.contains("specificFeed") &&
                                                  vtorConfigMap["specificFeed"] != specificFeed)){
        vtorConfigMap.insert("specificFeed", specificFeed);
    }
}

QString abaConfig::fgetSpecificFeed()
{
    if(!vtorConfigMap["specificFeed"].first().isEmpty())
        return vtorConfigMap["specificFeed"].first();
    else
        return "";
}

QString abaConfig::fgetPreferredTorrent()
{
    if(!vtorConfigMap["preferredTorrent"].first().isEmpty())
        return vtorConfigMap["preferredTorrent"].first();
    else
        return "";
}

QString abaConfig::fgetPreferredTorrentPath()
{
    if(!vtorConfigMap["preferredTorrentPath"].isEmpty())
        return vtorConfigMap["preferredTorrentPath"].first();
    else
        return "";
}

QString abaConfig::fgetSaveFolder()
{
    if(!vtorConfigMap["saveFolder"].first().isEmpty())
        return vtorConfigMap["saveFolder"].first();
    else
        return "";
}

void abaConfig::fsetTorrentOptions()
{
    vautoDownloadTorrents = vsettings.value("confTorrent/autodownload").toBool();
    if(!vtorConfigMap.contains("autodownloadTimer") || (vtorConfigMap.contains("autodownloadTimer") &&
            vtorConfigMap["autodownloadTimer"] != vsettings.value("confTorrent/autodownloadTimer").toStringList())){
        vtorConfigMap.insert("autodownloadTimer", vsettings.value("confTorrent/autodownloadTimer").toStringList());
    }
    QStringList config = vsettings.value("confTorrent/downloadLists").toStringList();
    config.removeDuplicates();
    config.removeAll("");
    if(!vtorConfigMap.contains("downloadLists") || (vtorConfigMap.contains("downloadLists") &&
            vtorConfigMap["downloadLists"] != config)){
        vtorConfigMap.insert("downloadLists", config);
    }
    if(!vtorConfigMap.contains("preferredSub") || (vtorConfigMap.contains("preferredSub") &&
            vtorConfigMap["preferredSub"] != vsettings.value("confTorrent/preferredSub").toStringList())){
        vtorConfigMap.insert("preferredSub", vsettings.value("confTorrent/preferredSub").toStringList());
    }
    if(!vtorConfigMap.contains("preferredQuality") || (vtorConfigMap.contains("preferredQuality") &&
            vtorConfigMap["preferredQuality"] != vsettings.value("confTorrent/preferredQuality").toStringList())){
        vtorConfigMap.insert("preferredQuality", vsettings.value("confTorrent/preferredQuality").toStringList());
    }
    if(!vtorConfigMap.contains("preferredTorrent") || (vtorConfigMap.contains("preferredTorrent") &&
            vtorConfigMap["preferredTorrent"] != vsettings.value("confTorrent/preferredTorrent").toStringList())){
        vtorConfigMap.insert("preferredTorrent", vsettings.value("confTorrent/preferredTorrent").toStringList());
    }
    if(!vtorConfigMap.contains("preferredTorrentPath") || (vtorConfigMap.contains("preferredTorrentPath") &&
            vtorConfigMap["preferredTorrentPath"] != vsettings.value("confTorrent/preferredTorrentPath").toStringList())){
        vtorConfigMap.insert("preferredTorrentPath", vsettings.value("confTorrent/preferredTorrentPath").toStringList());
    }
    if(!vtorConfigMap.contains("saveFolder") || (vtorConfigMap.contains("saveFolder") &&
            vtorConfigMap["saveFolder"] != vsettings.value("confTorrent/saveFolder").toStringList())){
        vtorConfigMap.insert("saveFolder", vsettings.value("confTorrent/saveFolder").toStringList());
    }
}

QStringList abaConfig::fgetDownloadLists()
{
    return vtorConfigMap["downloadLists"];
}

QString abaConfig::fgetPreferredSub()
{
    return vtorConfigMap["preferredSub"].first();
}

QString abaConfig::fgetPreferredQuality()
{
    return vtorConfigMap["preferredQuality"].first();
}

void abaConfig::fsetAuthCode(QVariant auth)
{
    vauthCode = auth;
    vsettings.setValue("authCode", vauthCode);
}
