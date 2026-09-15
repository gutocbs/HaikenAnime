#ifndef ARQUIVOS_H
#define ARQUIVOS_H

#include <QDebug>
#include <QObject>
#include <QPointer>
#include <QDir>
#include <QDirIterator>//Itera a pasta
#include <QFile> // Ler arquivos na pasta
#include <QDesktopServices> //Abre episódio
#include <QProcess> //Tentando abrir o epi por aqui
#include <QUrl> //Também é usado pra abrir o episódio
#include <QLoggingCategory> //Cria classe de log

#include "anime.h"
#include "confusuario.h" //Retorna os diretorios de animes
#include "logger.h" //Mensagens de log customizadas
#include "formatapalavras.h" //... Bom, pra comparar palavras
#include "anitomy/anitomy.h"

class arquivos : public QObject
{
    Q_OBJECT
public:
    explicit arquivos(QObject *parent = nullptr);
    QString fprocuraEpisodio(anime*);
    QString fprocuraEpisodioEspecifico(anime*, int);
    bool fcomparaDadosAnime(QString, const QString &rnomeAnime, QString, const QStringList &rnomesAlternativosAnime, int, int);
    bool fabreEpisodio(const QByteArray &rcaminhoArquivo);
    void frecebeDiretorios(confUsuario*);
    void frecebeAnimes(leitorlistaanimes*);
    int fcomparaSeasons(QString, int, int);

public slots:

private:
    QPointer<confUsuario> cconfUsuario;
    QPointer<leitorlistaanimes> cleitorlistaanimes;
    QVector<anime*> vlistaSelecionada;
    FormataPalavras formatador;
    QHash<QString,int> vEpisodiosTotaisPorAnime;
};

#endif // ARQUIVOS_H
