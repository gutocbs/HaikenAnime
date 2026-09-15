#include "confbase.h"

confBase *confBase::createInstance()
{
    return new confBase();
}

confBase::confBase(QObject *parent) : QObject(parent)
{
}

confBase *confBase::instance()
{
    return  Singleton<confBase>::instance(confBase::createInstance);
}


///Terminar de escrever os QDebugs do jeito certo
void confBase::fcriaDiretoriosBase()
{
    //falta o c:/
    QString lconfPath = "Configurações";
    QString limagePath = lconfPath + QDir::separator() + "Imagens";
    QString ltempPath = lconfPath + QDir::separator() + "Temp";
    if(!QDir("Configurações").exists()){
        if(!QDir().mkdir(lconfPath))
            qDebug() << "Could not create path Configurações";
    }
    if(!QDir(limagePath).exists()){
        if(!QDir().mkdir(limagePath))
            qDebug() << "Could not create the folder";
    }
    if(!QDir(limagePath + QDir::separator() + "Pequeno").exists()){
        if(!QDir().mkdir(limagePath + QDir::separator() + "Pequeno"))
            qDebug() << "Could not create the folder";
    }
    if(!QDir(limagePath + QDir::separator() + "Medio").exists()){
        if(!QDir().mkdir(limagePath + QDir::separator() + "Medio"))
            qDebug() << "Could not create the folder";
    }
    if(!QDir(limagePath + QDir::separator() + "Grande").exists()){
        if(!QDir().mkdir(limagePath + QDir::separator() + "Grande"))
            qDebug() << "Could not create the folder";
    }
    if(!QDir(ltempPath).exists()){
        if(!QDir().mkdir(ltempPath))
            qDebug() << "Could not create the folder";
    }
    if(!QDir("Configurações/Temp/Torrents").exists()){
        if(!QDir().mkdir(ltempPath + QDir::separator() + "Torrents"))
            qDebug() << "Could not create the folder";
    }
    if(!QDir("Configurações/Temp/Imagens").exists()){
        if(!QDir().mkdir(ltempPath + QDir::separator() + "Imagens"))
            qDebug() << "Could not create path Configurações/Temp/Imagens";
    }
    if(!QDir("Configurações/Temp/Lists").exists()){
        if(!QDir().mkdir(ltempPath + QDir::separator() + "Lists"))
            qDebug() << "Could not create path Configurações/Temp/Lists";
    }
    vdiretorioImagensPequenas = QDir::currentPath() + QDir::separator() + limagePath + QDir::separator() + "Pequeno" + QDir::separator();
    vdiretorioImagensMedio = QDir::currentPath() + QDir::separator() + limagePath + QDir::separator() + "Medio" + QDir::separator();
    vdiretorioImagensGrandes = QDir::currentPath() + QDir::separator() + limagePath + QDir::separator() + "Grande" + QDir::separator();
    vimagemStaple = QDir::currentPath() + QDir::separator() + lconfPath + QDir::separator() + "Temp" + QDir::separator() + "imagemStaple.jpg";
    vimagemAvatar = QDir::currentPath() + QDir::separator() + ltempPath + QDir::separator() + "Imagens" + QDir::separator() + "avatar";
}

