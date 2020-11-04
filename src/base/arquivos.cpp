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
bool arquivos::fcomparaDadosAnime(QString rfileName, const QString &rnomeAnime, QString rnomeAnimeIngles, const QStringList &rnomesAlternativosAnime,
                                  int repisodioAnime, int rtemporada){
    //Anitomy é uma classe linda que separa os elementos de uma string
    anitomy::Anitomy lanitomy;
    lanitomy.Parse(rfileName.toStdWString());
    const auto& lelements = lanitomy.elements();
    //Usamos isso para pegar o número do episódio e o nome do anime a partir do nome do arquivo
    rfileName = QString::fromStdWString(lelements.get(anitomy::kElementAnimeTitle));
    QString vtemporada = QString::fromStdWString(lelements.get(anitomy::kElementAnimeSeason));
    if(!vtemporada.isEmpty())
        rfileName.append(QString(" " + vtemporada));
    int lepisodioAnime = QString::fromStdWString(lelements.get(anitomy::kElementEpisodeNumber)).toInt();

    int repisodiosTotais = 0;
    if(rtemporada != 1)
        repisodiosTotais = fcomparaSeasons(rnomeAnime,lepisodioAnime, rtemporada);
    //Alguns animes, normalmente filmes e ovas, não tem número de episódio, sendo lido como episódio 0
    //Por esse motivo, é dado o número 1 como número de episódio, assim o programa consegue reconhecer como episódio não visto
    if(lepisodioAnime == 0)
        lepisodioAnime++;

    //Episódios totais é a variável que conta todos os episódios do anime, em todas as seasons. Caso algum sub coloque, por exemplo
    //One Piece episódio 201, ele ainda vai ser lido e saberemos qual o episódio/temporada certa.
    if(formatador.fcomparaNomes(rfileName,rnomeAnime) && (lepisodioAnime == repisodioAnime+1 ||
                                                                   lepisodioAnime - repisodiosTotais == repisodioAnime+1)){
        return true;
    }
    else if(formatador.fcomparaNomes(rfileName, std::move(rnomeAnimeIngles)) && (lepisodioAnime == repisodioAnime+1 ||
            lepisodioAnime - repisodiosTotais == repisodioAnime+1)){
        return true;
    }
    else{
        for(int i = 0; i < rnomesAlternativosAnime.size(); i++){
            if(formatador.fcomparaNomes(rfileName,rnomesAlternativosAnime.at(i)) && (lepisodioAnime == repisodioAnime+1 ||
                                        lepisodioAnime - repisodiosTotais == repisodioAnime+1)){
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

    QString nome = ranimeBuscado->vnome;
    QString nomeIngles = ranimeBuscado->vnomeIngles;
    QStringList nomesAlternativos = ranimeBuscado->vnomeAlternativo;
    int numEpisodiosAssistidos = ranimeBuscado->vnumEpisodiosAssistidos.toInt();
    int temporada = ranimeBuscado->vtemporada;
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
                //Compara o nome do anime e o número do episódio
                if(fcomparaDadosAnime(lit.fileName(), nome, nomeIngles, nomesAlternativos,
                                           numEpisodiosAssistidos, temporada))
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
                    //Compara o nome do anime e o número do episódio
                    if(fcomparaDadosAnime(lit.fileName(), nome, nomeIngles, nomesAlternativos,
                                               numEpisodiosAssistidos, temporada))
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

    QString nome = ranimeBuscado->vnome;
    QString nomeIngles = ranimeBuscado->vnomeIngles;
    QStringList nomesAlternativos = ranimeBuscado->vnomeAlternativo;
    int temporada = ranimeBuscado->vtemporada;
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
                //Compara o nome do anime e o número do episódio
                if(fcomparaDadosAnime(lit.fileName(), nome, nomeIngles, nomesAlternativos,
                                           rEpisodioBuscado-1, temporada))
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
                    //Compara o nome do anime e o número do episódio
                    if(fcomparaDadosAnime(lit.fileName(), nome, nomeIngles, nomesAlternativos,
                                               rEpisodioBuscado-1, temporada))
                        return lfile.fileName();
                }
            }
        }
    }
    return "";
}

bool arquivos::fabreEpisodio(const QByteArray &rcaminhoArquivo){
    QDesktopServices *abreEpisodio = new QDesktopServices;
    if(!rcaminhoArquivo.isEmpty()){
        abreEpisodio->openUrl(QUrl("file:///"+rcaminhoArquivo,QUrl::TolerantMode));
        delete abreEpisodio;
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
