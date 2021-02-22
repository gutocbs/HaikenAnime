#ifndef ARQUIVOANIME_H
#define ARQUIVOANIME_H

#include <QObject>
#include "src/base/anime.h"

class ArquivoAnime : public QObject
{
    Q_OBJECT
public:
    ArquivoAnime(anime *ranimeBuscado);
    ArquivoAnime(anime *ranimeBuscado, int);
    QString nomeArquivo;
    QString nome;
    QString nomeIngles;
    QStringList nomesAlternativos;
    int episodio;
    int temporada;
    int id;
};

#endif // ARQUIVOANIME_H
