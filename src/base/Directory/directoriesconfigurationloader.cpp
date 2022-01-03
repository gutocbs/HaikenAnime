#include "directoriesconfigurationloader.h"

DirectoriesConfigurationLoader::DirectoriesConfigurationLoader(QObject *parent) : QObject(parent)
{

}

void DirectoriesConfigurationLoader::createBaseDirectories()
{
    QString confDirPath = "Configurations";
    if(!QDir(confDirPath).exists())
        QDir().mkdir(confDirPath);

    createImageFolders();
    createTempFolders();
}

void DirectoriesConfigurationLoader::createImageFolders()
{
    QString confDirPath = "Configurations";
    QString imgDirPath = confDirPath + QDir::separator() + "Images";
    vdiretorioImagensGrandes = imgDirPath+ QDir::separator() + DirectoryEnums::enumToQString(DirectoryEnums::BIG) + QDir::separator();
    vdiretorioImagensMedio = imgDirPath+ QDir::separator() + DirectoryEnums::enumToQString(DirectoryEnums::MEDIUM) + QDir::separator();
    vdiretorioImagensPequenas = imgDirPath+ QDir::separator() + DirectoryEnums::enumToQString(DirectoryEnums::SMALL) + QDir::separator();
    if(!QDir(imgDirPath).exists())
        QDir().mkdir(imgDirPath);
    if(!QDir(vdiretorioImagensPequenas).exists())
        QDir().mkdir(vdiretorioImagensPequenas);
    if(!QDir(vdiretorioImagensMedio).exists())
        QDir().mkdir(vdiretorioImagensMedio);
    if(!QDir(vdiretorioImagensGrandes).exists())
        QDir().mkdir(vdiretorioImagensGrandes);

}

void DirectoriesConfigurationLoader::createTempFolders()
{
    QString confDirPath = "Configurations";
    QString tempDirPath = confDirPath + QDir::separator() + "Temp";
    QString tempDirImages = tempDirPath+ QDir::separator() + DirectoryEnums::enumToQString(DirectoryEnums::IMAGE);
    if(!QDir(tempDirPath).exists())
        QDir().mkdir(tempDirPath);
    if(!QDir(tempDirPath+ QDir::separator() + DirectoryEnums::enumToQString(DirectoryEnums::TORRENT)).exists())
        QDir().mkdir(tempDirPath+ QDir::separator() + DirectoryEnums::enumToQString(DirectoryEnums::TORRENT));
    if(!QDir(tempDirImages).exists())
        QDir().mkdir(tempDirImages);
    if(!QDir(tempDirPath+ QDir::separator() + DirectoryEnums::enumToQString(DirectoryEnums::LIST)).exists())
        QDir().mkdir(tempDirPath+ QDir::separator() + DirectoryEnums::enumToQString(DirectoryEnums::LIST));

    vimagemStaple = tempDirImages + QDir::separator() + "staple.jpg";
    vimagemAvatar = tempDirImages + QDir::separator() + "avatar.jpg";
}
