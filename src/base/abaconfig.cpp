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
    vauthCode = vsettings.value("authCode");
}

QVariant abaConfig::fgetAuthCode()
{
    return vauthCode;
}

void abaConfig::fsaveSettings()
{
    qDebug() << "TODO: SAVE SETTINGS";
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
    vsettings.setValue("service", QString(vmetaEnum.valueToKey(vclientEscolhido)));
}

void abaConfig::fsetUsername(QVariant username)
{
    vuser = username.toString();
    vsettings.setValue("username", vuser);
}

void abaConfig::fsetAuthCode(QVariant auth)
{
    vauthCode = auth;
    vsettings.setValue("authCode", vauthCode);
}
