#ifndef ANIME_H
#define ANIME_H

#include <QObject>
#include <QStringList>
#include <QDate> //Pega data de estreia e season

class anime : public QObject
{
    Q_OBJECT
public:
    explicit anime(QObject *parent = nullptr){
        Q_UNUSED(parent)
        vfinalizado = false;
        vtemporada = 0;
    }
    QString vnome;
    QString vnomeIngles;
    QString vnomeJapones;
    QStringList vnomeAlternativo; //StringList por que pode ter vários nomes alternativos
    QString vnumEpisodiosTotais;
    QString vnumEpisodiosAssistidos;
    QString vnumProximoEpisodioLancado;
    QString vnotaMediaSite;
    QString vLinkImagemMedia;
    QString vnotaMediaPessoal;
    QString vstatus;
    QString vsinopse;
    QString vid;
    QString vseason;
    QString vSeasonSemAno;
    QString vlista; //Esse atributo será usado para a função de busca e para função de mudar a lista do anime
    QString vformato; //Usado pra organizar a lista
    QString vdataEpisodio;
    QString vstreamCrunchyroll;
    QString vsiteAnilist;
    QDate vdataEstreia;
    bool vfinalizado;
    int vtemporada;
    //Adicionar mais depois

signals:

public slots:
};

#endif // ANIME_H
