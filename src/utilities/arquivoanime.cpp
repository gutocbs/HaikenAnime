#include "arquivoanime.h"

ArquivoAnime::ArquivoAnime(anime *ranimeBuscado)
{
    nome = ranimeBuscado->vnome;
    nomeIngles = ranimeBuscado->vnomeIngles;
    nomesAlternativos = ranimeBuscado->vnomeAlternativo;
    episodio = ranimeBuscado->vnumEpisodiosAssistidos.toInt();
    temporada = ranimeBuscado->vtemporada;
    id = ranimeBuscado->vid.toInt();
}

ArquivoAnime::ArquivoAnime(anime *ranimeBuscado, int episodioBuscado)
{
    nome = ranimeBuscado->vnome;
    nomeIngles = ranimeBuscado->vnomeIngles;
    nomesAlternativos = ranimeBuscado->vnomeAlternativo;
    episodio = episodioBuscado;
    temporada = ranimeBuscado->vtemporada;
    id = ranimeBuscado->vid.toInt();
}
