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

void abaConfig::fsetAuthCode(QVariant auth)
{
    vauthCode = auth;
    vsettings.setValue("authCode", vauthCode);
}
