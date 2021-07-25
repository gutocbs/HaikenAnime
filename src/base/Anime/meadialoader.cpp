#include "meadialoader.h"

MeadiaLoader::MeadiaLoader(QObject *parent) : QObject(parent)
{

}

//TODO - Refatorar função quando terminar de escrever
//TODO - Organizar arquivos nas pastas certas
bool MeadiaLoader::loadMediaFromFile()
{
    finishedLoading = false;

    //Checa se arquivo pode ser lido
    //MOCK
    QFile jsonMediaListFile(":/Anilist/qrc/Mocks/Anilist/MediaListMock.json");
//    QFile jsonMediaListFile("Configurações/Temp/animeList.txt");
    if(!jsonMediaListFile.open(QIODevice::ReadOnly)) {
           qCritical() << "Could not read file!";
           qCritical() << jsonMediaListFile.errorString();
           return false;
    }
    if(jsonMediaListFile.size() == 0)
        return false;

    QByteArray jsonData = jsonMediaListFile.readAll();
    jsonMediaListFile.close();

    QJsonDocument jsonMediaList = QJsonDocument::fromJson(jsonData);
    QVariantList jsonVariantList = qvariant_cast<QVariantList>(jsonMediaList["mediaList"]);
    QJsonArray mediaList = QJsonArray::fromVariantList(jsonVariantList);
    QHash<QString, QStringList> hashMediaNamesById = mediaListManager->getHashMediaNamesById(Enums::NOMEALTERNATIVO);

    for(int i = 0; i < mediaList.size(); i++){
        mediaObject = mediaList.takeAt(i).toObject().value("media").toObject();
        QPointer<Media> media(new Media);
        media->vid = getNumberValueFromKey("id");
        media->vnome = getQStringValueFromKey("title","romaji");
        media->vnomeIngles = getQStringValueFromKey("title", "english");
        if(!hashMediaNamesById.contains(media->vid))
            hashMediaNamesById.insert(media->vid, getQStringListValuesFromKey("synonyms"));
        media->vnomeAlternativo = hashMediaNamesById[media->vid];
    }
//        lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
//        if(!lnotaMediaSite.isEmpty())
//            lnovoAnime->vnotaMediaSite = lnotaMediaSite;
//        else
//            lnovoAnime->vnotaMediaSite = "-";

//        lnovoAnime->vLinkImagemMedia = lLinkImagemMedia;
//        lnovoAnime->vnotaMediaPessoal = lnotaMediaPessoal;
//        lnovoAnime->vstatus = lstatus;
//        lnovoAnime->vsinopse = lsinopse;
//        lnovoAnime->vseason = lseason + " " + lano;
//        lnovoAnime->vnumProximoEpisodioLancado = lproximoEpisodio;
//        lnovoAnime->vnumEpisodiosAssistidos = lvnumEpisodiosAssistidos;
//        lnovoAnime->vformato = lformato;
//        lnovoAnime->vdataEstreia = ldataEstreia;
//        lnovoAnime->vdataEpisodio = ldataEpisodioFinal;
//        lnovoAnime->vdatabaseSite = ldatabaseSite;
//        lnovoAnime->vstream = lstream;
//        lnovoAnime->vtemporada = ltemporadaAnime;


//    }

//    //Variáveis de animes
//    QString lid;
//    QString lnome;
//    QString lnomeIngles;
//    QStringList lnomeAlternativo; //StringList por que pode ter lários nomes alternativos
//    QString lnumEpisodiosTotais;
//    QString lvnumEpisodiosAssistidos;
//    QString lnotaMediaSite;
//    QString lLinkImagemMedia;
//    QString lnotaMediaPessoal;
//    QString lsinopse;
//    QString lseason;
//    QString lstatus;
//    QString llista;
//    QString lformato;
//    QString lstream;
//    QString ldatabaseSite;
//    QString lproximoEpisodio;
//    QString lano;
//    QString lmes;
//    QString lchapters;
//    QString lvolumes;
//    QString ldataEpisodioFinal;
//    QDate ldataEpisodioConvertida;
//    QTime lhoraLancamentoEpisodio;
//    int ltemporadaAnime = 1; //Todas as temporadas começam como 1
//    QDate ldataEstreia; //Data de estréia do próximo episódio
//    QStringList tempNomeAnime;

//    //Começa a ler o arquivo de database
//    if(lleJson.open(QIODevice::ReadOnly)){
//        while(!json.atEnd()){
//            QString linha = json.readLine();
//            if(linha.contains("null"))
//                linha = limpaNull(linha);
//            //Procura pelo primeiro dado do anime, a capa
//            if(linha.contains("\"large\":"))
//                lLinkImagemMedia = limpaImagem(linha);
//            else if(linha.contains("\"description\":"))
//                lsinopse = limpaSinopse(linha);
//            else if(linha.contains("\"episodes\":"))
//                lnumEpisodiosTotais = limpaEpisodiosTotais(linha);
//            else if(linha.contains("\"format\":"))
//                lformato = limpaFormato(linha);
//            else if(linha.contains("\"id\":"))
//                lid = limpaId(linha);
//            else if(linha.contains("\"url\":"))
//                lstream = limpaStreamUrl(linha);
//            else if(linha.contains("\"chapters\":"))
//                lchapters = limpaChapters(linha);
//            else if(linha.contains("\"volumes\":"))
//                lvolumes = limpaVolumes(linha);
//            else if(linha.contains("\"siteUrl\":"))
//                ldatabaseSite = limpaDatabaseUrl(linha);
//            else if(linha.contains("\"nextAiringEpisode\":"))
//                ldataEpisodioFinal = limpaDataProximoEpisodio(linha);
//            else if(linha.contains("\"airingAt\":"))
//                ldataEpisodioFinal.append(limpaAiring(linha, ldataEpisodioConvertida, lhoraLancamentoEpisodio));
//            else if(linha.contains("\"episode\":"))
//                lproximoEpisodio = limpaNumeroProximoEpisodio(linha);
//            else if(linha.contains("\"season\":"))
//                lseason = limpaSeason(linha);
//            else if(linha.contains("\"month\":"))
//                lmes = limpaMonth(linha);
//            else if(linha.contains("\"year\":")){
//                lano = limpaYear(linha, lseason);
//                ldataEstreia = setDataEstreia(lano, lmes, lseason);
//            }
//            else if(linha.contains("\"status\":"))
//                lstatus = limpaStatus(linha, ldataEpisodioFinal, ldataEpisodioConvertida, lhoraLancamentoEpisodio);
//            else if(linha.contains("\"synonyms\":")){
//                linha = json.readLine();
//                while(!linha.contains("],")){
//                    linha = limpaNomesAlternativos(linha);
//                    lnomeAlternativo.append(linha);
//                    linha = json.readLine();
//                }

//            }
//            else if(linha.contains("\"english\":"))
//                lnomeIngles = limpaNomeIngles(linha);
//            else if(linha.contains("\"romaji\":"))
//                lnome = limpaNomeRomaji(linha, ltemporadaAnime);
//            else if(linha.contains("\"progress\":"))
//                lvnumEpisodiosAssistidos = limpaProgresso(linha);
//            else if(linha.contains("\"averageScore\":"))
//                lnotaMediaSite = limpaScoreGeral(linha);
//            else if(linha.contains("\"score\":")){
//                lnotaMediaPessoal = limpaScoreMedio(linha);
//                linha = json.readLine();
//                llista = limpaLista(linha);
//                if(lnome == "?")
//                    lnome = lnomeIngles;
//                else if (lnomeIngles == "?")
//                    lnomeIngles = lnome;

//                QPointer<anime> lnovoAnime(new anime);

//                lnovoAnime->vnome = lnome;
//                lnovoAnime->vnomeIngles = lnomeIngles;
//                if(!lnomeAlternativo.isEmpty()){
//                    if(vlistaNomesAlternativos.contains(lid))
//                        lnomeAlternativo.append(vlistaNomesAlternativos[lid]);
//                    lnovoAnime->vnomeAlternativo = lnomeAlternativo;
//                    lnomeAlternativo.empty();
//                }
//                lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
//                if(!lnotaMediaSite.isEmpty())
//                    lnovoAnime->vnotaMediaSite = lnotaMediaSite;
//                else
//                    lnovoAnime->vnotaMediaSite = "-";

//                lnovoAnime->vLinkImagemMedia = lLinkImagemMedia;
//                lnovoAnime->vnotaMediaPessoal = lnotaMediaPessoal;
//                lnovoAnime->vstatus = lstatus;
//                lnovoAnime->vsinopse = lsinopse;
//                lnovoAnime->vid = lid;
//                lnovoAnime->vseason = lseason + " " + lano;
//                lnovoAnime->vnumProximoEpisodioLancado = lproximoEpisodio;
//                lnovoAnime->vnumEpisodiosAssistidos = lvnumEpisodiosAssistidos;
//                lnovoAnime->vformato = lformato;
//                lnovoAnime->vdataEstreia = ldataEstreia;
//                lnovoAnime->vdataEpisodio = ldataEpisodioFinal;
//                lnovoAnime->vdatabaseSite = ldatabaseSite;
//                lnovoAnime->vstream = lstream;
//                lnovoAnime->vtemporada = ltemporadaAnime;

//                vidEntrada.append(lid.toInt());

//                tempNomeAnime.clear();

//                tempNomeAnime.append(lnovoAnime->vnome);
//                tempNomeAnime.append(lnovoAnime->vnomeIngles);
//                tempNomeAnime.append(lnovoAnime->vnomeAlternativo);

//                if(lformato != "MANGA" && lformato != "NOVEL" && lformato != "ONE SHOT"){
//                    if(llista == "CURRENT"){
//                        lnovoAnime->vlista = "Watching";
//                        vlistaAnimeWatching.append(lnovoAnime);
//                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "COMPLETED"){
//                        lnovoAnime->vlista = "Completed";
//                        vlistaAnimeCompleted.append(lnovoAnime);
//                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "PLANNING"){
//                        lnovoAnime->vlista = "Plan to Watch";
//                        vlistaAnimePlanToWatch.append(lnovoAnime);
//                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "DROPPED"){
//                        lnovoAnime->vlista = "Dropped";
//                        vlistaAnimeDropped.append(lnovoAnime);
//                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "PAUSED"){
//                        lnovoAnime->vlista = "On Hold";
//                        vlistaAnimeOnHold.append(lnovoAnime);
//                        finsereDataHashId("anime", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    sAnimeAdicionadoNaLista(lnovoAnime->vid);
//                    if(!vhashListaAnimesPorId.contains(lnovoAnime->vid))
//                        vhashListaAnimesPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
//                    else if(vhashListaAnimesPorId[lnovoAnime->vid] != lnovoAnime->vlista)
//                        vhashListaAnimesPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
//                }
//                else if(lformato == "MANGA" || lformato == "ONE SHOT"){
//                    if(lnovoAnime->vstatus.contains("Air", Qt::CaseInsensitive))
//                        lnovoAnime->vstatus.replace("Air","Releas");
//                    lnovoAnime->vnumEpisodiosTotais = lchapters;
//                    if(llista == "CURRENT"){
//                        lnovoAnime->vlista = "Reading(Manga)";
//                        vlistaMangaReading.append(lnovoAnime);
//                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "COMPLETED"){
//                        lnovoAnime->vlista = "Completed(Manga)";
//                        vlistaMangaCompleted.append(lnovoAnime);
//                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "PLANNING"){
//                        lnovoAnime->vlista = "Plan to Read(Manga)";
//                        vlistaMangaPlanToRead.append(lnovoAnime);
//                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "DROPPED"){
//                        lnovoAnime->vlista = "Dropped(Manga)";
//                        vlistaMangaDropped.append(lnovoAnime);
//                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "PAUSED"){
//                        lnovoAnime->vlista = "On Hold(Manga)";
//                        vlistaMangaOnHold.append(lnovoAnime);
//                        finsereDataHashId("manga", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    sMangaAdicionadoNaLista(lnovoAnime->vid);
//                    if(!vhashListaMangasPorId.contains(lnovoAnime->vid))
//                        vhashListaMangasPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
//                }
//                else if(lformato == "NOVEL"){
//                    lnovoAnime->vnumEpisodiosTotais = lvolumes;
//                    if(llista == "CURRENT"){
//                        lnovoAnime->vlista = "Reading(Novel)";
//                        vlistaNovelReading.append(lnovoAnime);
//                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "COMPLETED"){
//                        lnovoAnime->vlista = "Completed(Novel)";
//                        vlistaNovelCompleted.append(lnovoAnime);
//                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "PLANNING"){
//                        lnovoAnime->vlista = "Plan to Read(Novel)";
//                        vlistaNovelPlanToRead.append(lnovoAnime);
//                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "DROPPED"){
//                        lnovoAnime->vlista = "Dropped(Novel)";
//                        vlistaNovelDropped.append(lnovoAnime);
//                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    else if(llista == "PAUSED"){
//                        lnovoAnime->vlista = "On Hold(Novel)";
//                        vlistaNovelOnHold.append(lnovoAnime);
//                        finsereDataHashId("novel", llista, lnovoAnime->vid, tempNomeAnime);
//                    }
//                    sNovelAdicionadoNaLista(lnovoAnime->vid);
//                    if(!vhashListaNovelsPorId.contains(lnovoAnime->vid))
//                        vhashListaNovelsPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
//                }
//                lnome.clear();
//                lnomeIngles.clear();
//                lnomeAlternativo.clear();
//                lnumEpisodiosTotais.clear();
//                lnotaMediaSite.clear();
//                lLinkImagemMedia.clear();
//                lnotaMediaPessoal.clear();
//                lstatus.clear();
//                lsinopse.clear();
//                lid.clear();
//                lseason.clear();
//                lproximoEpisodio.clear();
//                lvnumEpisodiosAssistidos.clear();
//                lformato.clear();
//                lformato.clear();
//                lstream.clear();
//                ldatabaseSite.clear();
//                ldataEpisodioFinal.clear();
//                ldataEpisodioConvertida = QDate();
//                lhoraLancamentoEpisodio = QTime();
//            }
//        }
//        lleJson.close();
//    }
//    else
//        return false;

//    vdatabaseReady = true;
    return true;

}

QString MeadiaLoader::getQStringValueFromKey(QString objectName, QString key)
{
    QString value = "";
    if(mediaObject.contains(objectName) && key.isEmpty())
        value = mediaObject.value(objectName).toString();
    else if(mediaObject.contains(objectName) && mediaObject.value(objectName).toObject().contains(key))
        value = mediaObject.value(objectName).toObject().value(key).toString();
    return value;
}

QStringList MeadiaLoader::getQStringListValuesFromKey(QString objectName, QString key)
{
    QJsonArray jsonArrayValue;
    QStringList value;
    //Verifica se o objeto possui filhos ou não
    if(mediaObject.contains(objectName) && key.isEmpty())
        jsonArrayValue = QJsonArray::fromVariantList(mediaObject.value(objectName).toArray().toVariantList());
    else if(mediaObject.contains(objectName) && mediaObject.value(objectName).toObject().contains(key))
        jsonArrayValue = QJsonArray::fromVariantList(mediaObject.value(objectName).toObject().value(key).toArray().toVariantList());
    foreach(QJsonValue jsonValue, jsonArrayValue){
        value.append(jsonValue.toString());
    }
    return value;
}

int MeadiaLoader::getNumberValueFromKey(QString objectName, QString key)
{
    int value = 0;
    if(mediaObject.contains(objectName) && key.isEmpty())
        value = mediaObject.value(objectName).toInt();
    else if(mediaObject.contains(objectName) && mediaObject.value(objectName).toObject().contains(key))
        value = mediaObject.value(objectName).toObject().value(key).toInt();
    return value;
}
