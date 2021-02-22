#include "arquivos.h"
#include <utility>
arquivos::arquivos(QObject *parent) : QObject(parent)
{
//    cconfUsuario = new confUsuario();
    cdatabase = new Database();
}

///fcomparaDadosAnime(QString rfileName, QString rnomeAnime, QString rnomeAnimeIngles, QStringList rnomesAlternativosAnime,int repisodioAnime, int rtemporada)
///Compara o arquivo com o anime a ser assistido. Caso for o episódio seguinte ao último assistido, retorna true.
///Caso seja um episódio inferior ou além do próximo que deve ser visto, retorna false.
/**
 * Compara o arquivo com o anime a ser assistido.
 * Caso for o episódio seguinte ao último assistido, retorna true.
 * Caso seja um episódio inferior ou além do próximo que deve ser visto, retorna false.
 * @param contem os dados do anime buscado.
 * @return true ou false.
 */
bool arquivos::fcomparaDadosAnime(ArquivoAnime* animeBuscado){

    //Anitomy é uma classe linda que separa os elementos de uma string
    anitomy::Anitomy lanitomy;
    lanitomy.Parse(animeBuscado->nomeArquivo.toStdWString());
    const auto& lelements = lanitomy.elements();
    //Usamos isso para pegar o número do episódio e o nome do anime a partir do nome do arquivo
    animeBuscado->nomeArquivo = QString::fromStdWString(lelements.get(anitomy::kElementAnimeTitle));
    QString vtemporada = QString::fromStdWString(lelements.get(anitomy::kElementAnimeSeason));
    if(!vtemporada.isEmpty())
        animeBuscado->nomeArquivo.append(QString(" " + vtemporada));
    int lepisodioAnime = QString::fromStdWString(lelements.get(anitomy::kElementEpisodeNumber)).toInt();

    int repisodiosTotais = 0;
    if(animeBuscado->temporada != 1)
        repisodiosTotais = fcomparaSeasons(animeBuscado->nome,lepisodioAnime, animeBuscado->temporada);
    //Alguns animes, normalmente filmes e ovas, não tem número de episódio, sendo lido como episódio 0
    //Por esse motivo, é dado o número 1 como número de episódio, assim o programa consegue reconhecer como episódio não visto
    if(lepisodioAnime == 0)
        lepisodioAnime++;

    //Episódios totais é a variável que conta todos os episódios do anime, em todas as seasons. Caso algum sub coloque, por exemplo
    //One Piece episódio 201, ele ainda vai ser lido e saberemos qual o episódio/temporada certa.
    if(formatador.fcomparaNomes(animeBuscado->nomeArquivo,animeBuscado->nome)
            && (lepisodioAnime == animeBuscado->episodio+1 ||
                lepisodioAnime - repisodiosTotais == animeBuscado->episodio+1)){
        return true;
    }
    else if(formatador.fcomparaNomes(animeBuscado->nomeArquivo, animeBuscado->nomeIngles)
            && (lepisodioAnime == animeBuscado->episodio+1 ||
                lepisodioAnime - repisodiosTotais == animeBuscado->episodio+1)){
        return true;
    }
    else{
        for(int i = 0; i < animeBuscado->nomesAlternativos.size(); i++){
            if(formatador.fcomparaNomes(animeBuscado->nomeArquivo,animeBuscado->nomesAlternativos.at(i))
                    && (lepisodioAnime == animeBuscado->episodio+1 ||
                        lepisodioAnime - repisodiosTotais == animeBuscado->episodio+1)){
                return true;
            }
        }
    }
    return false;
}

QString arquivos::fprocuraEpisodio(anime *ranimeBuscado){
    //Checa se o anime pode ser lido
    if(!cdatabase->instance()->fchecaDatabaseReady())
        return "";
    QPointer<ArquivoAnime> animeBuscado(new ArquivoAnime(ranimeBuscado));
    int id = ranimeBuscado->vid.toInt();

    //Verifica se a função retorna um valor que não está vazio, ou seja
    //Se existe uma pasta com o nome do anime
    if(!cconfUsuario->instance()->fretornaDiretorioEspecifico(id).isEmpty()){
        //Começa a iterar a pasta em busca das pastas de animes
        QDirIterator lit(cconfUsuario->instance()->fretornaDiretorioEspecifico(id), QDirIterator::Subdirectories);
        while(lit.hasNext()){
            QFile lfile(lit.next());
            QFileInfo lchecaSeArquivoOuPasta(lfile.fileName());
            //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de vídeo
            if(lchecaSeArquivoOuPasta.isFile() == true && (lfile.fileName().endsWith("mkv", Qt::CaseInsensitive) ||
                                                           lfile.fileName().endsWith("mp4", Qt::CaseInsensitive))){
                animeBuscado->nomeArquivo = lit.fileName();
                //Compara o nome do anime e o número do episódio
                if(fcomparaDadosAnime(animeBuscado))
                    return lfile.fileName();
            }
        }
    }
    else{
        //Começa a iterar a pasta em busca das pastas de animes
        for(int i = 0; i < cconfUsuario->instance()->fretornaDiretoriosAnimes().size(); i++){
            QDirIterator lit(cconfUsuario->instance()->fretornaDiretoriosAnimes().at(i), QDir::Files);
            while(lit.hasNext()){
                QFile lfile(lit.next());
                QFileInfo lchecaSeArquivoOuPasta(lfile.fileName());
                //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de vídeo
                if(lchecaSeArquivoOuPasta.isFile() == true && (lfile.fileName().endsWith("mkv", Qt::CaseInsensitive) ||
                                                               lfile.fileName().endsWith("mp4", Qt::CaseInsensitive))){
                    animeBuscado->nomeArquivo = lit.fileName();
                    //Compara o nome do anime e o número do episódio
                    if(fcomparaDadosAnime(animeBuscado))
                        return lfile.fileName();
                }
            }
        }
    }
    return "";
}

///anime* animebuscado, int episodioBuscado
QString arquivos::fprocuraEpisodioEspecifico(anime *ranimeBuscado, int rEpisodioBuscado){
    //Checa se o anime pode ser lido
    if(!cdatabase->instance()->fchecaDatabaseReady())
        return "";

    QPointer<ArquivoAnime> animeBuscado(new ArquivoAnime(ranimeBuscado, rEpisodioBuscado-1));
    int id = ranimeBuscado->vid.toInt();

    //Verifica se a função retorna um valor que não está vazio, ou seja
    //Se existe uma pasta com o nome do anime
    if(!cconfUsuario->instance()->fretornaDiretorioEspecifico(id).isEmpty()){
        //Começa a iterar a pasta em busca das pastas de animes
        QDirIterator lit(cconfUsuario->instance()->fretornaDiretorioEspecifico(id), QDirIterator::Subdirectories);
        while(lit.hasNext()){
            QFile lfile(lit.next());
            QFileInfo lchecaSeArquivoOuPasta(lfile.fileName());
            //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de vídeo
            if(lchecaSeArquivoOuPasta.isFile() == true && (lfile.fileName().endsWith("mkv", Qt::CaseInsensitive) ||
                                                           lfile.fileName().endsWith("mp4", Qt::CaseInsensitive))){
                animeBuscado->nomeArquivo = lit.fileName();
                //Compara o nome do anime e o número do episódio
                if(fcomparaDadosAnime(animeBuscado))
                    return lfile.fileName();
            }
        }
    }
    else{
        //Começa a iterar a pasta em busca das pastas de animes
        for(int i = 0; i < cconfUsuario->instance()->fretornaDiretoriosAnimes().size(); i++){
            QDirIterator lit(cconfUsuario->instance()->fretornaDiretoriosAnimes().at(i), QDir::Files);
            while(lit.hasNext()){
                QFile lfile(lit.next());
                QFileInfo lchecaSeArquivoOuPasta(lfile.fileName());
                //Checa se o que foi encontrado é um arquivo ou uma pasta e, no caso de ser um arquivo, se é um arquivo de vídeo
                if(lchecaSeArquivoOuPasta.isFile() == true && (lfile.fileName().endsWith("mkv", Qt::CaseInsensitive) ||
                                                               lfile.fileName().endsWith("mp4", Qt::CaseInsensitive))){
                    animeBuscado->nomeArquivo = lit.fileName();
                    //Compara o nome do anime e o número do episódio
                    if(fcomparaDadosAnime(animeBuscado))
                        return lfile.fileName();
                }
            }
        }
    }
    return "";
}

bool arquivos::fabreEpisodio(const QByteArray &rcaminhoArquivo){
    QScopedPointer<QDesktopServices> abreEpisodio(new QDesktopServices);
    if(!rcaminhoArquivo.isEmpty()){
        abreEpisodio->openUrl(QUrl("file:///"+rcaminhoArquivo,QUrl::TolerantMode));
        return true;
    }
    return false;
}

int arquivos::fcomparaSeasons(QString rnome, int repisodio, int rtemporada)
{
    int lepisodiosTotais = 0;
    //Compara o nome do anime com o nome dos outros animes na lista.
    //Se forem o mesmo anime, checo se são animes ou ovas/filmes.
    //Se forem animes, checo se a temporada é diferente da que eu busco, e caso for menor, eu só somo todos os episódios
    //Pra obter o número total de episódios
    rnome = formatador.fremoveTudo(rnome);

    if(vEpisodiosTotaisPorAnime.contains(rnome))
        return vEpisodiosTotaisPorAnime[rnome];

    QString nomeAnimeTemp;
    QPointer<anime> animeSelecionado;
    vlistaSelecionada = cdatabase->instance()->returnAnimeList("CURRENT");
    for(int i = 0; i < vlistaSelecionada.size(); i++){
        if(!cdatabase->instance()->fchecaDatabaseReady())
            return 0;
        animeSelecionado = cdatabase->instance()->fretornaAnimePorPosicao("CURRENT", i);
        nomeAnimeTemp = formatador.fremoveTudo(animeSelecionado->vnome);
        if(rnome.compare(nomeAnimeTemp) == 0 && animeSelecionado->vtemporada < rtemporada
                && animeSelecionado->vformato == "TV"){
            if(animeSelecionado->vnumEpisodiosTotais.toInt() != 0){
                if(lepisodiosTotais+animeSelecionado->vnumEpisodiosTotais.toInt() < repisodio){
                    lepisodiosTotais += animeSelecionado->vnumEpisodiosTotais.toInt();
                }
                else{
                    vEpisodiosTotaisPorAnime.insert(rnome,lepisodiosTotais);
                    return lepisodiosTotais;
                }
            }
        }
    }

    vlistaSelecionada = cdatabase->instance()->returnAnimeList("COMPLETED");
    for(int i = 0; i < vlistaSelecionada.size(); i++){
        if(!cdatabase->instance()->fchecaDatabaseReady())
            return 0;
        animeSelecionado = cdatabase->instance()->fretornaAnimePorPosicao("COMPLETED", i);
        nomeAnimeTemp = formatador.fremoveTudo(animeSelecionado->vnome);
        if(rnome.compare(nomeAnimeTemp) == 0 && animeSelecionado->vtemporada < rtemporada
                && animeSelecionado->vformato == "TV"){ //DUVIDA NISSO DA TV. SERÁ QUE OVAS CONTAM PRO INDEX DO HORRIBLE?
            if(animeSelecionado->vnumEpisodiosTotais.toInt() != 0){
                if(lepisodiosTotais+animeSelecionado->vnumEpisodiosTotais.toInt() < repisodio){
                    lepisodiosTotais += animeSelecionado->vnumEpisodiosTotais.toInt();
                }
                else{
                    vEpisodiosTotaisPorAnime.insert(rnome,lepisodiosTotais);
                    return lepisodiosTotais;
                }
            }
        }
    }

    vlistaSelecionada = cdatabase->instance()->returnAnimeList("PAUSED");
    for(int i = 0; i < vlistaSelecionada.size(); i++){
        if(!cdatabase->instance()->fchecaDatabaseReady())
            return 0;
        animeSelecionado = cdatabase->instance()->fretornaAnimePorPosicao("PAUSED", i);
        nomeAnimeTemp = formatador.fremoveTudo(animeSelecionado->vnome);
        if(rnome.compare(nomeAnimeTemp) == 0 && animeSelecionado->vtemporada < rtemporada
                && animeSelecionado->vformato == "TV"){ //DUVIDA NISSO DA TV. SERÁ QUE OVAS CONTAM PRO INDEX DO HORRIBLE?
            if(animeSelecionado->vnumEpisodiosTotais.toInt() != 0){
                if(lepisodiosTotais+animeSelecionado->vnumEpisodiosTotais.toInt() < repisodio){
                    lepisodiosTotais += animeSelecionado->vnumEpisodiosTotais.toInt();
                }
                else{
                    vEpisodiosTotaisPorAnime.insert(rnome,lepisodiosTotais);
                    return lepisodiosTotais;
                }
            }
        }
    }

    vlistaSelecionada = cdatabase->instance()->returnAnimeList("DROPPED");
    for(int i = 0; i < vlistaSelecionada.size(); i++){
        if(!cdatabase->instance()->fchecaDatabaseReady())
            return 0;
        animeSelecionado = cdatabase->instance()->fretornaAnimePorPosicao("DROPPED", i);
        nomeAnimeTemp = formatador.fremoveTudo(animeSelecionado->vnome);
        if(rnome.compare(nomeAnimeTemp) == 0 && animeSelecionado->vtemporada < rtemporada
                && animeSelecionado->vformato == "TV"){ //DUVIDA NISSO DA TV. SERÁ QUE OVAS CONTAM PRO INDEX DO HORRIBLE?
            if(animeSelecionado->vnumEpisodiosTotais.toInt() != 0){
                if(lepisodiosTotais+animeSelecionado->vnumEpisodiosTotais.toInt() < repisodio){
                    lepisodiosTotais += animeSelecionado->vnumEpisodiosTotais.toInt();
                }
                else{
                    vEpisodiosTotaisPorAnime.insert(rnome,lepisodiosTotais);
                    return lepisodiosTotais;
                }
            }
        }
    }

    vlistaSelecionada = cdatabase->instance()->returnAnimeList("PLANNING");
    for(int i = 0; i < vlistaSelecionada.size(); i++){
        if(!cdatabase->instance()->fchecaDatabaseReady())
            return 0;
        animeSelecionado = cdatabase->instance()->fretornaAnimePorPosicao("PLANNING", i);
        nomeAnimeTemp = formatador.fremoveTudo(animeSelecionado->vnome);
        if(rnome.compare(nomeAnimeTemp) == 0 && animeSelecionado->vtemporada < rtemporada
                && animeSelecionado->vformato == "TV"){ //DUVIDA NISSO DA TV. SERÁ QUE OVAS CONTAM PRO INDEX DO HORRIBLE?
            if(animeSelecionado->vnumEpisodiosTotais.toInt() != 0){
                if(lepisodiosTotais+animeSelecionado->vnumEpisodiosTotais.toInt() < repisodio){
                    lepisodiosTotais += animeSelecionado->vnumEpisodiosTotais.toInt();
                }
                else{
                    vEpisodiosTotaisPorAnime.insert(rnome,lepisodiosTotais);
                    return lepisodiosTotais;
                }
            }
        }
    }

    vEpisodiosTotaisPorAnime.insert(rnome,lepisodiosTotais);
    return lepisodiosTotais;
}
