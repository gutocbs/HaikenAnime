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

#include "src/base/anime.h"
#include "src/base/database.h"
#include "src/base/confusuario.h"
#include "src/utilities/logger.h"
#include "src/utilities/formatapalavras.h"
#include "lib/anitomy/anitomy.h"
//REFEITO
class arquivos : public QObject
{
    Q_OBJECT
public:
    explicit arquivos(QObject *parent = nullptr);
    QString fprocuraEpisodio(anime*);
    QString fprocuraEpisodioEspecifico(anime*, int);
    bool fcomparaDadosAnime(QString, const QString &rnomeAnime, QString, const QStringList &rnomesAlternativosAnime, int, int);
    bool fabreEpisodio(const QByteArray &rcaminhoArquivo);
    int fcomparaSeasons(QString, int, int);

public slots:

private:
    QPointer<confUsuario> cconfUsuario;
    QPointer<Database> cdatabase;
    QVector<anime*> vlistaSelecionada;
    FormataPalavras formatador;
    QHash<QString,int> vEpisodiosTotaisPorAnime;
};

#endif // ARQUIVOS_H
