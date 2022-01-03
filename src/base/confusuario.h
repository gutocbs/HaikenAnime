#ifndef CONFUSUARIO_H
#define CONFUSUARIO_H

#include <QDebug>
#include <QObject>
#include <QVector>
#include <QDir> //Buscar diretorios em busca de pastas de animes
#include <QDirIterator> //Iterar diretorios de animes
#include <QMap> //Salva e lê as pastas especificas de cada anime
#include <QThread>

#include "src/utilities/singleton.h"

#include "src/base/anime.h"
#include "src/base/database.h"
#include "lib/anitomy/anitomy.h"
#include "src/base/leitorlistaanimes.h"
#include "src/utilities/formatapalavras.h"
//REFEITO
class confUsuario : public QObject
{
    Q_OBJECT
    static confUsuario *createInstance();

public:
    explicit confUsuario(QObject *parent = nullptr);
    static confUsuario* instance();

    Q_DECL_DEPRECATED QString fretornaDiretorioEspecifico(int); //MediaDirectories::getMediaFolderPath
    Q_DECL_DEPRECATED QVector<QString> fretornaDiretoriosAnimes(); //
    Q_DECL_DEPRECATED void fbuscaDiretoriosAnimes(); //MediaDirectories::searchForMediaDirectories
    void frecebeConfigs(const QStringList &ldiretorios);
    void fgetThread(QThread &dThread);
    Q_DECL_DEPRECATED void fsalvaPastasArquivos();// FileManagerSaver::saveMediaDirectories
    Q_DECL_DEPRECATED void flePastasArquivos(); //FileManagerLoader::loadMediaDirectories
    Q_DECL_DEPRECATED void fselecionaPastaEspecificaAnime(const QString &ridAnime, const QString &rdirAnime); //MediaDirectories::updateMediaDirectory

public slots:
    Q_DECL_DEPRECATED void fsetupListasPraBusca(); //FileManagerLoader::searchDirectories()

signals:
    void schecouPastas();

private:
    QPointer<Database> cdatabase;
    FormataPalavras formatador;

    QVector<anime*> vlistaAnimes;
    QStringList vdiretorioAnimes;

    QMap<QString, QString> vdiretorioEspecificoAnime;

    int vlista;
    bool vterminouChecagem;
};

#endif // CONFUSUARIO_H
