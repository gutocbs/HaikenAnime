#ifndef DIRECTORIESCONFIGURATIONLOADER_H
#define DIRECTORIESCONFIGURATIONLOADER_H

#include <QObject>
#include <QDir> //Cria diretorios
#include <QFile> //Cria arquivos
#include "directoryenums.h"

class DirectoriesConfigurationLoader : public QObject
{
    Q_OBJECT
public:
    explicit DirectoriesConfigurationLoader(QObject *parent = nullptr);
    static void createBaseDirectories();
    static QString getDirectory(DirectoryEnums::directoryType dirType, DirectoryEnums::directoryImageType dirImageType);
    static inline QString vdiretorioImagensMedio = "";
    static inline QString vdiretorioImagensGrandes = "";
    static inline QString vdiretorioImagensPequenas = "";
    static inline QString vimagemStaple = "";
    static inline QString vimagemAvatar = "";
signals:
private:
    static void createImageFolders();
    static void createTempFolders();
};

#endif // DIRECTORIESCONFIGURATIONLOADER_H
