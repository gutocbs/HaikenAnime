#include "database.h"

Database *Database::createInstance()
{
    return new Database();
}

Database::Database(QObject *parent) : QObject(parent)
{
    vdatabaseReady = false;
}

Database::~Database()
{
    fsalvaIdNomeAno();
    fdeletaListaAnimes();
    qDeleteAll(vlistaAno);
}

Database *Database::instance()
{
    return  Singleton<Database>::instance(Database::createInstance);
}

bool Database::comparaNomes(QString nomeOficial, QString nomeIngles, QStringList nomesAlternativos, QString nomeBuscado)
{
    if(formatador.fcomparaNomes(nomeOficial, nomeBuscado) || formatador.fcomparaNomes(nomeIngles, nomeBuscado))
        return true;
    foreach(QString nomeAlternativo, nomesAlternativos){
        if(formatador.fcomparaNomes(nomeAlternativo, nomeBuscado))
            return true;
    }
    return false;
}

void Database::insereNomesHashNomeAnimesPorId(QString id, QStringList nomes)
{
    if(!vhashNomeAnimesPorId.contains(id))
        vhashNomeAnimesPorId.insert(id, nomes);
    else
        vhashNomeAnimesPorId[id].append(nomes);
}

bool Database::freadDatabaseFile2()
{
//    freadDatabaseFile2();
    vdatabaseReady = false;

    //Checa se arquivo pode ser lido
    QFile lleJson("Configurações/Temp/animeList.txt");
    if(lleJson.size() == 0)
        return false;

    //UTF-8 permite kanjis
    QTextStream json(&lleJson);
    json.setCodec("UTF-8");

    //Variáveis de animes
    QString lid;
    QString lnome;
    QString lnomeIngles;
    QStringList lnomeAlternativo; //StringList por que pode ter lários nomes alternativos
    QString lnumEpisodiosTotais;
    QString lvnumEpisodiosAssistidos;
    QString lnotaMediaSite;
    QString lLinkImagemMedia;
    QString lnotaMediaPessoal;
    QString lsinopse;
    QString lseason;
    QString lstatus;
    QString llista;
    QString lformato;
    QString lstream;
    QString ldatabaseSite;
    QString lproximoEpisodio;
    QString lano;
    QString lmes;
    QString lchapters;
    QString lvolumes;
    QString ldataEpisodioFinal;
    QDate ldataEpisodioConvertida;
    QTime lhoraLancamentoEpisodio;
    int ltemporadaAnime = 1; //Todas as temporadas começam como 1
    QDate ldataEstreia; //Data de estréia do próximo episódio
    QStringList tempNomeAnime;

    //Começa a ler o arquivo de database
    if(lleJson.open(QIODevice::ReadOnly)){
        while(!json.atEnd()){
            QString linha = json.readLine();
            if(linha.contains("null"))
                linha = limpaNull(linha);
            //Procura pelo primeiro dado do anime, a capa
            if(linha.contains("\"large\":"))
                lLinkImagemMedia = limpaImagem(linha);
            else if(linha.contains("\"description\":"))
                lsinopse = limpaSinopse(linha);
            else if(linha.contains("\"episodes\":"))
                lnumEpisodiosTotais = limpaEpisodiosTotais(linha);
            else if(linha.contains("\"format\":"))
                lformato = limpaFormato(linha);
            else if(linha.contains("\"id\":"))
                lid = limpaId(linha);
            else if(linha.contains("\"url\":"))
                lstream = limpaStreamUrl(linha);
            else if(linha.contains("\"chapters\":"))
                lchapters = limpaChapters(linha);
            else if(linha.contains("\"volumes\":"))
                lvolumes = limpaVolumes(linha);
            else if(linha.contains("\"siteUrl\":"))
                ldatabaseSite = limpaDatabaseUrl(linha);
            else if(linha.contains("\"nextAiringEpisode\":"))
                ldataEpisodioFinal = limpaDataProximoEpisodio(linha);
            else if(linha.contains("\"airingAt\":"))
                ldataEpisodioFinal.append(limpaAiring(linha, ldataEpisodioConvertida, lhoraLancamentoEpisodio));
            else if(linha.contains("\"episode\":"))
                lproximoEpisodio = limpaNumeroProximoEpisodio(linha);
            else if(linha.contains("\"season\":"))
                lseason = limpaSeason(linha);
            else if(linha.contains("\"month\":"))
                lmes = limpaMonth(linha);
            else if(linha.contains("\"year\":")){
                lano = limpaYear(linha, lseason);
                ldataEstreia = setDataEstreia(lano, lmes, lseason);
            }
            else if(linha.contains("\"status\":"))
                lstatus = limpaStatus(linha, ldataEpisodioFinal, ldataEpisodioConvertida, lhoraLancamentoEpisodio);
            else if(linha.contains("\"synonyms\":")){
                linha = json.readLine();
                while(!linha.contains("],")){
                    linha = limpaNomesAlternativos(linha);
                    lnomeAlternativo.append(linha);
                    linha = json.readLine();
                }

            }
            else if(linha.contains("\"english\":"))
                lnomeIngles = limpaNomeIngles(linha);
            else if(linha.contains("\"romaji\":"))
                lnome = limpaNomeRomaji(linha, ltemporadaAnime);
            else if(linha.contains("\"progress\":"))
                lvnumEpisodiosAssistidos = limpaProgresso(linha);
            else if(linha.contains("\"averageScore\":"))
                lnotaMediaSite = limpaScoreGeral(linha);
            else if(linha.contains("\"score\":")){
                lnotaMediaPessoal = limpaScoreMedio(linha);
                linha = json.readLine();
                llista = limpaLista(linha);
                if(lnome == "?")
                    lnome = lnomeIngles;
                else if (lnomeIngles == "?")
                    lnomeIngles = lnome;

                QPointer<anime> lnovoAnime(new anime);

                lnovoAnime->vnome = lnome;
                lnovoAnime->vnomeIngles = lnomeIngles;
                if(!lnomeAlternativo.isEmpty()){
                    if(vlistaNomesAlternativos.contains(lid))
                        lnomeAlternativo.append(vlistaNomesAlternativos[lid]);
                    lnovoAnime->vnomeAlternativo = lnomeAlternativo;
                    lnomeAlternativo.empty();
                }
                lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
                if(!lnotaMediaSite.isEmpty())
                    lnovoAnime->vnotaMediaSite = lnotaMediaSite;
                else
                    lnovoAnime->vnotaMediaSite = "-";

                lnovoAnime->vLinkImagemMedia = lLinkImagemMedia;
                lnovoAnime->vnotaMediaPessoal = lnotaMediaPessoal;
                lnovoAnime->vstatus = lstatus;
                lnovoAnime->vsinopse = lsinopse;
                lnovoAnime->vid = lid;
                lnovoAnime->vseason = lseason + " " + lano;
                lnovoAnime->vnumProximoEpisodioLancado = lproximoEpisodio;
                lnovoAnime->vnumEpisodiosAssistidos = lvnumEpisodiosAssistidos;
                lnovoAnime->vformato = lformato;
                lnovoAnime->vdataEstreia = ldataEstreia;
                lnovoAnime->vdataEpisodio = ldataEpisodioFinal;
                lnovoAnime->vdatabaseSite = ldatabaseSite;
                lnovoAnime->vstream = lstream;
                lnovoAnime->vtemporada = ltemporadaAnime;

                vidEntrada.append(lid.toInt());

                tempNomeAnime.clear();

                tempNomeAnime.append(lnovoAnime->vnome);
                tempNomeAnime.append(lnovoAnime->vnomeIngles);
                tempNomeAnime.append(lnovoAnime->vnomeAlternativo);

                if(lformato != "MANGA" && lformato != "NOVEL" && lformato != "ONE SHOT"){
                    if(llista == "CURRENT"){
                        lnovoAnime->vlista = "Watching";
                        vlistaAnimeWatching.append(lnovoAnime);
                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "COMPLETED"){
                        lnovoAnime->vlista = "Completed";
                        vlistaAnimeCompleted.append(lnovoAnime);
                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "PLANNING"){
                        lnovoAnime->vlista = "Plan to Watch";
                        vlistaAnimePlanToWatch.append(lnovoAnime);
                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "DROPPED"){
                        lnovoAnime->vlista = "Dropped";
                        vlistaAnimeDropped.append(lnovoAnime);
                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "PAUSED"){
                        lnovoAnime->vlista = "On Hold";
                        vlistaAnimeOnHold.append(lnovoAnime);
                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    sAnimeAdicionadoNaLista(lnovoAnime->vid);
                    if(!vhashListaAnimesPorId.contains(lnovoAnime->vid))
                        vhashListaAnimesPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
                    else if(vhashListaAnimesPorId[lnovoAnime->vid] != lnovoAnime->vlista)
                        vhashListaAnimesPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
                }
                else if(lformato == "MANGA" || lformato == "ONE SHOT"){
                    if(lnovoAnime->vstatus.contains("Air", Qt::CaseInsensitive))
                        lnovoAnime->vstatus.replace("Air","Releas");
                    lnovoAnime->vnumEpisodiosTotais = lchapters;
                    if(llista == "CURRENT"){
                        lnovoAnime->vlista = "Reading(Manga)";
                        vlistaMangaReading.append(lnovoAnime);
                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "COMPLETED"){
                        lnovoAnime->vlista = "Completed(Manga)";
                        vlistaMangaCompleted.append(lnovoAnime);
                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "PLANNING"){
                        lnovoAnime->vlista = "Plan to Read(Manga)";
                        vlistaMangaPlanToRead.append(lnovoAnime);
                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "DROPPED"){
                        lnovoAnime->vlista = "Dropped(Manga)";
                        vlistaMangaDropped.append(lnovoAnime);
                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "PAUSED"){
                        lnovoAnime->vlista = "On Hold(Manga)";
                        vlistaMangaOnHold.append(lnovoAnime);
                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    sMangaAdicionadoNaLista(lnovoAnime->vid);
                    if(!vhashListaMangasPorId.contains(lnovoAnime->vid))
                        vhashListaMangasPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
                }
                else if(lformato == "NOVEL"){
                    lnovoAnime->vnumEpisodiosTotais = lvolumes;
                    if(llista == "CURRENT"){
                        lnovoAnime->vlista = "Reading(Novel)";
                        vlistaNovelReading.append(lnovoAnime);
                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "COMPLETED"){
                        lnovoAnime->vlista = "Completed(Novel)";
                        vlistaNovelCompleted.append(lnovoAnime);
                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "PLANNING"){
                        lnovoAnime->vlista = "Plan to Read(Novel)";
                        vlistaNovelPlanToRead.append(lnovoAnime);
                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "DROPPED"){
                        lnovoAnime->vlista = "Dropped(Novel)";
                        vlistaNovelDropped.append(lnovoAnime);
                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    else if(llista == "PAUSED"){
                        lnovoAnime->vlista = "On Hold(Novel)";
                        vlistaNovelOnHold.append(lnovoAnime);
                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
                    }
                    sNovelAdicionadoNaLista(lnovoAnime->vid);
                    if(!vhashListaNovelsPorId.contains(lnovoAnime->vid))
                        vhashListaNovelsPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
                }
                lnome.clear();
                lnomeIngles.clear();
                lnomeAlternativo.clear();
                lnumEpisodiosTotais.clear();
                lnotaMediaSite.clear();
                lLinkImagemMedia.clear();
                lnotaMediaPessoal.clear();
                lstatus.clear();
                lsinopse.clear();
                lid.clear();
                lseason.clear();
                lproximoEpisodio.clear();
                lvnumEpisodiosAssistidos.clear();
                lformato.clear();
                lformato.clear();
                lstream.clear();
                ldatabaseSite.clear();
                ldataEpisodioFinal.clear();
                ldataEpisodioConvertida = QDate();
                lhoraLancamentoEpisodio = QTime();
            }
        }
        lleJson.close();
    }
    else
        return false;

    vdatabaseReady = true;
    return true;
}

bool Database::freadDatabaseFile()
{
    vdatabaseReady = false;
    QVector<int> idUpdate;

    //Checa se arquivo pode ser lido
    QFile listAnimesArquivo("Configurações/Temp/animeList.txt");
    if(listAnimesArquivo.size() == 0)
        return false;
    if(!listAnimesArquivo.open(QIODevice::ReadOnly))
        return false;

    //Começa a ler o arquivo de database
    QString data = listAnimesArquivo.readAll();
    listAnimesArquivo.close();

    data.replace("{\n    \"data\": {\n        \"Page\": {\n            \"mediaList\": [\n                {\n                    ","");
    data.replace("]            }        }    }","");
    data.replace("},                {","");
    QStringList animeListData = data.split("\"media\": {");
    animeListData.removeFirst();

    QString idAnime;
    foreach(QString singleAnimeData, animeListData){
        QStringList infoAnime = limpaStringList(singleAnimeData);

        foreach(QString data, infoAnime){
            if(data.contains("\"id\"")){
                idAnime = data.remove("\"id\":").remove(",").simplified();
                break;
            }
        }

        idUpdate.append(idAnime.toInt());
        updateAnimeInfo(infoAnime,idAnime);
    }

//    foreach(int id, vidEntrada){
//        if(!idUpdate.contains(id))
//            fdeletedaLista(QString::number(id));
//    }

    vdatabaseReady = true;
    return true;
}


QStringList Database::limpaStringList(QString singleAnimeData)
{
    QStringList lista = singleAnimeData.split(QRegExp("\\\n\\s*"));
    lista.removeDuplicates();
    lista.removeOne("");
    lista.removeOne("\"coverImage\": {");
    lista.removeOne("},");
    lista.removeOne("\"startDate\": {");
    lista.removeOne("{");
    lista.removeOne("}");
    lista.removeOne("],");
    lista.removeOne("\"streamingEpisodes\": [\",");
    lista.removeOne("\"site\": \"Crunchyroll\",");
    lista.removeOne("\"title\": {");

    return lista;
}

bool Database::updateAnimeInfo(QStringList singleAnimeData, QString idAnime)
{
    if(vidEntrada.contains(idAnime.toInt())){
        if(vhashListaAnimesPorId.contains(idAnime))
            updateAnime(singleAnimeData, idAnime);
        else if(vhashListaMangasPorId.contains(idAnime))
            updateManga(singleAnimeData, idAnime);
        else if(vhashListaNovelsPorId.contains(idAnime))
            updateNovel(singleAnimeData, idAnime);
    }
    else
        insereAnimeLista(singleAnimeData, idAnime);
    return true;
}

bool Database::insereAnimeLista(QStringList singleAnimeData, QString idAnime)
{
    QString notaMediaSite;
    QString season;
    QString status;
    QString ano;
    QString mes;
    QString chapters;
    QString volumes;
    QString dataEpisodioFinal;
    QDate dataEpisodioConvertida;
    QTime horaLancamentoEpisodio;
    int temporadaAnime = 1; //Todas as temporadas começam como 1
    QStringList todosOsNomes;

    anime* lnovoAnime = new anime(this);
    lnovoAnime->vid = idAnime;
    foreach(QString data, singleAnimeData){
        if(data.contains("\"large\":"))
            lnovoAnime->vLinkImagemMedia = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\\\"*")).trimmed();
        else if(data.contains("\"description\":"))
            lnovoAnime->vsinopse = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).remove(QRegExp("(\\\\r|\\\\n|\\|<br>)*")).remove("\\n").remove("\\").trimmed();
        else if(data.contains("\"episodes\":"))
            lnovoAnime->vnumEpisodiosTotais = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"format\":"))
            lnovoAnime->vformato = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"url\":"))
            lnovoAnime->vstream = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).left(data.lastIndexOf("episode-")).trimmed();
        else if(data.contains("\"chapters\":"))
            chapters = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"volumes\":"))
            volumes = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"siteUrl\":"))
            lnovoAnime->vdatabaseSite = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"nextAiringEpisode\":")){
            lnovoAnime->vdataEpisodio = data.remove(QRegExp("\\{?\\\"\\w*\\\"\\:\\s\\\"?\\{?")).remove(QRegExp("\"?,$")).trimmed();
            if(lnovoAnime->vdataEpisodio.compare("null") == 0)
                lnovoAnime->vdataEpisodio = '-';
        }
        else if(data.contains("\"airingAt\":"))
            lnovoAnime->vdataEpisodio.append(limpaAiring(data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed(),
                                                 dataEpisodioConvertida, horaLancamentoEpisodio));
        else if(data.contains("\"episode\":"))
            lnovoAnime->vnumProximoEpisodioLancado = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"chapters\":"))
            chapters = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"volumes\":"))
            volumes = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"season\":"))
            lnovoAnime->vSeasonSemAno = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"month\":"))
            mes = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"year\":")){
            ano = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
            lnovoAnime->vdataEstreia = setDataEstreia(ano, mes, season);
        }
        else if(data.contains("\"status\":")){
            if(lnovoAnime->vnotaMediaPessoal.compare("") == 0)
                lnovoAnime->vstatus = limpaStatus(data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed(),
                                              dataEpisodioFinal, dataEpisodioConvertida, horaLancamentoEpisodio);
            else
                lnovoAnime->vlista = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\\\"*")).trimmed();
        }
        else if(data.contains("\"english\":"))
            lnovoAnime->vnomeIngles = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"romaji\":")){
            lnovoAnime->vnome = limpaNomeRomaji(data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed(), temporadaAnime);
            if(lnovoAnime->vnome == "?")
                lnovoAnime->vnome = lnovoAnime->vnomeIngles;
            else if (lnovoAnime->vnomeIngles == "?")
                lnovoAnime->vnomeIngles = lnovoAnime->vnome;
        }
        else if(data.contains("\"progress\":"))
            lnovoAnime->vnumEpisodiosAssistidos = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"averageScore\":")){
            notaMediaSite = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
            if(!notaMediaSite.isEmpty())
                lnovoAnime->vnotaMediaSite = notaMediaSite;
            else
                lnovoAnime->vnotaMediaSite = "-";
        }
        else if(data.contains("\"score\""))
            lnovoAnime->vnotaMediaPessoal = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(!data.contains("\"id\"") && !data.contains("\"synonyms\"") && !data.contains("\"streamingEpisodes\"")){ //Nomes alternativos
            lnovoAnime->vnomeAlternativo.append(data.remove(QRegExp("\"?,$")).trimmed());
        }
    }
    lnovoAnime->vseason = lnovoAnime->vSeasonSemAno + " " + ano;
    lnovoAnime->vtemporada = temporadaAnime;

    todosOsNomes.clear();
    todosOsNomes.append(lnovoAnime->vnome);
    todosOsNomes.append(lnovoAnime->vnomeIngles);
    todosOsNomes.append(lnovoAnime->vnomeAlternativo);

    if(lnovoAnime->vformato != "MANGA" && lnovoAnime->vformato != "NOVEL" && lnovoAnime->vformato != "ONE SHOT" && lnovoAnime->vformato != "ONE_SHOT"){
        if(lnovoAnime->vlista == "CURRENT"){
            lnovoAnime->vlista = "Watching";
            vlistaAnimeWatching.append(lnovoAnime);
            finsereDataHashId("anime", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "COMPLETED"){
            lnovoAnime->vlista = "Completed";
            vlistaAnimeCompleted.append(lnovoAnime);
            finsereDataHashId("anime", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "PLANNING"){
            lnovoAnime->vlista = "Plan to Watch";
            vlistaAnimePlanToWatch.append(lnovoAnime);
            finsereDataHashId("anime", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "DROPPED"){
            lnovoAnime->vlista = "Dropped";
            vlistaAnimeDropped.append(lnovoAnime);
            finsereDataHashId("anime", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "PAUSED"){
            lnovoAnime->vlista = "On Hold";
            vlistaAnimeOnHold.append(lnovoAnime);
            finsereDataHashId("anime", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        sAnimeAdicionadoNaLista(lnovoAnime->vid);
        if(!vhashListaAnimesPorId.contains(lnovoAnime->vid))
            vhashListaAnimesPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
        else if(vhashListaAnimesPorId[lnovoAnime->vid] != lnovoAnime->vlista)
            vhashListaAnimesPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
    }
    else if(lnovoAnime->vformato == "MANGA" || lnovoAnime->vformato == "ONE SHOT" || lnovoAnime->vformato == "ONE_SHOT"){
        if(lnovoAnime->vstatus.contains("Air", Qt::CaseInsensitive))
            lnovoAnime->vstatus.replace("Air","Releas");
        lnovoAnime->vnumEpisodiosTotais = chapters;
        if(lnovoAnime->vlista == "CURRENT"){
            lnovoAnime->vlista = "Reading(Manga)";
            vlistaMangaReading.append(lnovoAnime);
            finsereDataHashId("manga", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "COMPLETED"){
            lnovoAnime->vlista = "Completed(Manga)";
            vlistaMangaCompleted.append(lnovoAnime);
            finsereDataHashId("manga", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "PLANNING"){
            lnovoAnime->vlista = "Plan to Read(Manga)";
            vlistaMangaPlanToRead.append(lnovoAnime);
            finsereDataHashId("manga", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "DROPPED"){
            lnovoAnime->vlista = "Dropped(Manga)";
            vlistaMangaDropped.append(lnovoAnime);
            finsereDataHashId("manga", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "PAUSED"){
            lnovoAnime->vlista = "On Hold(Manga)";
            vlistaMangaOnHold.append(lnovoAnime);
            finsereDataHashId("manga", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        sMangaAdicionadoNaLista(lnovoAnime->vid);
        if(!vhashListaMangasPorId.contains(lnovoAnime->vid))
            vhashListaMangasPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
    }
    else if(lnovoAnime->vformato == "NOVEL"){
        lnovoAnime->vnumEpisodiosTotais = volumes;
        if(lnovoAnime->vlista == "CURRENT"){
            lnovoAnime->vlista = "Reading(Novel)";
            vlistaNovelReading.append(lnovoAnime);
            finsereDataHashId("novel", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "COMPLETED"){
            lnovoAnime->vlista = "Completed(Novel)";
            vlistaNovelCompleted.append(lnovoAnime);
            finsereDataHashId("novel", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "PLANNING"){
            lnovoAnime->vlista = "Plan to Read(Novel)";
            vlistaNovelPlanToRead.append(lnovoAnime);
            finsereDataHashId("novel", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "DROPPED"){
            lnovoAnime->vlista = "Dropped(Novel)";
            vlistaNovelDropped.append(lnovoAnime);
            finsereDataHashId("novel", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        else if(lnovoAnime->vlista == "PAUSED"){
            lnovoAnime->vlista = "On Hold(Novel)";
            vlistaNovelOnHold.append(lnovoAnime);
            finsereDataHashId("novel", lnovoAnime->vlista, lnovoAnime->vid, todosOsNomes);
        }
        sNovelAdicionadoNaLista(lnovoAnime->vid);
        if(!vhashListaNovelsPorId.contains(lnovoAnime->vid))
            vhashListaNovelsPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
    }
    vidEntrada.append(lnovoAnime->vid.toInt());
    return true;
}

bool Database::updateAnime(QStringList singleAnimeData, QString idAnime)
{
    QString season;
    QString nota;
    QString progresso;
    QString status;
    QString notaMedia;
    QString airAt;
    QString episodios;
    foreach(QString data, singleAnimeData){
        if(data.contains("\"season\""))
            season = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"status\""))
            status = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"score\""))
            nota = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"progress\""))
            progresso = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"averageScore\""))
            notaMedia = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"airingAt\""))
            airAt = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"episodes\""))
            episodios = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
    }
    QString llista = fbuscaIDRetornaLista(idAnime);
    int lposicao = fbuscaIDRetornaPosicao(idAnime);
    if(lposicao == -1)
        return false;


    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Watching") != 0)
            fmudaLista(idAnime, status, type::ANIME);
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Completed") != 0)
            fmudaLista(idAnime, status, type::ANIME);
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("On Hold") != 0)
            fmudaLista(idAnime, status, type::ANIME);
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Dropped") != 0)
            fmudaLista(idAnime, status, type::ANIME);
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Plan to Watch") != 0)
            fmudaLista(idAnime, status, type::ANIME);
    }

    return true;
}

bool Database::updateManga(QStringList singleAnimeData, QString idAnime)
{
    QString season;
    QString nota;
    QString progresso;
    QString status;
    QString notaMedia;
    QString airAt;
    QString episodios;
    foreach(QString data, singleAnimeData){
        if(data.contains("\"season\""))
            season = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"status\""))
            status = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"score\""))
            nota = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"progress\""))
            progresso = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"averageScore\""))
            notaMedia = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"airingAt\""))
            airAt = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"episodes\""))
            episodios = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
    }
    QString llista = fbuscaIDRetornaLista(idAnime);
    int lposicao = fbuscaIDRetornaPosicao(idAnime);
    if(lposicao == -1)
        return false;

    /////////////////////////Manga
    else if(llista.compare("Reading(Manga)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Reading(Manga)") != 0)
            fmudaLista(idAnime, status, type::MANGA);
    }
    else if(llista.compare("Completed(Manga)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Completed(Manga)") != 0)
            fmudaLista(idAnime, status, type::MANGA);
    }
    else if(llista.compare("Dropped(Manga)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Dropped(Manga)") != 0)
            fmudaLista(idAnime, status, type::MANGA);
    }
    else if(llista.compare("On Hold(Manga)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("On Hold(Manga)") != 0)
            fmudaLista(idAnime, status, type::MANGA);
    }
    else if(llista.compare("Plan to Read(Manga)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Plan to Read(Manga)") != 0)
            fmudaLista(idAnime, status, type::MANGA);
    }

    return true;
}

bool Database::updateNovel(QStringList singleAnimeData, QString idAnime)
{
    QString season;
    QString nota;
    QString progresso;
    QString status;
    QString notaMedia;
    QString airAt;
    QString episodios;
    foreach(QString data, singleAnimeData){
        if(data.contains("\"season\""))
            season = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"status\""))
            status = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"score\""))
            nota = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"progress\""))
            progresso = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"averageScore\""))
            notaMedia = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"airingAt\""))
            airAt = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
        else if(data.contains("\"episodes\""))
            episodios = data.remove(QRegExp("\"\\w*\":\\s\"?")).remove(QRegExp("\"?,$")).trimmed();
    }
    QString llista = fbuscaIDRetornaLista(idAnime);
    int lposicao = fbuscaIDRetornaPosicao(idAnime);
    if(lposicao == -1)
        return false;


    /////////////////////////NOVEL
    else if(llista.compare("Reading(Novel)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Reading(Novel)") != 0)
            fmudaLista(idAnime, status, type::NOVEL);
    }
    else if(llista.compare("Completed(Novel)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Completed(Novel)") != 0)
            fmudaLista(idAnime, status, type::NOVEL);
    }
    else if(llista.compare("Dropped(Novel)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Dropped(Novel)") != 0)
            fmudaLista(idAnime, status, type::NOVEL);
    }
    else if(llista.compare("On Hold(Novel)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("On Hold(Novel)") != 0)
            fmudaLista(idAnime, status, type::NOVEL);
    }
    else if(llista.compare("Plan to Read(Novel)", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching[lposicao]->vseason.compare(season) != 0)
            vlistaAnimeWatching[lposicao]->vseason = season;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaPessoal.compare(nota) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = nota;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos.compare(progresso) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = progresso;
        if(vlistaAnimeWatching[lposicao]->vnotaMediaSite.compare(notaMedia) != 0)
            vlistaAnimeWatching[lposicao]->vnotaMediaSite = notaMedia;
        if(vlistaAnimeWatching[lposicao]->vdataEpisodio.compare(airAt) != 0)
            vlistaAnimeWatching[lposicao]->vdataEpisodio = airAt;
        if(vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais.compare(episodios) != 0)
            vlistaAnimeWatching[lposicao]->vnumEpisodiosTotais = episodios;
        status = updateStatus(status);
        if(status.compare("Plan to Read(Novel)") != 0)
            fmudaLista(idAnime, status, type::NOVEL);
    }
    return true;
}


QString Database::updateStatus(QString statusVelho)
{
    if(statusVelho.compare("CURRENT") == 0)
        return "Watching";
    else if(statusVelho.compare("COMPLETED") == 0)
        return "Completed";
    else if(statusVelho.compare("PAUSED") == 0)
        return "On Hold";
    else if(statusVelho.compare("DROPPED") == 0)
        return "Dropped";
    else if(statusVelho.compare("PLANNING") == 0)
        return "Plan to Watch";
    return "";
}

bool Database::fchecaDatabaseReady()
{
    return vdatabaseReady;
}

QVector<anime *> Database::returnAnimeYearlyList(int ano)
{
    if(!vlistaAnimeSeasonWinter.isEmpty())
        vlistaAnimeSeasonWinter.clear();
    if(!vlistaAnimeSeasonFall.isEmpty())
       vlistaAnimeSeasonFall.clear();
    if(!vlistaAnimeSeasonSummer.isEmpty())
       vlistaAnimeSeasonSummer.clear();
    if(!vlistaAnimeSeasonSpring.isEmpty())
        vlistaAnimeSeasonSpring.clear();
    if(!vlistaAno.isEmpty())
       vlistaAno.clear();
    QFile lleJson("Configurações/Temp/Lists/animeList"+QString::number(ano)+".txt");
    if(lleJson.size() == 0)
        return vlistaAno;
    if(!vhashSizeListasPorAno.contains(ano))
        vhashSizeListasPorAno.insert(ano, QString::number(lleJson.size()));

    //UTF-8 permite kanjis
    QTextStream json(&lleJson);
    json.setCodec("UTF-8");

    //Variáveis de animes
    QString lid;
    QString lnome;
    QString lnomeIngles;
    QStringList lnomeAlternativo; //StringList por que pode ter lários nomes alternativos
    QString lnumEpisodiosTotais;
    QString lvnumEpisodiosAssistidos;
    QString lnotaMediaSite;
    QString lLinkImagemMedia;
    QString lnotaMediaPessoal;
    QString lsinopse;
    QString lseason;
    QString lstatus;
    QString llista;
    QString lformato;
    QString lstream;
    QString ldatabaseSite;
    QString lproximoEpisodio;
    QString lano;
    QString lmes;
    QString lchapters;
    QString lvolumes;
    QString ldataEpisodioFinal;
    QDate ldataEpisodioConvertida;
    QTime lhoraLancamentoEpisodio;
    int ltemporadaAnime = 1; //Todas as temporadas começam como 1
    QDate ldataEstreia; //Data de estréia do próximo episódio
    QStringList tempNomeAnime;

    //Começa a ler o arquivo de database
    if(lleJson.open(QIODevice::ReadOnly)){
        while(!json.atEnd()){
            QString linha = json.readLine();
            if(linha.contains("null"))
                linha = limpaNull(linha);
            //Procura pelo primeiro dado do anime, a capa
            if(linha.contains("\"large\":"))
                lLinkImagemMedia = limpaImagem(linha);
            else if(linha.contains("\"description\":"))
                lsinopse = limpaSinopse(linha);
            else if(linha.contains("\"episodes\":"))
                lnumEpisodiosTotais = limpaEpisodiosTotais(linha);
            else if(linha.contains("\"format\":"))
                lformato = limpaFormato(linha);
            else if(linha.contains("\"id\":"))
                lid = limpaId(linha);
            else if(linha.contains("\"url\":"))
                lstream = limpaStreamUrl(linha);
            else if(linha.contains("\"chapters\":"))
                lchapters = limpaChapters(linha);
            else if(linha.contains("\"volumes\":"))
                lvolumes = limpaVolumes(linha);
            else if(linha.contains("\"siteUrl\":"))
                ldatabaseSite = limpaDatabaseUrl(linha);
            else if(linha.contains("\"nextAiringEpisode\":"))
                ldataEpisodioFinal = limpaDataProximoEpisodio(linha);
            else if(linha.contains("\"airingAt\":"))
                ldataEpisodioFinal.append(limpaAiring(linha, ldataEpisodioConvertida, lhoraLancamentoEpisodio));
            else if(linha.contains("\"episode\":"))
                lproximoEpisodio = limpaNumeroProximoEpisodio(linha);
            else if(linha.contains("\"season\":"))
                lseason = limpaSeason(linha);
            else if(linha.contains("\"month\":"))
                lmes = limpaMonth(linha);
            else if(linha.contains("\"year\":")){
                lano = limpaYear(linha, lseason);
                ldataEstreia = setDataEstreia(lano, lmes, lseason);
            }
            else if(linha.contains("\"status\":"))
                lstatus = limpaStatus(linha, ldataEpisodioFinal, ldataEpisodioConvertida, lhoraLancamentoEpisodio);
            else if(linha.contains("\"synonyms\":")){
                linha = json.readLine();
                while(!linha.contains("],")){
                    linha = limpaNomesAlternativos(linha);
                    lnomeAlternativo.append(linha);
                    linha = json.readLine();
                }

            }
            else if(linha.contains("\"english\":"))
                lnomeIngles = limpaNomeIngles(linha);
            else if(linha.contains("\"progress\":"))
                lvnumEpisodiosAssistidos = limpaProgresso(linha);
            else if(linha.contains("\"averageScore\":"))
                lnotaMediaSite = limpaScoreGeral(linha);
            else if(linha.contains("\"romaji\":")){
                lnome = limpaNomeRomaji(linha, ltemporadaAnime);
                linha = json.readLine();
                llista = limpaLista(linha);
                if(lnome == "?")
                    lnome = lnomeIngles;
                else if (lnomeIngles == "?")
                    lnomeIngles = lnome;
                lnotaMediaPessoal = "";

                QPointer<anime> lnovoAnime(new anime);

                lnovoAnime->vnome = lnome;
                lnovoAnime->vnomeIngles = lnomeIngles;
                if(!lnomeAlternativo.isEmpty()){
                    if(vlistaNomesAlternativos.contains(lid))
                        lnomeAlternativo.append(vlistaNomesAlternativos[lid]);
                    lnovoAnime->vnomeAlternativo = lnomeAlternativo;
                    lnomeAlternativo.empty();
                }
                lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
                if(!lnotaMediaSite.isEmpty())
                    lnovoAnime->vnotaMediaSite = lnotaMediaSite;
                else
                    lnovoAnime->vnotaMediaSite = "-";

                lnovoAnime->vLinkImagemMedia = lLinkImagemMedia;
                lnovoAnime->vnotaMediaPessoal = lnotaMediaPessoal;
                lnovoAnime->vstatus = lstatus;
                lnovoAnime->vsinopse = lsinopse;
                lnovoAnime->vid = lid;
                lnovoAnime->vseason = lseason + " " + lano;
                lnovoAnime->vnumProximoEpisodioLancado = lproximoEpisodio;
                lnovoAnime->vnumEpisodiosAssistidos = lvnumEpisodiosAssistidos;
                lnovoAnime->vformato = lformato;
                lnovoAnime->vdataEstreia = ldataEstreia;
                lnovoAnime->vdataEpisodio = ldataEpisodioFinal;
                lnovoAnime->vdatabaseSite = ldatabaseSite;
                lnovoAnime->vstream = lstream;
                lnovoAnime->vtemporada = ltemporadaAnime;
                tempNomeAnime.clear();

                tempNomeAnime.append(lnovoAnime->vnome);
                tempNomeAnime.append(lnovoAnime->vnomeIngles);
                tempNomeAnime.append(lnovoAnime->vnomeAlternativo);

                if(ldataEstreia.year() == ano){
                    vlistaAno.append(lnovoAnime);
                    if(lseason.compare("Winter") == 0)
                        vlistaAnimeSeasonWinter.append(lnovoAnime);
                    else if(lseason.compare("Spring") == 0)
                        vlistaAnimeSeasonSpring.append(lnovoAnime);
                    else if(lseason.compare("Summer") == 0)
                        vlistaAnimeSeasonSummer.append(lnovoAnime);
                    else
                        vlistaAnimeSeasonFall.append(lnovoAnime);
                    sAnimeAdicionadoNaLista(lid);
                }
                QStringList tempNomeAnime;
                if(!vhashNomeAnimesPorId.contains(lnovoAnime->vid)){
                    tempNomeAnime.append(lnovoAnime->vnome);
                    tempNomeAnime.append(lnovoAnime->vnomeIngles);
                    tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                    vhashNomeAnimesPorId.insert(lnovoAnime->vid, tempNomeAnime);
                }
                if(!vhashPosicaoAnimesPorId.contains(lnovoAnime->vid))
                    vhashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaAno.size()-1);
                if(!vhashListaAnimesPorId.contains(lnovoAnime->vid))
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, QString::number(ano));

                lnome.clear();
                lnomeIngles.clear();
                lnomeAlternativo.clear();
                lnumEpisodiosTotais.clear();
                lnotaMediaSite.clear();
                lLinkImagemMedia.clear();
                lnotaMediaPessoal.clear();
                lstatus.clear();
                lsinopse.clear();
                lid.clear();
                lseason.clear();
                lproximoEpisodio.clear();
                lvnumEpisodiosAssistidos.clear();
                lformato.clear();
                lformato.clear();
                lstream.clear();
                ldatabaseSite.clear();
                ldataEpisodioFinal.clear();
                ldataEpisodioConvertida = QDate();
                lhoraLancamentoEpisodio = QTime();
            }
        }
        lleJson.close();
    }
    return vlistaAno;
}

anime *Database::fbuscaAnimeNoAno(int ano, const QString &rid)
{
    QFile lleJson("Configurações/Temp/Lists/animeList"+QString::number(ano)+".txt");
    if(lleJson.size() == 0)
        return new anime;
    if(!vhashSizeListasPorAno.contains(ano))
        vhashSizeListasPorAno.insert(ano, QString::number(lleJson.size()));

    //UTF-8 permite kanjis
    QTextStream json(&lleJson);
    json.setCodec("UTF-8");

    //Variáveis de animes
    QString lid;
    QString lnome;
    QString lnomeIngles;
    QStringList lnomeAlternativo; //StringList por que pode ter lários nomes alternativos
    QString lnumEpisodiosTotais;
    QString lvnumEpisodiosAssistidos;
    QString lnotaMediaSite;
    QString lLinkImagemMedia;
    QString lnotaMediaPessoal;
    QString lsinopse;
    QString lseason;
    QString lstatus;
    QString llista;
    QString lformato;
    QString lstream;
    QString ldatabaseSite;
    QString lproximoEpisodio;
    QString lano;
    QString lmes;
    QString lchapters;
    QString lvolumes;
    QString ldataEpisodioFinal;
    QDate ldataEpisodioConvertida;
    QTime lhoraLancamentoEpisodio;
    int ltemporadaAnime = 1; //Todas as temporadas começam como 1
    QDate ldataEstreia; //Data de estréia do próximo episódio
    QStringList tempNomeAnime;
    bool encontrou = false;

    //Começa a ler o arquivo de database
    if(lleJson.open(QIODevice::ReadOnly)){
        while(!json.atEnd()){
            QString linha = json.readLine();
            if(linha.contains("null"))
                linha = limpaNull(linha);
            //Procura pelo primeiro dado do anime, a capa
            if(linha.contains("\"large\":"))
                lLinkImagemMedia = limpaImagem(linha);
            else if(linha.contains("\"description\":"))
                lsinopse = limpaSinopse(linha);
            else if(linha.contains("\"episodes\":"))
                lnumEpisodiosTotais = limpaEpisodiosTotais(linha);
            else if(linha.contains("\"format\":"))
                lformato = limpaFormato(linha);
            else if(linha.contains("\"id\":")){
                lid = limpaId(linha);
                if(lid == rid)
                    encontrou = true;
            }
            if(encontrou){
                if(linha.contains("\"url\":"))
                    lstream = limpaStreamUrl(linha);
                else if(linha.contains("\"chapters\":"))
                    lchapters = limpaChapters(linha);
                else if(linha.contains("\"volumes\":"))
                    lvolumes = limpaVolumes(linha);
                else if(linha.contains("\"siteUrl\":"))
                    ldatabaseSite = limpaDatabaseUrl(linha);
                else if(linha.contains("\"nextAiringEpisode\":"))
                    ldataEpisodioFinal = limpaDataProximoEpisodio(linha);
                else if(linha.contains("\"airingAt\":"))
                    ldataEpisodioFinal.append(limpaAiring(linha, ldataEpisodioConvertida, lhoraLancamentoEpisodio));
                else if(linha.contains("\"episode\":"))
                    lproximoEpisodio = limpaNumeroProximoEpisodio(linha);
                else if(linha.contains("\"season\":"))
                    lseason = limpaSeason(linha);
                else if(linha.contains("\"month\":"))
                    lmes = limpaMonth(linha);
                else if(linha.contains("\"year\":")){
                    lano = limpaYear(linha, lseason);
                    ldataEstreia = setDataEstreia(lano, lmes, lseason);
                }
                else if(linha.contains("\"status\":"))
                    lstatus = limpaStatus(linha, ldataEpisodioFinal, ldataEpisodioConvertida, lhoraLancamentoEpisodio);
                else if(linha.contains("\"synonyms\":")){
                    linha = json.readLine();
                    while(!linha.contains("],")){
                        linha = limpaNomesAlternativos(linha);
                        lnomeAlternativo.append(linha);
                        linha = json.readLine();
                    }

                }
                else if(linha.contains("\"english\":"))
                    lnomeIngles = limpaNomeIngles(linha);

                else if(linha.contains("\"progress\":"))
                    lvnumEpisodiosAssistidos = limpaProgresso(linha);
                else if(linha.contains("\"averageScore\":"))
                    lnotaMediaSite = limpaScoreGeral(linha);
                else if(linha.contains("\"romaji\":")){
                    lnome = limpaNomeRomaji(linha, ltemporadaAnime);
                    linha = json.readLine();
                    llista = limpaLista(linha);
                    if(lnome == "?")
                        lnome = lnomeIngles;
                    else if (lnomeIngles == "?")
                        lnomeIngles = lnome;


                    QPointer<anime> lnovoAnime(new anime);

                    //Por que esse retorno caso seja vazio?
                    if(fbuscaIDRetornaLista(lid).isEmpty())
                        lnovoAnime->vlista = fbuscaIDRetornaLista(lid)+" "+ lseason;
                    else
                        lnovoAnime->vlista = fbuscaIDRetornaLista(lid);
                    lnovoAnime->vnome = lnome;
                    lnovoAnime->vnomeIngles = lnomeIngles;
                    if(!lnomeAlternativo.isEmpty()){
                        if(vlistaNomesAlternativos.contains(lid))
                            lnomeAlternativo.append(vlistaNomesAlternativos[lid]);
                        lnovoAnime->vnomeAlternativo = lnomeAlternativo;
                        lnomeAlternativo.empty();
                    }
                    lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
                    if(!lnotaMediaSite.isEmpty())
                        lnovoAnime->vnotaMediaSite = lnotaMediaSite;
                    else
                        lnovoAnime->vnotaMediaSite = "-";

                    lnovoAnime->vLinkImagemMedia = lLinkImagemMedia;
                    lnovoAnime->vnotaMediaPessoal = lnotaMediaPessoal;
                    lnovoAnime->vstatus = lstatus;
                    lnovoAnime->vsinopse = lsinopse;
                    lnovoAnime->vid = lid;
                    lnovoAnime->vseason = lseason + " " + lano;
                    lnovoAnime->vnumProximoEpisodioLancado = lproximoEpisodio;
                    lnovoAnime->vnumEpisodiosAssistidos = lvnumEpisodiosAssistidos;
                    lnovoAnime->vformato = lformato;
                    lnovoAnime->vdataEstreia = ldataEstreia;
                    lnovoAnime->vdataEpisodio = ldataEpisodioFinal;
                    lnovoAnime->vdatabaseSite = ldatabaseSite;
                    lnovoAnime->vstream = lstream;
                    lnovoAnime->vtemporada = ltemporadaAnime;
                    return lnovoAnime;
                }
            }
        }
        lleJson.close();
    }
    return new anime;
}

anime *Database::fretornaAnimePorID(const QString &rid)
{
    if(!vdatabaseReady)
        return new anime;
    QString llista = fbuscaIDRetornaLista(rid);
    int lposicao = fbuscaIDRetornaPosicao(rid);
    if(lposicao != -1){
        if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
            return vlistaAnimeWatching[lposicao];
        }
        else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
            return vlistaAnimePlanToWatch[lposicao];
        }
        else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
            return vlistaAnimeOnHold[lposicao];
        }
        else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
            return vlistaAnimeDropped[lposicao];
        }
        else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
            return vlistaAnimeCompleted[lposicao];
        }
        else{
            bool ok;
            //Checa se a lista é um número válido
            llista.toInt(&ok);
            if(ok){
                return fbuscaAnimeNoAno(llista.toInt(), rid);
            }
        }
    }
    return new anime;
}

anime *Database::fretornaAnimePorPosicao(const QString &lista, int posicao)
{
    if(!vdatabaseReady)
        return new anime;
    if(lista.compare("CURRENT", Qt::CaseInsensitive) == 0 && vlistaAnimeWatching.size() > posicao)
        return vlistaAnimeWatching[posicao];
    else if(lista.compare("PLANNING", Qt::CaseInsensitive) == 0 && vlistaAnimePlanToWatch.size() > posicao)
        return vlistaAnimePlanToWatch[posicao];
    else if(lista.compare("PAUSED", Qt::CaseInsensitive) == 0 && vlistaAnimeOnHold.size() > posicao)
        return vlistaAnimeOnHold[posicao];
    else if(lista.compare("DROPPED", Qt::CaseInsensitive) == 0 && vlistaAnimeDropped.size() > posicao)
        return vlistaAnimeDropped[posicao];
    else if(lista.compare("COMPLETED", Qt::CaseInsensitive) == 0 && vlistaAnimeCompleted.size() > posicao)
        return vlistaAnimeCompleted[posicao];
    else if(lista.compare("SEARCH", Qt::CaseInsensitive) == 0 && vlistaAnimeCompleted.size() > posicao)
        return vlistaBusca[posicao];
    else{
        bool ok;
        //Checa se a lista é um número válido
        lista.toInt(&ok);
        if(ok && vlistaAno.size() > posicao){
            return vlistaAno[posicao];
        }
    }

    return new anime;
}

QHash<QString, int> Database::retornaHash(Database::tipoHash tipoHash, Database::type tipoMidia, int valorInt)
{
    if(!vdatabaseReady)
        return QHash<QString,int>();
    if(tipoHash == POSICAO){
        if(tipoMidia == ANIME)
            return vhashPosicaoAnimesPorId;
        else if(tipoMidia == MANGA)
            return vhashPosicaoMangasPorId;
        else if(tipoMidia == NOVEL)
            return vhashPosicaoNovelsPorId;
    }
    return QHash<QString,int>();
}

QHash<QString, QString> Database::retornaHash(Database::tipoHash tipoHash, Database::type tipoMidia, QString valorQString)
{
    if(!vdatabaseReady)
        return QHash<QString,QString>();
    if(tipoHash == LISTA){
        if(tipoMidia == ANIME)
            return vhashListaAnimesPorId;
        else if(tipoMidia == MANGA)
            return vhashListaMangasPorId;
        else if(tipoMidia == NOVEL)
            return vhashListaNovelsPorId;
    }
    return QHash<QString,QString>();
}

QHash<QString, QStringList> Database::retornaHash(Database::tipoHash tipoHash, Database::type tipoMidia, QStringList valorQStringList)
{
    if(!vdatabaseReady)
        return QHash<QString, QStringList>();
    if(tipoHash == NOME){
        if(tipoMidia == ANIME)
            return vhashNomeAnimesPorId;
        else if(tipoMidia == MANGA)
            return vhashNomeMangasPorId;
        else if(tipoMidia == NOVEL)
            return vhashNomeNovelsPorId;
    }
    return vlistaNomesAlternativos;
}

bool Database::fmudaLista(const QString &rid, const QString &rlista, Database::type rtipo)
{
    QString llista = fbuscaIDRetornaLista(rid);
        int lposicao = fbuscaIDRetornaPosicao(rid);
        if(lposicao == -1)
            return false;
        bool ok;
        //Checa se a lista é um número válido
        llista.toInt(&ok);
        if(rtipo == type::ANIME){
            if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaAnimeWatching[lposicao];
                if(rlista.compare("Completed") == 0){
                    lnovoAnime->vlista = "Completed";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                    vlistaAnimeCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeCompleted.size()-1);
                }
                else if(rlista.compare("Dropped") == 0){
                    lnovoAnime->vlista = "Dropped";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                    vlistaAnimeDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeDropped.size()-1);
                }
                else if(rlista.compare("On Hold") == 0){
                    lnovoAnime->vlista = "On Hold";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                    vlistaAnimeOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Watch") == 0){
                    lnovoAnime->vlista = "Plan to Watch";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                    vlistaAnimePlanToWatch.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimePlanToWatch.size()-1);
                }
                vlistaAnimeWatching.remove(lposicao);
                return true;
            }
            else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaAnimeCompleted[lposicao];
                if(rlista.compare("Watching") == 0){
                    lnovoAnime->vlista = "Watching";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Watching");
                    vlistaAnimeWatching.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeWatching.size()-1);
                }
                else if(rlista.compare("Dropped") == 0){
                    lnovoAnime->vlista = "Dropped";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                    vlistaAnimeDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeDropped.size()-1);
                }
                else if(rlista.compare("On Hold") == 0){
                    lnovoAnime->vlista = "On Hold";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                    vlistaAnimeOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Watch") == 0){
                    lnovoAnime->vlista = "Plan to Watch";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                    vlistaAnimePlanToWatch.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimePlanToWatch.size()-1);
                }
                vlistaAnimeCompleted.remove(lposicao);
                return true;
            }
            else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaAnimeOnHold[lposicao];
                if(rlista.compare("Watching") == 0){
                    lnovoAnime->vlista = "Watching";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Watching");
                    vlistaAnimeWatching.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeWatching.size()-1);
                }
                else if(rlista.compare("Completed") == 0){
                    lnovoAnime->vlista = "Completed";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                    vlistaAnimeCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeCompleted.size()-1);
                }
                else if(rlista.compare("Dropped") == 0){
                    lnovoAnime->vlista = "Dropped";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                    vlistaAnimeDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeDropped.size()-1);
                }
                else if(rlista.compare("Plan to Watch") == 0){
                    lnovoAnime->vlista = "Plan to Watch";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                    vlistaAnimePlanToWatch.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimePlanToWatch.size()-1);
                }
                vlistaAnimeOnHold.remove(lposicao);
                return true;
            }
            else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaAnimeDropped[lposicao];
                if(rlista.compare("Watching") == 0){
                    lnovoAnime->vlista = "Watching";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Watching");
                    vlistaAnimeWatching.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeWatching.size()-1);
                }
                else if(rlista.compare("Completed") == 0){
                    lnovoAnime->vlista = "Completed";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                    vlistaAnimeCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeCompleted.size()-1);
                }
                else if(rlista.compare("On Hold") == 0){
                    lnovoAnime->vlista = "On Hold";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                    vlistaAnimeOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Watch") == 0){
                    lnovoAnime->vlista = "Plan to Watch";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                    vlistaAnimePlanToWatch.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimePlanToWatch.size()-1);
                }
                vlistaAnimeDropped.remove(lposicao);
                return true;
            }
            else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaAnimePlanToWatch[lposicao];
                if(rlista.compare("Watching") == 0){
                    lnovoAnime->vlista = "Watching";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Watching");
                    vlistaAnimeWatching.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeWatching.size()-1);
                }
                else if(rlista.compare("Completed") == 0){
                    lnovoAnime->vlista = "Completed";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                    vlistaAnimeCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeCompleted.size()-1);
                }
                else if(rlista.compare("On Hold") == 0){
                    lnovoAnime->vlista = "On Hold";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                    vlistaAnimeOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeOnHold.size()-1);
                }
                else if(rlista.compare("Dropped") == 0){
                    lnovoAnime->vlista = "Dropped";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                    vlistaAnimeDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeDropped.size()-1);
                }
                vlistaAnimePlanToWatch.remove(lposicao);
                return true;
            }
            //oK, no caso, pega listas por ano
            else if(ok){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = fbuscaAnimeNoAno(llista.toInt(), rid);
                if(rlista.compare("Watching") == 0){
                    lnovoAnime->vlista = "Watching";
                    vhashListaAnimesPorId.insert(rid, "Watching");
                    vlistaAnimeWatching.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeWatching.size()-1);
                }
                else if(rlista.compare("Completed") == 0){
                    lnovoAnime->vlista = "Completed";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                    vlistaAnimeCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeCompleted.size()-1);
                }
                else if(rlista.compare("Dropped") == 0){
                    lnovoAnime->vlista = "Dropped";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                    vlistaAnimeDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeDropped.size()-1);
                }
                else if(rlista.compare("On Hold") == 0){
                    lnovoAnime->vlista = "On Hold";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                    vlistaAnimeOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimeOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Watch") == 0){
                    lnovoAnime->vlista = "Plan to Watch";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                    vlistaAnimePlanToWatch.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaAnimePlanToWatch.size()-1);
                }
                return true;
            }
        }
        else if(rtipo == type::NOVEL){
            if(llista.compare("Reading(Novel)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaNovelReading[lposicao];
                if(rlista.compare("Completed(Novel)") == 0){
                    lnovoAnime->vlista = "Completed(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Novel)");
                    vlistaNovelCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelCompleted.size()-1);
                }
                else if(rlista.compare("Dropped(Novel)") == 0){
                    lnovoAnime->vlista = "Dropped(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Novel)");
                    vlistaNovelDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelDropped.size()-1);
                }
                else if(rlista.compare("On Hold(Novel)") == 0){
                    lnovoAnime->vlista = "On Hold(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Novel)");
                    vlistaNovelOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Read(Novel)") == 0){
                    lnovoAnime->vlista = "Plan to Read(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Novel)");
                    vlistaNovelPlanToRead.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelPlanToRead.size()-1);
                }
                vlistaNovelReading.remove(lposicao);
                return true;
            }
            else if(llista.compare("Completed(Novel)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaNovelCompleted[lposicao];
                if(rlista.compare("Reading(Novel)") == 0){
                    lnovoAnime->vlista = "Reading(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Novel)");
                    vlistaNovelReading.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelReading.size()-1);
                }
                else if(rlista.compare("Dropped(Novel)") == 0){
                    lnovoAnime->vlista = "Dropped(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Novel)");
                    vlistaNovelDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelDropped.size()-1);
                }
                else if(rlista.compare("On Hold(Novel)") == 0){
                    lnovoAnime->vlista = "On Hold(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Novel)");
                    vlistaNovelOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Read(Novel)") == 0){
                    lnovoAnime->vlista = "Plan to Read(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Novel)");
                    vlistaNovelPlanToRead.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelPlanToRead.size()-1);
                }
                vlistaNovelCompleted.remove(lposicao);
                return true;
            }
            else if(llista.compare("On Hold(Novel)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaNovelOnHold[lposicao];
                if(rlista.compare("Reading(Novel)") == 0){
                    lnovoAnime->vlista = "Reading(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Novel)");
                    vlistaNovelReading.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelReading.size()-1);
                }
                else if(rlista.compare("Completed(Novel)") == 0){
                    lnovoAnime->vlista = "Completed(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Novel)");
                    vlistaNovelCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelCompleted.size()-1);
                }
                else if(rlista.compare("Dropped(Novel)") == 0){
                    lnovoAnime->vlista = "Dropped(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Novel)");
                    vlistaNovelDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelDropped.size()-1);
                }
                else if(rlista.compare("Plan to Read(Novel)") == 0){
                    lnovoAnime->vlista = "Plan to Read(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Novel)");
                    vlistaNovelPlanToRead.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelPlanToRead.size()-1);
                }
                vlistaNovelOnHold.remove(lposicao);
                return true;
            }
            else if(llista.compare("Dropped(Novel)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaNovelDropped[lposicao];
                if(rlista.compare("Reading(Novel)") == 0){
                    lnovoAnime->vlista = "Reading(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Novel)");
                    vlistaNovelReading.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelReading.size()-1);
                }
                else if(rlista.compare("Completed(Novel)") == 0){
                    lnovoAnime->vlista = "Completed(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Novel)");
                    vlistaNovelCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelCompleted.size()-1);
                }
                else if(rlista.compare("On Hold(Novel)") == 0){
                    lnovoAnime->vlista = "On Hold(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Novel)");
                    vlistaNovelOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Read(Novel)") == 0){
                    lnovoAnime->vlista = "Plan to Read(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Novel)");
                    vlistaNovelPlanToRead.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelPlanToRead.size()-1);
                }
                vlistaNovelDropped.remove(lposicao);
                return true;
            }
            else if(llista.compare("Plan to Read(Novel)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaNovelPlanToRead[lposicao];
                if(rlista.compare("Reading(Novel)") == 0){
                    lnovoAnime->vlista = "Reading(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Novel)");
                    vlistaNovelReading.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelReading.size()-1);
                }
                else if(rlista.compare("Completed(Novel)") == 0){
                    lnovoAnime->vlista = "Completed(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Novel)");
                    vlistaNovelCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelCompleted.size()-1);
                }
                else if(rlista.compare("On Hold(Novel)") == 0){
                    lnovoAnime->vlista = "On Hold(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Novel)");
                    vlistaNovelOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelOnHold.size()-1);
                }
                else if(rlista.compare("Dropped(Novel)") == 0){
                    lnovoAnime->vlista = "Dropped(Novel)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Novel)");
                    vlistaNovelDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaNovelDropped.size()-1);
                }
                vlistaNovelPlanToRead.remove(lposicao);
                return true;
            }
        }
        else if(rtipo == type::MANGA){
            if(llista.compare("Reading(Manga)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaMangaReading[lposicao];
                if(rlista.compare("Completed(Manga)") == 0){
                    lnovoAnime->vlista = "Completed(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Manga)");
                    vlistaMangaCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaCompleted.size()-1);
                }
                else if(rlista.compare("Dropped(Manga)") == 0){
                    lnovoAnime->vlista = "Dropped(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Manga)");
                    vlistaMangaDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaDropped.size()-1);
                }
                else if(rlista.compare("On Hold(Manga)") == 0){
                    lnovoAnime->vlista = "On Hold(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Manga)");
                    vlistaMangaOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Read(Manga)") == 0){
                    lnovoAnime->vlista = "Plan to Read(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Manga)");
                    vlistaMangaPlanToRead.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaPlanToRead.size()-1);
                }
                vlistaMangaReading.remove(lposicao);
                return true;
            }
            else if(llista.compare("Completed(Manga)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaMangaCompleted[lposicao];
                if(rlista.compare("Reading(Manga)") == 0){
                    lnovoAnime->vlista = "Reading(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Manga)");
                    vlistaMangaReading.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaReading.size()-1);
                }
                else if(rlista.compare("Dropped(Manga)") == 0){
                    lnovoAnime->vlista = "Dropped(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Manga)");
                    vlistaMangaDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaDropped.size()-1);
                }
                else if(rlista.compare("On Hold(Manga)") == 0){
                    lnovoAnime->vlista = "On Hold(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Manga)");
                    vlistaMangaOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Read(Manga)") == 0){
                    lnovoAnime->vlista = "Plan to Read(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Manga)");
                    vlistaMangaPlanToRead.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaPlanToRead.size()-1);
                }
                vlistaMangaCompleted.remove(lposicao);
                return true;
            }
            else if(llista.compare("On Hold(Manga)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaMangaOnHold[lposicao];
                if(rlista.compare("Reading(Manga)") == 0){
                    lnovoAnime->vlista = "Reading(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Manga)");
                    vlistaMangaReading.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaReading.size()-1);
                }
                else if(rlista.compare("Completed(Manga)") == 0){
                    lnovoAnime->vlista = "Completed(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Manga)");
                    vlistaMangaCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaCompleted.size()-1);
                }
                else if(rlista.compare("Dropped(Manga)") == 0){
                    lnovoAnime->vlista = "Dropped(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Manga)");
                    vlistaMangaDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaDropped.size()-1);
                }
                else if(rlista.compare("Plan to Read(Manga)") == 0){
                    lnovoAnime->vlista = "Plan to Read(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Manga)");
                    vlistaMangaPlanToRead.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaPlanToRead.size()-1);
                }
                vlistaMangaOnHold.remove(lposicao);
                return true;
            }
            else if(llista.compare("Dropped(Manga)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaMangaDropped[lposicao];
                if(rlista.compare("Reading(Manga)") == 0){
                    lnovoAnime->vlista = "Reading(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Manga)");
                    vlistaMangaReading.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaReading.size()-1);
                }
                else if(rlista.compare("Completed(Manga)") == 0){
                    lnovoAnime->vlista = "Completed(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Manga)");
                    vlistaMangaCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaCompleted.size()-1);
                }
                else if(rlista.compare("On Hold(Manga)") == 0){
                    lnovoAnime->vlista = "On Hold(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Manga)");
                    vlistaMangaOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaOnHold.size()-1);
                }
                else if(rlista.compare("Plan to Read(Manga)") == 0){
                    lnovoAnime->vlista = "Plan to Read(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Manga)");
                    vlistaMangaPlanToRead.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaPlanToRead.size()-1);
                }
                vlistaMangaDropped.remove(lposicao);
                return true;
            }
            else if(llista.compare("Plan to Read(Manga)", Qt::CaseInsensitive) == 0){
                QPointer<anime> lnovoAnime(new anime);
                lnovoAnime = vlistaMangaPlanToRead[lposicao];
                if(rlista.compare("Reading(Manga)") == 0){
                    lnovoAnime->vlista = "Reading(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Manga)");
                    vlistaMangaReading.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaReading.size()-1);
                }
                else if(rlista.compare("Completed(Manga)") == 0){
                    lnovoAnime->vlista = "Completed(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Manga)");
                    vlistaMangaCompleted.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaCompleted.size()-1);
                }
                else if(rlista.compare("On Hold(Manga)") == 0){
                    lnovoAnime->vlista = "On Hold(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Manga)");
                    vlistaMangaOnHold.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaOnHold.size()-1);
                }
                else if(rlista.compare("Dropped(Manga)") == 0){
                    lnovoAnime->vlista = "Dropped(Manga)";
                    vhashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Manga)");
                    vlistaMangaDropped.append(lnovoAnime);
                    vhashPosicaoAnimesPorId.insert(rid,vlistaMangaDropped.size()-1);
                }
                vlistaMangaPlanToRead.remove(lposicao);
                return true;
            }
        }
        return false;
}

bool Database::fmudaNota(const QString &rid, const QString &rnota)
{
    QString llista = fbuscaIDRetornaLista(rid);
    int lposicao = fbuscaIDRetornaPosicao(rid);
    if(lposicao == -1)
        return false;
    bool ok;
    //Checa se a lista é um número válido
    llista.toInt(&ok);

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        vlistaAnimeWatching[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        vlistaAnimeCompleted[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        vlistaAnimeOnHold[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        vlistaAnimeDropped[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        vlistaAnimePlanToWatch[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(ok){
        QPointer<anime> lnovoAnime(new anime);
        lnovoAnime = fbuscaAnimeNoAno(llista.toInt(), rid);
        lnovoAnime->vnotaMediaPessoal = rnota;
        return true;
    }
    return false;
}

bool Database::fmudaProgresso(const QString &rid, const QString &rprogresso)
{
    QString llista = fbuscaIDRetornaLista(rid);
    int lposicao = fbuscaIDRetornaPosicao(rid);
    if(lposicao == -1)
        return false;
    bool ok;
    //Checa se a lista é um número válido
    llista.toInt(&ok);

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0 && lposicao){
        vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0 && lposicao <= vlistaAnimeCompleted.size()-1){
        vlistaAnimeCompleted[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0 && lposicao <= vlistaAnimeOnHold.size()-1){
        vlistaAnimeOnHold[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0 && lposicao <= vlistaAnimeDropped.size()-1){
        vlistaAnimeDropped[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0 && lposicao <= vlistaAnimePlanToWatch.size()-1){
        vlistaAnimePlanToWatch[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(ok){
        QPointer<anime> lnovoAnime(new anime);
        lnovoAnime = fbuscaAnimeNoAno(llista.toInt(), rid);
        lnovoAnime->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    return false;
}

bool Database::fdeletedaLista(const QString &rid)
{
    QString llista = fbuscaIDRetornaLista(rid);
        int lposicao = fbuscaIDRetornaPosicao(rid);
        if(lposicao == -1)
            return false;

    if(llista == "Watching"){
        vhashListaAnimesPorId.insert(rid, QString::number(vlistaAnimeWatching[lposicao]->vdataEstreia.year()));
        vhashPosicaoAnimesPorId.insert(rid,0);
        vlistaAnimeWatching.remove(lposicao);
        return true;
    }
    else if(llista == "Completed"){
        vhashListaAnimesPorId.insert(rid, QString::number(vlistaAnimeCompleted[lposicao]->vdataEstreia.year()));
        vhashPosicaoAnimesPorId.insert(rid,0);
        vlistaAnimeCompleted.remove(lposicao);
        return true;
    }
    else if(llista == "On Hold"){
        vhashListaAnimesPorId.insert(rid, QString::number(vlistaAnimeOnHold[lposicao]->vdataEstreia.year()));
        vhashPosicaoAnimesPorId.insert(rid,0);
        vlistaAnimeOnHold.remove(lposicao);
        return true;
    }
    else if(llista == "Dropped"){
        vhashListaAnimesPorId.insert(rid, QString::number(vlistaAnimeDropped[lposicao]->vdataEstreia.year()));
        vhashPosicaoAnimesPorId.insert(rid,0);
        vlistaAnimeDropped.remove(lposicao);
        return true;
    }
    else if(llista == "Plan to Watch"){
        vhashListaAnimesPorId.insert(rid, QString::number(vlistaAnimePlanToWatch[lposicao]->vdataEstreia.year()));
        vhashPosicaoAnimesPorId.insert(rid,0);
        vlistaAnimePlanToWatch.remove(lposicao);
        return true;
    }
    else if(llista == "Reading(Manga)"){
        vhashListaMangasPorId.insert(rid, QString::number(vlistaMangaReading[lposicao]->vdataEstreia.year()));
        vhashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaReading.remove(lposicao);
        return true;
    }
    else if(llista == "Completed(Manga)"){
        vhashListaMangasPorId.insert(rid, QString::number(vlistaMangaCompleted[lposicao]->vdataEstreia.year()));
        vhashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaCompleted.remove(lposicao);
        return true;
    }
    else if(llista == "On Hold(Manga)"){
        vhashListaMangasPorId.insert(rid, QString::number(vlistaMangaOnHold[lposicao]->vdataEstreia.year()));
        vhashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaOnHold.remove(lposicao);
        return true;
    }
    else if(llista == "Dropped(Manga)"){
        vhashListaMangasPorId.insert(rid, QString::number(vlistaMangaDropped[lposicao]->vdataEstreia.year()));
        vhashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaDropped.remove(lposicao);
        return true;
    }
    else if(llista == "Plan to Read(Manga)"){
        vhashListaMangasPorId.insert(rid, QString::number(vlistaMangaPlanToRead[lposicao]->vdataEstreia.year()));
        vhashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaPlanToRead.remove(lposicao);
        return true;
    }
    else if(llista == "Reading(Novel)"){
        vhashListaNovelsPorId.insert(rid, QString::number(vlistaNovelReading[lposicao]->vdataEstreia.year()));
        vhashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelReading.remove(lposicao);
        return true;
    }
    else if(llista == "Completed(Novel)"){
        vhashListaNovelsPorId.insert(rid, QString::number(vlistaNovelCompleted[lposicao]->vdataEstreia.year()));
        vhashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelCompleted.remove(lposicao);
        return true;
    }
    else if(llista == "On Hold(Novel)"){
        vhashListaNovelsPorId.insert(rid, QString::number(vlistaNovelOnHold[lposicao]->vdataEstreia.year()));
        vhashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelOnHold.remove(lposicao);
        return true;
    }
    else if(llista == "Dropped(Novel)"){
        vhashListaNovelsPorId.insert(rid, QString::number(vlistaNovelDropped[lposicao]->vdataEstreia.year()));
        vhashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelDropped.remove(lposicao);
        return true;
    }
    else if(llista == "Plan to Read(Novel)"){
        vhashListaNovelsPorId.insert(rid, QString::number(vlistaNovelPlanToRead[lposicao]->vdataEstreia.year()));
        vhashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelPlanToRead.remove(lposicao);
        return true;
    }
    return false;

}

bool Database::finsereNomeAlternativo(const QString &rid, const QStringList &rnome)
{
    QString llista = fbuscaIDRetornaLista(rid);
    int lposicao = fbuscaIDRetornaPosicao(rid);
    if(lposicao == -1)
        return false;

    bool returnIsTrue = false;
    QStringList tempAnimeList;
    if(vlistaNomesAlternativos.contains(rid))
        tempAnimeList = vlistaNomesAlternativos[rid];
    if(!tempAnimeList.contains(rnome.first()))
        tempAnimeList.append(rnome);

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        vlistaAnimeWatching[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        vlistaAnimePlanToWatch[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        vlistaAnimeOnHold[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        vlistaAnimeDropped[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        vlistaAnimeCompleted[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    if(llista.compare("Reading(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaReading[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Plan to Read(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaPlanToRead[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("On Hold(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaOnHold[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Dropped(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaDropped[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Completed(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaCompleted[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Reading(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelReading[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Plan to Read(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelPlanToRead[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("On Hold(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelOnHold[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Dropped(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelDropped[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }
    else if(llista.compare("Completed(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelCompleted[lposicao]->vnomeAlternativo.append(rnome);
        vhashNomeAnimesPorId[rid].append(rnome);
        returnIsTrue = true;
    }

    if(returnIsTrue){
        if(!vlistaNomesAlternativos.contains(rid))
            vlistaNomesAlternativos.insert(rid, tempAnimeList);
        else
            vlistaNomesAlternativos[rid].append(tempAnimeList);
        return true;
    }

    return false;
}

void Database::fdeletaListaAnimes()
{
    vdatabaseReady = false;
    if(!vlistaAnimeWatching.isEmpty()){
        qDeleteAll(vlistaAnimeWatching.begin(),vlistaAnimeWatching.end()); //Deletar todas as listas
        vlistaAnimeWatching.clear();
    }
    if(!vlistaAnimeCompleted.isEmpty()){
        qDeleteAll(vlistaAnimeCompleted.begin(),vlistaAnimeCompleted.end()); //Deletar todas as listas
        vlistaAnimeCompleted.clear();
    }
    if(!vlistaAnimeDropped.isEmpty()){
        qDeleteAll(vlistaAnimeDropped.begin(),vlistaAnimeDropped.end()); //Deletar todas as listas
        vlistaAnimeDropped.clear();
    }
    if(!vlistaAnimeOnHold.isEmpty()){
        qDeleteAll(vlistaAnimeOnHold.begin(),vlistaAnimeOnHold.end()); //Deletar todas as listas
        vlistaAnimeOnHold.clear();
    }
    if(!vlistaAnimePlanToWatch.isEmpty()){
        qDeleteAll(vlistaAnimePlanToWatch.begin(),vlistaAnimePlanToWatch.end()); //Deletar todas as listas
        vlistaAnimePlanToWatch.clear();
    }
    if(!vlistaMangaReading.isEmpty()){
        qDeleteAll(vlistaMangaReading.begin(),vlistaMangaReading.end()); //Deletar todas as listas
        vlistaMangaReading.clear();
    }
    if(!vlistaMangaCompleted.isEmpty()){
        qDeleteAll(vlistaMangaCompleted.begin(),vlistaMangaCompleted.end()); //Deletar todas as listas
        vlistaMangaCompleted.clear();
    }
    if(!vlistaMangaDropped.isEmpty()){
        qDeleteAll(vlistaMangaDropped.begin(),vlistaMangaDropped.end()); //Deletar todas as listas
        vlistaMangaDropped.clear();
    }
    if(!vlistaMangaOnHold.isEmpty()){
        qDeleteAll(vlistaMangaOnHold.begin(),vlistaMangaOnHold.end()); //Deletar todas as listas
        vlistaMangaOnHold.clear();
    }
    if(!vlistaMangaPlanToRead.isEmpty()){
        qDeleteAll(vlistaMangaPlanToRead.begin(),vlistaMangaPlanToRead.end()); //Deletar todas as listas
        vlistaMangaPlanToRead.clear();
    }
    if(!vlistaNovelReading.isEmpty()){
        qDeleteAll(vlistaNovelReading.begin(),vlistaNovelReading.end()); //Deletar todas as listas
        vlistaNovelReading.clear();
    }
    if(!vlistaNovelCompleted.isEmpty()){
        qDeleteAll(vlistaNovelCompleted.begin(),vlistaNovelCompleted.end()); //Deletar todas as listas
        vlistaNovelCompleted.clear();
    }
    if(!vlistaNovelDropped.isEmpty()){
        qDeleteAll(vlistaNovelDropped.begin(),vlistaNovelDropped.end()); //Deletar todas as listas
        vlistaNovelDropped.clear();
    }
    if(!vlistaNovelOnHold.isEmpty()){
        qDeleteAll(vlistaNovelOnHold.begin(),vlistaNovelOnHold.end()); //Deletar todas as listas
        vlistaNovelOnHold.clear();
    }
    if(!vlistaNovelPlanToRead.isEmpty()){
        qDeleteAll(vlistaNovelPlanToRead.begin(),vlistaNovelPlanToRead.end()); //Deletar todas as listas
        vlistaNovelPlanToRead.clear();
    }
}

//Para não ter que ler as listas de cada ano várias vezes, salvo as informações principais neste arquivo para pegar os dados rapidamente
//com uma hash
void Database::fcarregaIdNomeAno()
{
    QFile larquivo("Configurações/idListaNome.txt");
    if(larquivo.size() != 0){
        if(larquivo.open(QIODevice::ReadOnly)){
            while(!larquivo.atEnd()){
                QString lstreamTexto = larquivo.readLine();
                QStringList lidListaNome = lstreamTexto.split(";");
                lidListaNome.last() = lidListaNome.last().trimmed();
                QString id = lidListaNome.takeFirst();
                QString lista = lidListaNome.takeFirst();
                QString posicaoLista = lidListaNome.takeFirst();
                vhashListaAnimesPorId.insert(id,lista);
                vhashPosicaoAnimesPorId.insert(id,posicaoLista.toInt());
                vhashNomeAnimesPorId.insert(id,lidListaNome);
            }
            larquivo.close();
        }
    }
    QFile larquivoManga("Configurações/idListaNomeManga.txt");
    if(larquivoManga.size() != 0){
        if(larquivoManga.open(QIODevice::ReadOnly)){
            while(!larquivoManga.atEnd()){
                QString lstreamTexto = larquivoManga.readLine();
                QStringList lidListaNome = lstreamTexto.split(";");
                lidListaNome.last() = lidListaNome.last().trimmed();
                QString id = lidListaNome.takeFirst();
                QString lista = lidListaNome.takeFirst();
                QString posicaoLista = lidListaNome.takeFirst();
                vhashListaMangasPorId.insert(id,lista);
                vhashPosicaoMangasPorId.insert(id,posicaoLista.toInt());
                vhashNomeMangasPorId.insert(id,lidListaNome);
            }
            larquivoManga.close();
        }
    }
    QFile larquivoNovel("Configurações/idListaNomeNovel.txt");
    if(larquivoNovel.size() != 0){
        if(larquivoNovel.open(QIODevice::ReadOnly)){
            while(!larquivoNovel.atEnd()){
                QString lstreamTexto = larquivoNovel.readLine();
                QStringList lidListaNome = lstreamTexto.split(";");
                lidListaNome.last() = lidListaNome.last().trimmed();
                QString id = lidListaNome.takeFirst();
                QString lista = lidListaNome.takeFirst();
                QString posicaoLista = lidListaNome.takeFirst();
                vhashListaNovelsPorId.insert(id,lista);
                vhashPosicaoNovelsPorId.insert(id,posicaoLista.toInt());
                vhashNomeNovelsPorId.insert(id,lidListaNome);
            }
            larquivoNovel.close();
        }
    }
    QFile larquivoSizePorAno("Configurações/Temp/sizePerYear.txt");
    if(larquivoSizePorAno.size() != 0){
        if(larquivoSizePorAno.open(QIODevice::ReadOnly)){
            while(!larquivoSizePorAno.atEnd()){
                QString lstreamTexto = larquivoSizePorAno.readLine();
                QStringList lidAnoSize = lstreamTexto.split(";");
                lidAnoSize.last() = lidAnoSize.last().trimmed();
                QString ano = lidAnoSize.takeFirst();
                QString size = lidAnoSize.takeFirst();
                vhashSizeListasPorAno.insert(ano.toInt(),size);
            }
            larquivoSizePorAno.close();
        }
    }

}

void Database::fsalvaIdNomeAno()
{
    QFile larquivo("Configurações/idListaNome.txt");
    if(larquivo.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivo);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vhashNomeAnimesPorId.keys()){
            lstreamTexto << key << ";" << vhashListaAnimesPorId[key] << ";" << vhashPosicaoAnimesPorId[key] << ";" <<
                            vhashNomeAnimesPorId[key].join(";").trimmed() << Qt::endl;
        }
        larquivo.close();
    }
    QFile larquivoManga("Configurações/idListaNomeManga.txt");
    if(larquivoManga.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivoManga);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vhashNomeMangasPorId.keys()){
            lstreamTexto << key << ";" << vhashListaMangasPorId[key] << ";" << vhashPosicaoMangasPorId[key] << ";" <<
                            vhashNomeMangasPorId[key].join(";").trimmed() << Qt::endl;
        }
        larquivoManga.close();
    }
    QFile larquivoNovel("Configurações/idListaNomeNovel.txt");
    if(larquivoNovel.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivoNovel);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vhashNomeNovelsPorId.keys()){
            lstreamTexto << key << ";" << vhashListaNovelsPorId[key] << ";" << vhashPosicaoNovelsPorId[key] << ";" <<
                            vhashNomeNovelsPorId[key].join(";").trimmed() << Qt::endl;
        }
        larquivoNovel.close();
    }
    QFile larquivoSizePorAno("Configurações/Temp/sizePerYear.txt");
    if(larquivoSizePorAno.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivoSizePorAno);
        lstreamTexto.setCodec("UTF-8");
        foreach(int key, vhashSizeListasPorAno.keys()){
            lstreamTexto << QString::number(key) << ";" << vhashSizeListasPorAno[key] << Qt::endl;
        }
        larquivoSizePorAno.close();
    }

}

void Database::fsalvaListaNomesAlternativos()
{
    QFile larquivo("Configurações/nomesAlternativosAnimes.txt");
    if(larquivo.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivo);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vlistaNomesAlternativos.keys()){
            lstreamTexto << key << ";" << vlistaNomesAlternativos[key].join(";").trimmed() << Qt::endl;
        }
        larquivo.close();
    }
}

void Database::fleListaNomesAlternativos()
{
    QFile larquivo("Configurações/nomesAlternativosAnimes.txt");
    if(larquivo.size() == 0)
        return;
    if(larquivo.open(QIODevice::ReadOnly)){
        while(!larquivo.atEnd()){
            QString lstreamTexto = larquivo.readLine();
            QStringList lnomesAlternativos = lstreamTexto.split(";");
            lnomesAlternativos.last() = lnomesAlternativos.last().trimmed();
            QString id = lnomesAlternativos.takeFirst();
            vlistaNomesAlternativos.insert(id,lnomesAlternativos);
            if(vdatabaseReady)
                finsereNomeAlternativo(id,lnomesAlternativos);
        }
        larquivo.close();
    }
}

void Database::finsereDataHashId(QString tipo, QString lista, QString id, QStringList nomes)
{
    int size = 0;
    if(tipo.compare("anime") == 0){
        if(lista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            size = vlistaAnimeWatching.size()-1;
        else if(lista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            size = vlistaAnimeCompleted.size()-1;
        else if(lista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            size = vlistaAnimePlanToWatch.size()-1;
        else if(lista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            size = vlistaAnimeDropped.size()-1;
        else if(lista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            size = vlistaAnimeOnHold.size()-1;

        if(!vhashPosicaoAnimesPorId.contains(id))
            vhashPosicaoAnimesPorId.insert(id, size);
        else if(vhashPosicaoAnimesPorId[id] != size)
            vhashPosicaoAnimesPorId.insert(id, size);
        if(!vhashNomeAnimesPorId.contains(id))
            vhashNomeAnimesPorId.insert(id, nomes);
    }
    else if(tipo.compare("manga") == 0){
        if(lista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            size = vlistaMangaReading.size()-1;
        else if(lista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            size = vlistaMangaCompleted.size()-1;
        else if(lista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            size = vlistaMangaPlanToRead.size()-1;
        else if(lista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            size = vlistaMangaDropped.size()-1;
        else if(lista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            size = vlistaMangaOnHold.size()-1;

        if(!vhashPosicaoMangasPorId.contains(id))
            vhashPosicaoMangasPorId.insert(id, size);
        else if(vhashPosicaoMangasPorId[id] != size)
            vhashPosicaoMangasPorId.insert(id, size);
        if(!vhashNomeMangasPorId.contains(id))
            vhashNomeMangasPorId.insert(id, nomes);
    }
    else if(tipo.compare("novel") == 0){
        if(lista.compare("CURRENT", Qt::CaseInsensitive) == 0)
            size = vlistaNovelReading.size()-1;
        else if(lista.compare("COMPLETED", Qt::CaseInsensitive) == 0)
            size = vlistaNovelCompleted.size()-1;
        else if(lista.compare("PLANNING", Qt::CaseInsensitive) == 0)
            size = vlistaNovelPlanToRead.size()-1;
        else if(lista.compare("DROPPED", Qt::CaseInsensitive) == 0)
            size = vlistaNovelDropped.size()-1;
        else if(lista.compare("PAUSED", Qt::CaseInsensitive) == 0)
            size = vlistaNovelOnHold.size()-1;

        if(!vhashPosicaoNovelsPorId.contains(id))
            vhashPosicaoNovelsPorId.insert(id, size);
        else if(vhashPosicaoNovelsPorId[id] != size)
            vhashPosicaoNovelsPorId.insert(id, size);
        if(!vhashNomeNovelsPorId.contains(id))
            vhashNomeNovelsPorId.insert(id, nomes);
    }
}

void Database::insereDataHashPosicao(Database::type tipo, QString id, int posicao)
{
    return;
}

void Database::fcarregaListaAnoEmThread()
{
    //Essa função serve para saber se os animes do ano x já estão salvos na lista de ids.
    //Quando eles já estão salvos, o ano vai ser salvo no hash.
    //Isso é importante para a função de download de torrents, onde eles vão buscar o id dos animes.
    for(int i = 0; i < QDate::currentDate().year() - 1999; i++){
        if(!vhashSizeListasPorAno.contains(2000+i)){
            returnAnimeYearlyList(2000+i);
        }
    }
}

QVector<anime *> Database::returnList(QString lista, type tipo, QString argumento)
{
    if(!vdatabaseReady)
        return vlistaTemp;
    if(tipo == MANGA)
        return returnMangaList(lista);
    else if(tipo == NOVEL)
        return returnNovelList(lista);
    else if(tipo == SEASON)
        return returnAnimeSeasonalList(lista);
    else
        return returnAnimeList(lista, argumento);
}

QVector<anime *> Database::returnAnimeList(QString lista, QString argumento)
{
    if(lista.compare("CURRENT") == 0)
        return vlistaAnimeWatching;
    else if(lista.compare("COMPLETED") == 0)
        return vlistaAnimeCompleted;
    else if(lista.compare("PLANNING") == 0)
        return vlistaAnimePlanToWatch;
    else if(lista.compare("DROPPED") == 0)
        return vlistaAnimeDropped;
    else if(lista.compare("PAUSED") == 0)
        return vlistaAnimeOnHold;
    else if(lista.compare("SEARCH") == 0 && !argumento.isEmpty())
        return returnSearchList(argumento, ANIME);
    else{
        bool ok;
        //Checa se a lista é um número válido
        lista.toInt(&ok);
        if(ok){
            return returnAnimeYearlyList(lista.toInt());
        }
        return vlistaAnimeWatching;
    }
}

QVector<anime *> Database::returnAnimeSeasonalList(QString season)
{
    if(season.compare("WINTER") == 0)
        return vlistaAnimeSeasonWinter;
    else if(season.compare("SUMMER") == 0)
        return vlistaAnimeSeasonSummer;
    else if(season.compare("FALL") == 0)
        return vlistaAnimeSeasonFall;
    else if(season.compare("SPRING") == 0)
        return vlistaAnimeSeasonSpring;
    else
        return vlistaAnimeSeasonWinter;
}

QVector<anime *> Database::returnMangaList(QString lista, QString argumento)
{
    if(lista.compare("CURRENT") == 0)
        return vlistaMangaReading;
    else if(lista.compare("COMPLETED") == 0)
        return vlistaMangaCompleted;
    else if(lista.compare("PLANNING") == 0)
        return vlistaMangaPlanToRead;
    else if(lista.compare("DROPPED") == 0)
        return vlistaMangaDropped;
    else if(lista.compare("PAUSED") == 0)
        return vlistaMangaOnHold;
    else if(lista.compare("SEARCH") == 0 && !argumento.isEmpty())
        return returnSearchList(argumento, MANGA);
    else
        return vlistaMangaReading;

}

QVector<anime *> Database::returnNovelList(QString lista, QString argumento)
{
    if(lista == "CURRENT")
        return vlistaNovelReading;
    else if(lista == "COMPLETED")
        return vlistaNovelCompleted;
    else if(lista == "PLANNING")
        return vlistaNovelPlanToRead;
    else if(lista == "DROPPED")
        return vlistaNovelDropped;
    else if(lista == "PAUSED")
        return vlistaNovelOnHold;
    else if(lista.compare("SEARCH") == 0 && !argumento.isEmpty())
        return returnSearchList(argumento, NOVEL);
    else
        return vlistaNovelReading;
}

QVector<anime *> Database::returnSearchList(const QString &rnome, Database::type rtipoMidia)
{
    vlistaBusca.clear();
    if(!vdatabaseReady)
        return vlistaBusca;

    if(rtipoMidia == type::ANIME){
        foreach(QString key, vhashNomeAnimesPorId.keys()){
            if(!vdatabaseReady)
                return vlistaBusca;
            for(int i = 0; i < vhashNomeAnimesPorId[key].size(); i++){
                if(vhashNomeAnimesPorId[key].at(i).contains(rnome, Qt::CaseInsensitive)){
                    QString lista = vhashListaAnimesPorId[key];
                    int pos = vhashPosicaoAnimesPorId[key];
                    if(pos == -1)
                        break;
                    else if(lista.compare("Watching", Qt::CaseInsensitive) == 0 && vlistaAnimeWatching.size() > pos){
                        vlistaBusca.append(vlistaAnimeWatching[pos]);
                        break;
                    }
                    else if(lista.compare("Plan to Watch", Qt::CaseInsensitive) == 0 && vlistaAnimePlanToWatch.size() > pos){
                        vlistaBusca.append(vlistaAnimePlanToWatch[pos]);
                        break;
                    }
                    else if(lista.compare("On Hold", Qt::CaseInsensitive) == 0 && vlistaAnimeOnHold.size() > pos){
                        vlistaBusca.append(vlistaAnimeOnHold[pos]);
                        break;
                    }
                    else if(lista.compare("Completed", Qt::CaseInsensitive) == 0 && vlistaAnimeCompleted.size() > pos){
                        vlistaBusca.append(vlistaAnimeCompleted[pos]);
                        break;
                    }
                    else if(lista.compare("Dropped", Qt::CaseInsensitive) == 0 && vlistaAnimeDropped.size() > pos){
                        vlistaBusca.append(vlistaAnimeDropped[pos]);
                        break;
                    }
                    else{
                        bool ok;
                        //Checa se a lista é um número válido
                        lista.toInt(&ok);
                        if(ok){
                            vlistaBusca.append(fbuscaAnimeNoAno(lista.toInt(), key));
                        }
                        break;
                    }
                }
            }
        }
    }
    else if(rtipoMidia == type::MANGA){
        foreach(QString key, vhashNomeMangasPorId.keys()){
            if(!vdatabaseReady)
                return vlistaBusca;

            for(int i = 0; i < vhashNomeMangasPorId[key].size(); i++){
                if(vhashNomeMangasPorId[key].at(i).contains(rnome, Qt::CaseInsensitive)){
                    QString lista = vhashListaMangasPorId[key];
                    int pos = vhashPosicaoMangasPorId[key];
                    if(pos == -1)
                        break;
                    else if(lista.compare("Reading(Manga)", Qt::CaseInsensitive) == 0 && vlistaMangaReading.size() > pos){
                        vlistaBusca.append(vlistaMangaReading[pos]);
                        break;
                    }
                    else if(lista.compare("Plan to Read(Manga)", Qt::CaseInsensitive) == 0 && vlistaMangaPlanToRead.size() > pos){
                        vlistaBusca.append(vlistaMangaPlanToRead[pos]);
                        break;
                    }
                    else if(lista.compare("On Hold(Manga)", Qt::CaseInsensitive) == 0 && vlistaMangaOnHold.size() > pos){
                        vlistaBusca.append(vlistaMangaOnHold[pos]);
                        break;
                    }
                    else if(lista.compare("Completed(Manga)", Qt::CaseInsensitive) == 0 && vlistaMangaCompleted.size() > pos){
                        vlistaBusca.append(vlistaMangaCompleted[pos]);
                        break;
                    }
                    else if(lista.compare("Dropped(Manga)", Qt::CaseInsensitive) == 0 && vlistaMangaDropped.size() > pos){
                        vlistaBusca.append(vlistaMangaDropped[pos]);
                        break;
                    }
                }
            }
        }
    }
    else if(rtipoMidia == type::NOVEL){
        foreach(QString key, vhashNomeNovelsPorId.keys()){
            if(!vdatabaseReady)
                return vlistaBusca;

            for(int i = 0; i < vhashNomeNovelsPorId[key].size(); i++){
                if(vhashNomeNovelsPorId[key].at(i).contains(rnome, Qt::CaseInsensitive)){
                    QString lista = vhashListaNovelsPorId[key];
                    int pos = vhashPosicaoNovelsPorId[key];
                    if(pos == -1)
                        break;
                    else if(lista.compare("Reading(Novel)", Qt::CaseInsensitive) == 0 && vlistaNovelReading.size() > pos){
                        vlistaBusca.append(vlistaNovelReading[pos]);
                        break;
                    }
                    else if(lista.compare("Plan to Read(Novel)", Qt::CaseInsensitive) == 0 && vlistaNovelPlanToRead.size() > pos){
                        vlistaBusca.append(vlistaNovelPlanToRead[pos]);
                        break;
                    }
                    else if(lista.compare("On Hold(Novel)", Qt::CaseInsensitive) == 0 && vlistaNovelOnHold.size() > pos){
                        vlistaBusca.append(vlistaNovelOnHold[pos]);
                        break;
                    }
                    else if(lista.compare("Completed(Novel)", Qt::CaseInsensitive) == 0 && vlistaNovelCompleted.size() > pos){
                        vlistaBusca.append(vlistaNovelCompleted[pos]);
                        break;
                    }
                    else if(lista.compare("Dropped(Novel)", Qt::CaseInsensitive) == 0 && vlistaNovelDropped.size() > pos){
                        vlistaBusca.append(vlistaNovelDropped[pos]);
                        break;
                    }
                }
            }
        }
    }
    return vlistaBusca;
}

QVector<anime *> Database::returnSortList(const QString &rordem, QString lista, type rtipo)
{
    vlistaTemp = returnList(lista, rtipo);

    if(!rordem.isEmpty())
    {
        if(rordem.startsWith("c")){
            if(rordem.contains("nome", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{ return a->vnome < b->vnome; });
            else if(rordem.contains("data", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{ return a->vdataEstreia < b->vdataEstreia;});
            else if(rordem.contains("progresso", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vnumEpisodiosAssistidos.toInt() < b->vnumEpisodiosAssistidos.toInt() ;});
            else if(rordem.contains("nota", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vnotaMediaPessoal.toInt() < b->vnotaMediaPessoal.toInt() ;});
            else if(rordem.contains("formato", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{ return a->vformato < b->vformato ;});
        }
        else{
            if(rordem.contains("nome", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{ return a->vnome > b->vnome ;});
            else if(rordem.contains("data", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{ return a->vdataEstreia > b->vdataEstreia ;});
            else if(rordem.contains("progresso", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{
                    return static_cast<float>(a->vnumEpisodiosAssistidos.toInt()) > static_cast<float>(b->vnumEpisodiosAssistidos.toInt()) ;});
            else if(rordem.contains("nota", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vnotaMediaPessoal.toInt()> b->vnotaMediaPessoal.toInt() ;});
            else if(rordem.contains("formato", Qt::CaseInsensitive))
                std::sort(vlistaTemp.begin(),vlistaTemp.end(),[](anime* a, anime* b)->bool{ return a->vformato > b->vformato ;});
        }
    }
    return vlistaTemp;
}

QString Database::fbuscaIDRapido(const QString &rnomeAnime)
{
    foreach(QString key, vhashNomeAnimesPorId.keys()){
        if(vhashNomeAnimesPorId[key].contains(rnomeAnime))
            return key;
    }
    return "";
}

QString Database::fbuscaIDRetornaLista(const QString &ridAnime)
{
    if(!vdatabaseReady)
        return "";
    if(vhashListaAnimesPorId.contains(ridAnime))
        return vhashListaAnimesPorId[ridAnime];
    else if(vhashListaMangasPorId.contains(ridAnime))
        return vhashListaMangasPorId[ridAnime];
    else if(vhashListaNovelsPorId.contains(ridAnime))
        return vhashListaNovelsPorId[ridAnime];
    return "";
}

int Database::fbuscaIDRetornaPosicao(const QString &ridAnime)
{
    //Checa se podemos ler as listas
    if(!vdatabaseReady)
        return -1;
    bool addHash = false;
    QString llista = fbuscaIDRetornaLista(ridAnime);
    QVector<anime*> tempList;
    if(vhashPosicaoAnimesPorId.contains(ridAnime)){
        int lposicao = vhashPosicaoAnimesPorId[ridAnime];
        if(llista.compare("Watching", Qt::CaseInsensitive) == 0 && (vlistaAnimeWatching.size() <= lposicao ||
                vlistaAnimeWatching[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaAnimeWatching;
            addHash = true;
        }
        else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0 && (vlistaAnimePlanToWatch.size() <= lposicao ||
                    vlistaAnimePlanToWatch[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaAnimePlanToWatch;
            addHash = true;
        }
        else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0 && (vlistaAnimeOnHold.size() <= lposicao ||
                    vlistaAnimeOnHold[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaAnimeOnHold;
            addHash = true;
        }
        else if(llista.compare("Completed", Qt::CaseInsensitive) == 0 && (vlistaAnimeCompleted.size() <= lposicao ||
                    vlistaAnimeCompleted[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaAnimeCompleted;
            addHash = true;
        }
        else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0 && (vlistaAnimeDropped.size() <= lposicao ||
                    vlistaAnimeDropped[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaAnimeDropped;
            addHash = true;
        }
        if(addHash){
            //Checa se a lista está disponível para ser lida
            if(!vdatabaseReady)
                return -1;
            for(int i = 0; i < tempList.size(); i++){
                if(tempList[i]->vid.compare(ridAnime) == 0){
                    vhashPosicaoAnimesPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        return vhashPosicaoAnimesPorId[ridAnime];
    }
    else if(vhashPosicaoMangasPorId.contains(ridAnime)){
        int lposicao = vhashPosicaoMangasPorId[ridAnime];
        if(llista.compare("Reading(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaReading.size() <= lposicao ||
                    vlistaMangaReading[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaMangaReading;
            addHash = true;
        }
        else if(llista.compare("Completed(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaCompleted.size() <= lposicao ||
                    vlistaMangaCompleted[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaMangaCompleted;
            addHash = true;
        }
        else if(llista.compare("On Hold(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaOnHold.size() <= lposicao ||
                    vlistaMangaOnHold[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaMangaOnHold;
            addHash = true;
        }
        else if(llista.compare("Dropped(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaDropped.size() <= lposicao ||
                    vlistaMangaDropped[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaMangaDropped;
            addHash = true;
        }
        else if(llista.compare("Plan to Read(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaPlanToRead.size() <= lposicao ||
                    vlistaMangaPlanToRead[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaMangaPlanToRead;
            addHash = true;
        }
        if(addHash){
            //Checa se a lista está disponível para ser lida
            if(!vdatabaseReady)
                return -1;
            for(int i = 0; i < tempList.size(); i++){
                if(tempList[i]->vid.compare(ridAnime) == 0){
                    vhashPosicaoMangasPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        return vhashPosicaoMangasPorId[ridAnime];
    }
    else if(vhashPosicaoNovelsPorId.contains(ridAnime)){
        int lposicao = vhashPosicaoNovelsPorId[ridAnime];
        if(llista.compare("Reading(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelReading.size() <= lposicao ||
                    vlistaNovelReading[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaNovelReading;
            addHash = true;
        }
        else if(llista.compare("Completed(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelCompleted.size() <= lposicao ||
                    vlistaNovelCompleted[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaNovelCompleted;
            addHash = true;
        }
        else if(llista.compare("On Hold(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelOnHold.size() <= lposicao ||
                    vlistaNovelOnHold[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaNovelOnHold;
            addHash = true;
        }
        else if(llista.compare("Plan to Read(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelDropped.size() <= lposicao ||
                    vlistaNovelDropped[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaNovelDropped;
            addHash = true;
        }
        else if(llista.compare("Dropped(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelPlanToRead.size() <= lposicao ||
                    vlistaNovelPlanToRead[lposicao]->vid.compare(ridAnime) != 0)){
            tempList = vlistaNovelPlanToRead;
            addHash = true;
        }
        if(addHash){
            //Checa se a lista está disponível para ser lida
            if(!vdatabaseReady)
                return -1;
            for(int i = 0; i < tempList.size(); i++){
                if(tempList[i]->vid.compare(ridAnime) == 0){
                    vhashPosicaoNovelsPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        return vhashPosicaoNovelsPorId[ridAnime];
    }
    return -1;
}

QString Database::fbuscaIDRetornaEpisodio(const QString &rid)
{
    QString llista = fbuscaIDRetornaLista(rid);
    int lposicao = fbuscaIDRetornaPosicao(rid);
    if(lposicao == -1)
        return "0";

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching.size() > lposicao)
            return vlistaAnimeWatching[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        if(vlistaAnimePlanToWatch.size() > lposicao)
            return vlistaAnimePlanToWatch[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeOnHold.size() > lposicao)
            return vlistaAnimeOnHold[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeCompleted.size() > lposicao)
            return vlistaAnimeCompleted[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeDropped.size() > lposicao)
            return vlistaAnimeDropped[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "Reading(Manga)"){
        if(vlistaMangaReading.size() > lposicao)
            return vlistaMangaReading[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "Completed(Manga)"){
        if(vlistaMangaCompleted.size() > lposicao)
            return vlistaMangaCompleted[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "On Hold(Manga)"){
        if(vlistaMangaOnHold.size() > lposicao)
            return vlistaMangaOnHold[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "Dropped(Manga)"){
        if(vlistaMangaDropped.size() > lposicao)
            return vlistaMangaDropped[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "Plan to Read(Manga)"){
        if(vlistaMangaPlanToRead.size() > lposicao)
            return vlistaMangaPlanToRead[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "Reading(Novel)"){
        if(vlistaNovelReading.size() > lposicao)
            return vlistaNovelReading[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "Completed(Novel)"){
        if(vlistaNovelCompleted.size() > lposicao)
            return vlistaNovelCompleted[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "On Hold(Novel)"){
        if(vlistaNovelOnHold.size() > lposicao)
            return vlistaNovelOnHold[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "Dropped(Novel)"){
        if(vlistaNovelDropped.size() > lposicao)
            return vlistaNovelDropped[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista == "Plan to Read(Novel)"){
        if(vlistaNovelPlanToRead.size() > lposicao)
            return vlistaNovelPlanToRead[lposicao]->vnumEpisodiosAssistidos;
    }
    return "0";

}

QString Database::fbuscaIDRetornaNota(const QString &rid)
{
    QString llista = fbuscaIDRetornaLista(rid);
    int lposicao = fbuscaIDRetornaPosicao(rid);
    if(lposicao == -1)
        return "0";

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching.size() > lposicao)
            return vlistaAnimeWatching[lposicao]->vnotaMediaPessoal;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        if(vlistaAnimePlanToWatch.size() > lposicao)
            return vlistaAnimePlanToWatch[lposicao]->vnotaMediaPessoal;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeOnHold.size() > lposicao)
            return vlistaAnimeOnHold[lposicao]->vnotaMediaPessoal;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeCompleted.size() > lposicao)
            return vlistaAnimeCompleted[lposicao]->vnotaMediaPessoal;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeDropped.size() > lposicao)
            return vlistaAnimeDropped[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "Reading(Manga)"){
        if(vlistaMangaReading.size() > lposicao)
            return vlistaMangaReading[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "Completed(Manga)"){
        if(vlistaMangaCompleted.size() > lposicao)
            return vlistaMangaCompleted[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "On Hold(Manga)"){
        if(vlistaMangaOnHold.size() > lposicao)
            return vlistaMangaOnHold[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "Dropped(Manga)"){
        if(vlistaMangaDropped.size() > lposicao)
            return vlistaMangaDropped[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "Plan to Read(Manga)"){
        if(vlistaMangaPlanToRead.size() > lposicao)
            return vlistaMangaPlanToRead[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "Reading(Novel)"){
        if(vlistaNovelReading.size() > lposicao)
            return vlistaNovelReading[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "Completed(Novel)"){
        if(vlistaNovelCompleted.size() > lposicao)
            return vlistaNovelCompleted[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "On Hold(Novel)"){
        if(vlistaNovelOnHold.size() > lposicao)
            return vlistaNovelOnHold[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "Dropped(Novel)"){
        if(vlistaNovelDropped.size() > lposicao)
            return vlistaNovelDropped[lposicao]->vnotaMediaPessoal;
    }
    else if(llista == "Plan to Read(Novel)"){
        if(vlistaNovelPlanToRead.size() > lposicao)
            return vlistaNovelPlanToRead[lposicao]->vnotaMediaPessoal;
    }
    return "0";
}

QString Database::fbuscaIDRetornaTitulo(const QString &rid)
{
    QString llista = fbuscaIDRetornaLista(rid);
    int lposicao = fbuscaIDRetornaPosicao(rid);
    if(lposicao == -1)
        return "0";

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeWatching.size() > lposicao)
            return vlistaAnimeWatching[lposicao]->vnome;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        if(vlistaAnimePlanToWatch.size() > lposicao)
            return vlistaAnimePlanToWatch[lposicao]->vnome;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeOnHold.size() > lposicao)
            return vlistaAnimeOnHold[lposicao]->vnome;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeCompleted.size() > lposicao)
            return vlistaAnimeCompleted[lposicao]->vnome;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        if(vlistaAnimeDropped.size() > lposicao)
            return vlistaAnimeDropped[lposicao]->vnome;
    }
    else if(llista == "Reading(Manga)"){
        if(vlistaMangaReading.size() > lposicao)
            return vlistaMangaReading[lposicao]->vnome;
    }
    else if(llista == "Completed(Manga)"){
        if(vlistaMangaCompleted.size() > lposicao)
            return vlistaMangaCompleted[lposicao]->vnome;
    }
    else if(llista == "On Hold(Manga)"){
        if(vlistaMangaOnHold.size() > lposicao)
            return vlistaMangaOnHold[lposicao]->vnome;
    }
    else if(llista == "Dropped(Manga)"){
        if(vlistaMangaDropped.size() > lposicao)
            return vlistaMangaDropped[lposicao]->vnome;
    }
    else if(llista == "Plan to Read(Manga)"){
        if(vlistaMangaPlanToRead.size() > lposicao)
            return vlistaMangaPlanToRead[lposicao]->vnome;
    }
    else if(llista == "Reading(Novel)"){
        if(vlistaNovelReading.size() > lposicao)
            return vlistaNovelReading[lposicao]->vnome;
    }
    else if(llista == "Completed(Novel)"){
        if(vlistaNovelCompleted.size() > lposicao)
            return vlistaNovelCompleted[lposicao]->vnome;
    }
    else if(llista == "On Hold(Novel)"){
        if(vlistaNovelOnHold.size() > lposicao)
            return vlistaNovelOnHold[lposicao]->vnome;
    }
    else if(llista == "Dropped(Novel)"){
        if(vlistaNovelDropped.size() > lposicao)
            return vlistaNovelDropped[lposicao]->vnome;
    }
    else if(llista == "Plan to Read(Novel)"){
        if(vlistaNovelPlanToRead.size() > lposicao)
            return vlistaNovelPlanToRead[lposicao]->vnome;
    }
    return "";
}

QString Database::fbuscaNomeRetornaID(const QString &rnomeAnime)
{
    if(!vdatabaseReady)
        return "";
    //Essa função é importante por que se ela não encontrar o nome certo logo de cara, vai fazer uma busca mais detalhada e adicionar
    //o nome procurado nas informações do anime, quando encontrar.
    foreach(QString key, vhashNomeAnimesPorId.keys()){
        for(int i = 0; i < vhashNomeAnimesPorId[key].size(); i++){
            if(vhashNomeAnimesPorId[key].at(i) == rnomeAnime){
                return key;
            }
        }
    }
    QStringList tempNomeAnime;
    QVector<anime*> tempList = vlistaAnimeWatching;


    int listSize;
    QString nome;
    QString nomeIngles;
    QStringList nomeAlternativo;
    QString id;
    for(int k = 0; k < 5; k++){
        if(k == 0)
            tempList = vlistaAnimeWatching;
        else if(k == 1)
            tempList = vlistaAnimePlanToWatch;
        else if(k == 2)
            tempList = vlistaAnimeOnHold;
        else if(k == 3)
            tempList = vlistaAnimeDropped;
        else if(k == 4)
            tempList = vlistaAnimeCompleted;

        //Checa se a lista não foi deletada
        if(tempList.isEmpty())
            return "";

        listSize = tempList.size();
        for(int i = 0; i < listSize; i++){
            //Checa se a lista não foi apagada
            if(!vdatabaseReady)
                return "";

            nome = tempList[i]->vnome;
            nomeIngles = tempList[i]->vnomeIngles;
            nomeAlternativo = tempList[i]->vnomeAlternativo;
            id = tempList[i]->vid;
            if(formatador.fcomparaNomes(nome, rnomeAnime) ||
                    formatador.fcomparaNomes(nomeIngles, rnomeAnime)){
                tempNomeAnime.append(rnomeAnime);
                if(!vhashNomeAnimesPorId.contains(id))
                    vhashNomeAnimesPorId.insert(id, tempNomeAnime);
                else
                    vhashNomeAnimesPorId[id].append(tempNomeAnime);
                return id;
            }
            else if(!nomeAlternativo.isEmpty()){
                for(int w = 0; w < nomeAlternativo.size(); w++){
                    if(formatador.fcomparaNomes(nomeAlternativo.at(w), rnomeAnime)){
                        tempNomeAnime.append(rnomeAnime);
                        if(!vhashNomeAnimesPorId.contains(id))
                            vhashNomeAnimesPorId.insert(id, tempNomeAnime);
                        else
                            vhashNomeAnimesPorId[id].append(tempNomeAnime);
                        return id;
                    }
                }
            }
        }
    }

    return "";
}

QString Database::limpaNull(QString linha)
{
    linha.replace("null", "?");
    linha.replace("null,", "?");
    return linha;
}

QString Database::limpaImagem(QString linha)
{
    linha.remove("\",");
    linha.remove("\"");
    linha.remove("large: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaSinopse(QString linha)
{
    linha.remove("\",");
    linha.remove("\\n");
    linha.remove("\\r");
    linha.remove("\\");
    linha.remove("\"description\": \"");
    linha = linha.trimmed();
    linha = linha.toUtf8();
    return linha;
}

QString Database::limpaEpisodiosTotais(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("episodes: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaFormato(QString linha)
{
    linha.remove("\",");
    linha.remove("\"");
    linha.replace("_", " ");
    linha.remove("format: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaId(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("id: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaStreamUrl(QString linha)
{
    if(linha.contains("crunchyroll")){
        linha.remove(",");
        linha.remove("\"");
        linha.remove("url: ");
        if(linha.contains("?"))
            linha.replace("?", "null");
        linha = linha.left(linha.lastIndexOf("episode-")).trimmed();
    }
    return linha;
}

QString Database::limpaChapters(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("chapters: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaVolumes(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("volumes: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaDatabaseUrl(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("siteUrl: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaDataProximoEpisodio(QString linha)
{
    if(linha.contains("?"))
        linha = "Not      Airing";
    else
        linha = "";
    return linha;
}

QString Database::limpaAiring(QString linha, QDate &ldataEpisodioConvertida, QTime &lhoraLancamentoEpisodio)
{

    linha.remove(",");
    linha.remove("\"");
    linha.remove("airingAt: ");
    linha = linha.trimmed();
    QDateTime ldataEpisodio;

    ldataEpisodio.setSecsSinceEpoch(linha.toInt());
    ldataEpisodioConvertida = ldataEpisodio.date();
    lhoraLancamentoEpisodio = ldataEpisodio.time();

    if(ldataEpisodioConvertida.dayOfWeek() == 1)
        linha = "Monday ";
    else if(ldataEpisodioConvertida.dayOfWeek() == 2)
        linha = "Tuesday ";
    else if(ldataEpisodioConvertida.dayOfWeek() == 3)
        linha = "Wednesday ";
    else if(ldataEpisodioConvertida.dayOfWeek() == 4)
        linha = "Thursday ";
    else if(ldataEpisodioConvertida.dayOfWeek() == 5)
        linha = "Friday  ";
    else if(ldataEpisodioConvertida.dayOfWeek() == 6)
        linha = "Saturday ";
    else if(ldataEpisodioConvertida.dayOfWeek() == 7)
        linha = "Sunday ";
    linha.append(lhoraLancamentoEpisodio.toString("hh:mm"));
    return linha;

}

QString Database::limpaNumeroProximoEpisodio(QString linha)
{
    linha.remove("\",");
    linha.remove("\"");
    linha.remove("episode: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaSeason(QString linha)
{
    linha.remove("\",");
    linha.remove("\"");
    linha.remove("season: ");
    linha = linha.trimmed();
    linha = linha.toLower();
    linha[0] = linha.at(0).toUpper();
    if(linha.contains("?"))
        linha = "-";
    return linha;
}

QString Database::limpaMonth(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("month: ");
    linha = linha.trimmed();
    return linha;
}

QString Database::limpaYear(QString linha, QString season)
{
    linha.remove("\",");
    linha.remove("\"");
    linha.remove("year: ");
    if(season == "-")
        linha = "";
    return linha.trimmed();
}

QString Database::limpaStatus(QString linha, QString &dataEpisodioFinal, QDate &ldataEpisodioConvertida, QTime &lhoraLancamentoEpisodio)
{
    linha = linha.trimmed();
    if(linha == "FINISHED")
        linha = "Finished Airing";
    else if(linha == "RELEASING")
        linha = "Ongoing";
    else{
        linha = "Not Aired Yet";
        dataEpisodioFinal = QLocale(QLocale::English).toString(ldataEpisodioConvertida,"MMMM d ");
        dataEpisodioFinal.append(lhoraLancamentoEpisodio.toString("hh:mm"));
        if(dataEpisodioFinal.isEmpty())
            dataEpisodioFinal = "-";
    }
    return linha;
}

QString Database::limpaNomesAlternativos(QString linha)
{
    linha.remove("\",");

    //Checa se existem aspas no título do anime, como Sing "YESTERDAY" to me
    //e mantem apenas as aspas do título
    if(linha.contains("\\\"")){
        linha.remove("\"");
        linha.replace("\\","\"");
    }
    else
        linha.remove("\"");
    return linha.trimmed();
}

QString Database::limpaNomeIngles(QString linha)
{
    linha.remove(",");
    if(linha.contains("\\\"")){
        linha.remove("\"");
        linha.replace("\\","\"");
    }
    else
        linha.remove("\"");
    linha.remove("english:");
    return linha.trimmed();
}

QString Database::limpaNomeRomaji(QString linha, int &temporada)
{

    linha.remove(",");
    if(linha.contains("\\\"")){
        linha.remove("\"");
        linha.replace("\\","\"");
    }
    else
        linha.remove("\"");
    linha.remove("romaji:");
    linha = linha.trimmed();
    if(linha.at(linha.length()-1).isNumber())
        temporada = linha.at(linha.length()-1).digitValue();
    else
        temporada = 1;
    return linha;
}

QString Database::limpaProgresso(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("progress: ");
    linha = linha.trimmed();
    if(linha.isEmpty())
        return "0";
    else
        return linha;
}

QString Database::limpaScoreMedio(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("score: ");
    return linha.trimmed();
}

QString Database::limpaScoreGeral(QString linha)
{
    linha.remove(",");
    linha.remove("\"");
    linha.remove("averageScore: ");
    return linha.trimmed();
}

QString Database::limpaLista(QString linha)
{
    linha.remove("\",");
    linha.remove("\"");
    linha.remove("status: ");
    return linha.trimmed();
}

QDate Database::setDataEstreia(QString ano, QString mes, QString season)
{
    QDate dataEstreia;
    if(ano != "?" && mes != "?"){
        dataEstreia.setDate(ano.toInt(), mes.toInt(), 1);
    }
    else if(mes != "?"){
        if(season == "Winter")
            mes = "3";
        else if(season == "Spring")
            mes = "6";
        else if(season == "Summer")
            mes = "9";
        else
            mes = "12";
        if(mes == "?")
            mes = "12";
        dataEstreia.setDate(ano.toInt(), mes.toInt(), 31);
    }
    else if(ano != "?"){
        if(mes == "?")
            mes = "12";
        dataEstreia.setDate(2050, mes.toInt(), 31);
    }
    return dataEstreia;
}
