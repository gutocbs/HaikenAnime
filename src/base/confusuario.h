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

class confUsuario : public QObject
{
    Q_OBJECT
    static confUsuario *createInstance();

public:
    explicit confUsuario(QObject *parent = nullptr);
    static confUsuario* instance();

    QString fretornaDiretorioEspecifico(int);
    QVector<QString> fretornaDiretoriosAnimes();
    void fbuscaDiretoriosAnimes();
    void frecebeConfigs(const QStringList &ldiretorios);
    void fgetThread(QThread &dThread);
    void fsalvaPastasArquivos();
    void flePastasArquivos();
    void fselecionaPastaEspecificaAnime(const QString &ridAnime, const QString &rdirAnime);

public slots:
    void fsetupListasPraBusca();

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
