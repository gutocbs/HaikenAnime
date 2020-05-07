#ifndef MAINCLASS_H
#define MAINCLASS_H

#include <QObject>
#include <QVector>
#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>
#include <QMetaEnum>

#include "src/base/leitorlistaanimes.h"
#include "src/base/anime.h"
#include "src/base/confbase.h"
#include "src/base/arquivos.h"
#include "src/base/confusuario.h"
#include "src/utilities/client.h"

class MainClass : public QObject
{
    Q_OBJECT
//    enum listas{CURRENT, COMPLETED, PAUSED, DROPPED, PLANNING};
//    enum type{ANIME, MANGA, NOVEL};
//    Q_ENUM(type)
public:
    explicit MainClass(QObject *parent = nullptr);
    enum lista{CURRENT, COMPLETED, PAUSED, DROPPED, PLANNING};
    Q_ENUM(lista)
public slots:
    void fmostraListaAnimes();
    void finfoAnimeSelecionado(QVariant);
    QVariant fretornaNomeAnimePosicao(QVariant);
    QVariant fretornaEpisodiosAnimePosicao(QVariant);
    QVariant fretornaNotaAnimePosicao(QVariant);
    QVariant fretornaListaAnimePosicao(QVariant);

    void fordemLista(QVariant);
    void fabreSite(QVariant);
    void fselecionaListaCurrent();
    void fselecionaListaCompleted();
    void fselecionaListaPaused();
    void fselecionaListaDropped();
    void fselecionaListaPlanning();
    void fabreProximoEpisodio();
    void fabrePastaAnime();
    void fabreStream();

    void fproximaPagina();
    void fanteriorPagina();

    void fmudaListaAnime(QVariant);

signals:
    void sidAnime1(QVariant data);
    void sidAnime2(QVariant data);
    void sidAnime3(QVariant data);
    void sidAnime4(QVariant data);
    void sidAnime5(QVariant data);
    void sidAnime6(QVariant data);
    void sidAnime7(QVariant data);
    void sidAnime8(QVariant data);
    void sidAnime9(QVariant data);
    void sidAnime10(QVariant data);
    void sidAnime11(QVariant data);
    void sidAnime12(QVariant data);

    void sidAnimeSelecionado(QVariant data);
    void snomeAnimeSelecionado(QVariant data);
    void snomeAlternativoAnimeSelecionado(QVariant data);
    void ssinopseAnimeSelecionado(QVariant data);
    void sstatusAnimeSelecionado(QVariant data);
    void sseasonAnimeSelecionado(QVariant data);
    void smediaGloballAnimeSelecionado(QVariant data);
    void smediaPessoalAnimeSelecionado(QVariant data);
    void sreleaseAnimeSelecionado(QVariant data);
    void sepisodiosLancadosAnimeSelecionado(QVariant data);
    void stipoAnimeSelecionado(QVariant data);
    void simagemAnimeSelecionado(QVariant data);

    void sdirImagensGrandes(QVariant data);
    void sdirImagensMedias(QVariant data);


private:
    //Classes
    leitorlistaanimes *cleitorListaAnimes;
    confBase *cconfiguracoesDiretoriosPadrao;
    arquivos *carquivos;
    confUsuario *cconfiguracoesUsuarioDiretorios;
    Client *cclient;

    QVector<anime*> vlistaSelecionada;

    int vindexAnimeSelecionado;
    int vposicaoGridAnimeSelecionado;
    int vpagina;

    QString vordemLista;
    QString vlistaAtual;
    leitorlistaanimes::type vtipoAtual;

    QMetaEnum vmetaEnum;
};

#endif // MAINCLASS_H
