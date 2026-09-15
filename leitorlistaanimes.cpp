#include "leitorlistaanimes.h"

leitorlistaanimes::leitorlistaanimes(QObject *parent) : QObject(parent)
{
}

leitorlistaanimes::~leitorlistaanimes(){
//    fdeletaListaAnimes();
    qDeleteAll(vlistaWatching.begin(),vlistaWatching.end()); //Deletar todas as listas
    qDeleteAll(vlistaCompleted.begin(),vlistaCompleted.end()); //Deletar todas as listas
    qDeleteAll(vlistaDropped.begin(),vlistaDropped.end()); //Deletar todas as listas
    qDeleteAll(vlistaOnHold.begin(),vlistaOnHold.end()); //Deletar todas as listas
    qDeleteAll(vlistaPlanToWatch.begin(),vlistaPlanToWatch.end()); //Deletar todas as listas
    qDeleteAll(vlistaBusca.begin(),vlistaBusca.end()); //Deletar todas as listas
}

bool leitorlistaanimes::fleJson(){
    QFile lleJson("Configurações/Temp/animeList.txt");
    if(lleJson.size() == 0)
        return false;
    QTextStream json(&lleJson);
    json.setCodec("UTF-8");
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
    QString lstreamCrunchyroll;
    QString lsiteAnilist;
    QString lproximoEpisodio;
    QString lano;
    QString lmes;
    QString lchapters;
    QString lvolumes;
    QString ldataEpisodioFinal;
    int ltemporadaAnime = 1;
    QDateTime ldataEpisodio;
    QTime lhoraLancamentoEpisodio;
    QDate ldataEpisodioConvertida;
    QDate ldataEstreia;///Data de estréia do próximo episódio
    QStringList tempNomeAnime;

    if(lleJson.open(QIODevice::ReadOnly)){
        while(!json.atEnd()){
            QString llinha = json.readLine();
            if(llinha.contains("null")){
                llinha.replace("null", "?");
                llinha.replace("null,", "?");
            }
            //Procura pelo primeiro dado do anime, a capa
            if(llinha.contains("\"large\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("large: ");
                lLinkImagemMedia = llinha.trimmed();
            }
            else if(llinha.contains("\"description\":")){
                llinha.remove("\",");
                llinha.remove("\\n");
                llinha.remove("\\r");
                llinha.remove("\\");
                llinha.remove("\"description\": \"");
                llinha = llinha.trimmed();
                lsinopse = llinha.toUtf8();
            }
            else if(llinha.contains("\"episodes\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("episodes: ");
                lnumEpisodiosTotais = llinha.trimmed();
            }
            else if(llinha.contains("\"format\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.replace("_", " ");
                llinha.remove("format: ");
                lformato = llinha.trimmed();
            }
            else if(llinha.contains("\"id\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("id: ");
                lid = llinha.trimmed();
            }
            else if(llinha.contains("\"url\":")){
                if(llinha.contains("crunchyroll")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("url: ");
                    if(llinha.contains("?"))
                        llinha.replace("?", "null");
                    lstreamCrunchyroll = llinha.left(llinha.lastIndexOf("episode-")).trimmed();
                }
            }
            else if(llinha.contains("\"chapters\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("chapters: ");
                lchapters = llinha.trimmed();
            }
            else if(llinha.contains("\"volumes\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("volumes: ");
                lvolumes = llinha.trimmed();
            }
            else if(llinha.contains("\"siteUrl\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("siteUrl: ");
                lsiteAnilist = llinha.trimmed();
            }
            else if(llinha.contains("\"nextAiringEpisode\":")){
                if(llinha.contains("?"))
                    ldataEpisodioFinal = "Not      Airing";
            }
            else if(llinha.contains("\"airingAt\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("airingAt: ");
                llinha = llinha.trimmed();
                ldataEpisodio.setSecsSinceEpoch(llinha.toInt());
                ldataEpisodioConvertida = ldataEpisodio.date();
                lhoraLancamentoEpisodio = ldataEpisodio.time();
                if(ldataEpisodioConvertida.dayOfWeek() == 1)
                    ldataEpisodioFinal = "Monday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 2)
                    ldataEpisodioFinal = "Tuesday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 3)
                    ldataEpisodioFinal = "Wednesday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 4)
                    ldataEpisodioFinal = "Thursday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 5)
                    ldataEpisodioFinal = "Friday  ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 6)
                    ldataEpisodioFinal = "Saturday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 7)
                    ldataEpisodioFinal = "Sunday ";
                ldataEpisodioFinal.append(lhoraLancamentoEpisodio.toString("hh:mm"));
            }
            else if(llinha.contains("\"episode\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("episode: ");
                lproximoEpisodio = llinha.trimmed();
            }
            else if(llinha.contains("\"season\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("season: ");
                llinha = llinha.trimmed();
                llinha = llinha.toLower();
                llinha[0] = llinha.at(0).toUpper();
                if(llinha.contains("?"))
                    lseason = "-";
                else
                    lseason = llinha;
            }
            else if(llinha.contains("\"month\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("month: ");
                lmes = llinha.trimmed();
            }
            else if(llinha.contains("\"year\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("year: ");
                if(lseason == "-")
                    lano = "";
                else
                    lano = llinha.trimmed();
            }
            else if(llinha.contains("\"status\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("status: ");
                llinha = llinha.trimmed();
                if(llinha == "FINISHED")
                    lstatus = "Finished Airing";
                else if(llinha == "RELEASING")
                    lstatus = "Ongoing";
                else{
                    lstatus = "Not Aired Yet";
                    ldataEpisodioFinal = QLocale(QLocale::English).toString(ldataEpisodioConvertida,"MMMM d ");
                    ldataEpisodioFinal.append(lhoraLancamentoEpisodio.toString("hh:mm"));
                    if(ldataEpisodioFinal.isEmpty())
                        ldataEpisodioFinal = "-";
                }
            }
            else if(llinha.contains("\"synonyms\":")){
                llinha = json.readLine();
                while(!llinha.contains("],")){
                    llinha.remove("\",");

                    //Checa se existem aspas no título do anime, como Sing "YESTERDAY" to me
                    //e mantem apenas as aspas do título
                    if(llinha.contains("\\\"")){
                        llinha.remove("\"");
                        llinha.replace("\\","\"");
                    }
                    else
                        llinha.remove("\"");
                    lnomeAlternativo.append(llinha.trimmed());
                    llinha = json.readLine();
                }
            }
            else if(llinha.contains("\"english\":")){
                llinha.remove(",");
                if(llinha.contains("\\\"")){
                    llinha.remove("\"");
                    llinha.replace("\\","\"");
                }
                else
                    llinha.remove("\"");
                llinha.remove("english:");
                lnomeIngles = llinha.trimmed();
            }
            else if(llinha.contains("\"romaji\":")){
                llinha.remove(",");
                if(llinha.contains("\\\"")){
                    llinha.remove("\"");
                    llinha.replace("\\","\"");
                }
                else
                    llinha.remove("\"");
                llinha.remove("romaji:");
                lnome = llinha.trimmed();
                if(lnome.endsWith("2"))
                    ltemporadaAnime = 2;
                else if(lnome.endsWith("3"))
                    ltemporadaAnime = 3;
                else if(lnome.endsWith("4"))
                    ltemporadaAnime = 4;
                else
                    ltemporadaAnime = 1;

            }
            else if(llinha.contains("\"progress\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("progress: ");
                lvnumEpisodiosAssistidos = llinha.trimmed();
            }
            else if(llinha.contains("\"averageScore\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("averageScore: ");
                lnotaMediaSite = llinha.trimmed();
            }
            else if(llinha.contains("\"score\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("score: ");
                lnotaMediaPessoal = llinha.trimmed();
                llinha = json.readLine();
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("status: ");
                llista = llinha.trimmed();
                QPointer<anime> lnovoAnime(new anime);
//                anime* lnovoAnime = new anime;
                if(lnome == "?")
                    lnome = lnomeIngles;
                else if (lnomeIngles == "?")
                    lnomeIngles = lnome;
                lnovoAnime->vnome = lnome;
                lnovoAnime->vnomeIngles = lnomeIngles;
                if(!lnomeAlternativo.isEmpty()){
                    lnovoAnime->vnomeAlternativo = lnomeAlternativo;
                    lnomeAlternativo.empty();
                }
                lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
                if(!lnotaMediaSite.isEmpty())
                    lnovoAnime->vnotaMediaSite = lnotaMediaSite;
                else
                    lnovoAnime->vnotaMediaSite = "-";
                if(lano != "?" && lmes != "?"){
                    ldataEstreia.setDate(lano.toInt(), lmes.toInt(), 1);
                }
                else if(lmes != "?"){
                    if(lseason == "Winter")
                        lmes = "3";
                    else if(lseason == "Spring")
                        lmes = "6";
                    else if(lseason == "Summer")
                        lmes = "9";
                    else
                        lmes = "12";
                    if(lmes == "?")
                        lmes = "12";
                    ldataEstreia.setDate(lano.toInt(), lmes.toInt(), 31);
                }
                else if(lano != "?"){
                    if(lmes == "?")
                        lmes = "12";
                    ldataEstreia.setDate(2050, lmes.toInt(), 31);
                }
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
                lnovoAnime->vsiteAnilist = lsiteAnilist;
                lnovoAnime->vstreamCrunchyroll = lstreamCrunchyroll;
                lnovoAnime->vtemporada = ltemporadaAnime;
                tempNomeAnime.clear();
                if(lformato != "MANGA" && lformato != "NOVEL" && lformato != "ONE SHOT"){
                    if(llinha.trimmed() == "CURRENT"){
                        lnovoAnime->vlista = "Watching";
                        vlistaWatching.append(lnovoAnime);
                        if(!vHashPosicaoAnimesPorId.contains(lnovoAnime->vid))
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaWatching.size()-1);
                        else if(vHashPosicaoAnimesPorId[lnovoAnime->vid] != vlistaWatching.size()-1)
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaWatching.size()-1);
                        if(!vHashNomeAnimesPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeAnimesPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "COMPLETED"){
                        lnovoAnime->vlista = "Completed";
                        vlistaCompleted.append(lnovoAnime);
                        if(!vHashPosicaoAnimesPorId.contains(lnovoAnime->vid))
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaCompleted.size()-1);
                        else if(vHashPosicaoAnimesPorId[lnovoAnime->vid] != vlistaCompleted.size()-1)
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaCompleted.size()-1);
                        if(!vHashNomeAnimesPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeAnimesPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "PLANNING"){
                        lnovoAnime->vlista = "Plan to Watch";
                        vlistaPlanToWatch.append(lnovoAnime);
                        if(!vHashPosicaoAnimesPorId.contains(lnovoAnime->vid))
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaPlanToWatch.size()-1);
                        else if(vHashPosicaoAnimesPorId[lnovoAnime->vid] != vlistaPlanToWatch.size()-1)
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaPlanToWatch.size()-1);
                        if(!vHashNomeAnimesPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeAnimesPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "DROPPED"){
                        lnovoAnime->vlista = "Dropped";
                        vlistaDropped.append(lnovoAnime);
                        if(!vHashPosicaoAnimesPorId.contains(lnovoAnime->vid))
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaDropped.size()-1);
                        else if(vHashPosicaoAnimesPorId[lnovoAnime->vid] != vlistaDropped.size()-1)
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaDropped.size()-1);
                        if(!vHashNomeAnimesPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeAnimesPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "PAUSED"){
                        lnovoAnime->vlista = "On Hold";
                        vlistaOnHold.append(lnovoAnime);
                        if(!vHashPosicaoAnimesPorId.contains(lnovoAnime->vid))
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaOnHold.size()-1);
                        else if(vHashPosicaoAnimesPorId[lnovoAnime->vid] != vlistaOnHold.size()-1)
                            vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vlistaOnHold.size()-1);
                        if(!vHashNomeAnimesPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeAnimesPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    sAnimeAdicionadoNaLista(lnovoAnime->vid);
                    if(!vHashListaAnimesPorId.contains(lnovoAnime->vid))
                        vHashListaAnimesPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
                    else if(vHashListaAnimesPorId[lnovoAnime->vid] != lnovoAnime->vlista)
                        vHashListaAnimesPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
                }
                else if(lformato == "MANGA" || lformato == "ONE SHOT"){
                    if(lnovoAnime->vstatus.contains("Air", Qt::CaseInsensitive))
                        lnovoAnime->vstatus.replace("Air","Releas");
                    lnovoAnime->vnumEpisodiosTotais = lchapters;
                    if(llinha.trimmed() == "CURRENT"){
                        lnovoAnime->vlista = "Reading(Manga)";
                        vlistaMangaReading.append(lnovoAnime);
                        if(!vHashPosicaoMangasPorId.contains(lnovoAnime->vid))
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaReading.size()-1);
                        else if(vHashPosicaoMangasPorId[lnovoAnime->vid] != vlistaMangaReading.size()-1)
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaReading.size()-1);
                        if(!vHashNomeMangasPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeMangasPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "COMPLETED"){
                        lnovoAnime->vlista = "Completed(Manga)";
                        vlistaMangaCompleted.append(lnovoAnime);
                        if(!vHashPosicaoMangasPorId.contains(lnovoAnime->vid))
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaCompleted.size()-1);
                        else if(vHashPosicaoMangasPorId[lnovoAnime->vid] != vlistaMangaCompleted.size()-1)
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaCompleted.size()-1);
                        if(!vHashNomeMangasPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeMangasPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "PLANNING"){
                        lnovoAnime->vlista = "Plan to Read(Manga)";
                        vlistaMangaPlanToRead.append(lnovoAnime);
                        if(!vHashPosicaoMangasPorId.contains(lnovoAnime->vid))
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaPlanToRead.size()-1);
                        else if(vHashPosicaoMangasPorId[lnovoAnime->vid] != vlistaMangaPlanToRead.size()-1)
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaPlanToRead.size()-1);
                        if(!vHashNomeMangasPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeMangasPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "DROPPED"){
                        lnovoAnime->vlista = "Dropped(Manga)";
                        vlistaMangaDropped.append(lnovoAnime);
                        if(!vHashPosicaoMangasPorId.contains(lnovoAnime->vid))
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaDropped.size()-1);
                        else if(vHashPosicaoMangasPorId[lnovoAnime->vid] != vlistaMangaDropped.size()-1)
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaDropped.size()-1);
                        if(!vHashNomeMangasPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeMangasPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "PAUSED"){
                        lnovoAnime->vlista = "On Hold(Manga)";
                        vlistaMangaOnHold.append(lnovoAnime);
                        if(!vHashPosicaoMangasPorId.contains(lnovoAnime->vid))
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaOnHold.size()-1);
                        else if(vHashPosicaoMangasPorId[lnovoAnime->vid] != vlistaMangaOnHold.size()-1)
                            vHashPosicaoMangasPorId.insert(lnovoAnime->vid, vlistaMangaOnHold.size()-1);
                        if(!vHashNomeMangasPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeMangasPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    sMangaAdicionadoNaLista(lnovoAnime->vid);
                    if(!vHashListaMangasPorId.contains(lnovoAnime->vid))
                        vHashListaMangasPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
                }
                else if(lformato == "NOVEL"){
                    lnovoAnime->vnumEpisodiosTotais = lvolumes;
                    if(llinha.trimmed() == "CURRENT"){
                        lnovoAnime->vlista = "Reading(Novel)";
                        vlistaNovelReading.append(lnovoAnime);
                        if(!vHashPosicaoNovelsPorId.contains(lnovoAnime->vid))
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelReading.size()-1);
                        else if(vHashPosicaoNovelsPorId[lnovoAnime->vid] != vlistaNovelReading.size()-1)
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelReading.size()-1);
                        if(!vHashNomeNovelsPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeNovelsPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "COMPLETED"){
                        lnovoAnime->vlista = "Completed(Novel)";
                        vlistaNovelCompleted.append(lnovoAnime);
                        if(!vHashPosicaoNovelsPorId.contains(lnovoAnime->vid))
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelCompleted.size()-1);
                        else if(vHashPosicaoNovelsPorId[lnovoAnime->vid] != vlistaNovelCompleted.size()-1)
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelCompleted.size()-1);
                        if(!vHashNomeNovelsPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeNovelsPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "PLANNING"){
                        lnovoAnime->vlista = "Plan to Read(Novel)";
                        vlistaNovelPlanToRead.append(lnovoAnime);
                        if(!vHashPosicaoNovelsPorId.contains(lnovoAnime->vid))
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelPlanToRead.size()-1);
                        else if(vHashPosicaoNovelsPorId[lnovoAnime->vid] != vlistaNovelPlanToRead.size()-1)
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelPlanToRead.size()-1);
                        if(!vHashNomeNovelsPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeNovelsPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "DROPPED"){
                        lnovoAnime->vlista = "Dropped(Novel)";
                        vlistaNovelDropped.append(lnovoAnime);
                        if(!vHashPosicaoNovelsPorId.contains(lnovoAnime->vid))
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelDropped.size()-1);
                        else if(vHashPosicaoNovelsPorId[lnovoAnime->vid] != vlistaNovelDropped.size()-1)
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelDropped.size()-1);
                        if(!vHashNomeNovelsPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeNovelsPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    else if(llinha.trimmed() == "PAUSED"){
                        lnovoAnime->vlista = "On Hold(Novel)";
                        vlistaNovelOnHold.append(lnovoAnime);
                        if(!vHashPosicaoNovelsPorId.contains(lnovoAnime->vid))
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelOnHold.size()-1);
                        else if(vHashPosicaoNovelsPorId[lnovoAnime->vid] != vlistaNovelOnHold.size()-1)
                            vHashPosicaoNovelsPorId.insert(lnovoAnime->vid, vlistaNovelOnHold.size()-1);
                        if(!vHashNomeNovelsPorId.contains(lnovoAnime->vid)){
                            tempNomeAnime.append(lnovoAnime->vnome);
                            tempNomeAnime.append(lnovoAnime->vnomeIngles);
                            tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                            vHashNomeNovelsPorId.insert(lnovoAnime->vid, tempNomeAnime);
                        }
                    }
                    sNovelAdicionadoNaLista(lnovoAnime->vid);
                    if(!vHashListaNovelsPorId.contains(lnovoAnime->vid))
                        vHashListaNovelsPorId.insert(lnovoAnime->vid, lnovoAnime->vlista);
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
                lstreamCrunchyroll.clear();
                lsiteAnilist.clear();
                ldataEpisodioFinal.clear();
                ldataEpisodioConvertida = QDate();
                lhoraLancamentoEpisodio = QTime();
            }
        }
        lleJson.close();
    }
    //Caso o arquivo não possa ser lido, retorna false
    else
        return false;
    return true;
}

QVector<anime*> leitorlistaanimes::fleListaAno(int ano)
{
    QVector<anime*> vetorTemporario;
    QFile lleJson("Configurações/Temp/Lists/animeList"+QString::number(ano)+".txt");
    if(lleJson.size() == 0)
        return vetorTemporario;
    if(!vHashSizeListasPorAno.contains(ano))
        vHashSizeListasPorAno.insert(ano, QString::number(lleJson.size()));
    QTextStream json(&lleJson);
    json.setCodec("UTF-8");
    QString lid;
    QString lnome;
    QString lnomeIngles;
//    QString lnomeJapones;
    QStringList lnomeAlternativo; //StringList por que pode ter lários nomes alternativos
    QString lnumEpisodiosTotais;
    QString lvnumEpisodiosAssistidos;
    QString lnotaMediaSite;
    QString lLinkImagemMedia;
    QString lnotaMediaPessoal;
    QString lsinopse;
    QString lseason;
    QString lstatus;
//    QString llista;
    QString lformato;
    QString lstreamCrunchyroll;
    QString lsiteAnilist;
    QString lproximoEpisodio;
    QString lano;
    QString lmes;
    QString lchapters;
    QString lvolumes;
    QString ldataEpisodioFinal;
    int ltemporadaAnime = 1;
    QDateTime ldataEpisodio;
    QTime lhoraLancamentoEpisodio;
    QDate ldataEpisodioConvertida;
    QDate ldataEstreia;///Data de estréia do próximo episódio

    if(lleJson.open(QIODevice::ReadOnly)){
        while(!json.atEnd()){
            QString llinha = json.readLine();
            if(llinha.contains("null")){
                llinha.replace("null", "?");
                llinha.replace("null,", "?");
            }
            //Procura pelo primeiro dado do anime, a capa
            if(llinha.contains("\"large\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("large: ");
                lLinkImagemMedia = llinha.trimmed();
            }
            else if(llinha.contains("\"description\":")){
                llinha.remove("\",");
                llinha.remove("\\n");
                llinha.remove("\\r");
                llinha.remove("\\");
                llinha.remove("\"description\": \"");
                llinha = llinha.trimmed();
                lsinopse = llinha.toUtf8();
            }
            else if(llinha.contains("\"episodes\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("episodes: ");
                lnumEpisodiosTotais = llinha.trimmed();
            }
            else if(llinha.contains("\"format\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.replace("_", " ");
                llinha.remove("format: ");
                lformato = llinha.trimmed();
            }
            else if(llinha.contains("\"id\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("id: ");
                lid = llinha.trimmed();
            }
            else if(llinha.contains("\"url\":")){
                if(llinha.contains("crunchyroll")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("url: ");
                    if(llinha.contains("?"))
                        llinha.replace("?", "null");
                    lstreamCrunchyroll = llinha.left(llinha.lastIndexOf("episode-")).trimmed();
                }
            }
            else if(llinha.contains("\"chapters\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("chapters: ");
                lchapters = llinha.trimmed();
            }
            else if(llinha.contains("\"volumes\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("volumes: ");
                lvolumes = llinha.trimmed();
            }
            else if(llinha.contains("\"siteUrl\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("siteUrl: ");
                lsiteAnilist = llinha.trimmed();
            }
            else if(llinha.contains("\"nextAiringEpisode\":")){
                if(llinha.contains("?"))
                    ldataEpisodioFinal = "Not      Airing";
            }
            else if(llinha.contains("\"airingAt\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("airingAt: ");
                llinha = llinha.trimmed();
                ldataEpisodio.setSecsSinceEpoch(llinha.toInt());
                ldataEpisodioConvertida = ldataEpisodio.date();
                lhoraLancamentoEpisodio = ldataEpisodio.time();
                if(ldataEpisodioConvertida.dayOfWeek() == 1)
                    ldataEpisodioFinal = "Monday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 2)
                    ldataEpisodioFinal = "Tuesday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 3)
                    ldataEpisodioFinal = "Wednesday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 4)
                    ldataEpisodioFinal = "Thursday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 5)
                    ldataEpisodioFinal = "Friday  ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 6)
                    ldataEpisodioFinal = "Saturday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 7)
                    ldataEpisodioFinal = "Sunday ";
                ldataEpisodioFinal.append(lhoraLancamentoEpisodio.toString("hh:mm"));
            }
            else if(llinha.contains("\"episode\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("episode: ");
                lproximoEpisodio = llinha.trimmed();
            }
            else if(llinha.contains("\"season\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("season: ");
                llinha = llinha.trimmed();
                llinha = llinha.toLower();
                llinha[0] = llinha.at(0).toUpper();
                if(llinha.contains("?"))
                    lseason = "-";
                else
                    lseason = llinha;
            }
            else if(llinha.contains("\"month\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("month: ");
                lmes = llinha.trimmed();
            }
            else if(llinha.contains("\"year\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("year: ");
                if(lseason == "-")
                    lano = "";
                else
                    lano = llinha.trimmed();
            }
            else if(llinha.contains("\"status\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("status: ");
                llinha = llinha.trimmed();
                if(llinha == "FINISHED")
                    lstatus = "Finished Airing";
                else if(llinha == "RELEASING")
                    lstatus = "Ongoing";
                else
                    lstatus = "Not Aired Yet";
            }
            else if(llinha.contains("\"synonyms\":")){
                llinha = json.readLine();
                while(!llinha.contains("],")){
                    llinha.remove("\",");
                    llinha.remove("\"");
                    lnomeAlternativo.append(llinha.trimmed());
                    llinha = json.readLine();
                }
            }
            else if(llinha.contains("\"progress\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("progress: ");
                lvnumEpisodiosAssistidos = llinha.trimmed();
            }
            else if(llinha.contains("\"averageScore\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("averageScore: ");
                lnotaMediaSite = llinha.trimmed();
            }
            else if(llinha.contains("\"english\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("english:");
                lnomeIngles = llinha.trimmed();
            }
            else if(llinha.contains("\"romaji\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("romaji:");
                lnome = llinha.trimmed();
                if(lnome.endsWith("2"))
                    ltemporadaAnime = 2;
                else if(lnome.endsWith("3"))
                    ltemporadaAnime = 3;
                else if(lnome.endsWith("4"))
                    ltemporadaAnime = 4;
                else
                    ltemporadaAnime = 1;

                 lnotaMediaPessoal = "";
                QPointer<anime> lnovoAnime(new anime);
//                anime* lnovoAnime = new anime;
                if(lnome == "?")
                    lnome = lnomeIngles;
                else if (lnomeIngles == "?")
                    lnomeIngles = lnome;
                lnovoAnime->vnome = lnome;
                lnovoAnime->vnomeIngles = lnomeIngles;
                if(!lnomeAlternativo.isEmpty()){
                    lnovoAnime->vnomeAlternativo = lnomeAlternativo;
                    lnomeAlternativo.empty();
                }
                lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
                if(!lnotaMediaSite.isEmpty())
                    lnovoAnime->vnotaMediaSite = lnotaMediaSite;
                else
                    lnovoAnime->vnotaMediaSite = "-";
                if(lano != "?" && lmes != "?"){
                    ldataEstreia.setDate(lano.toInt(), lmes.toInt(), 1);
                }
                else if(lmes != "?"){
                    if(lseason == "Winter")
                        lmes = "3";
                    else if(lseason == "Spring")
                        lmes = "6";
                    else if(lseason == "Summer")
                        lmes = "9";
                    else
                        lmes = "12";
                    if(lmes == "?")
                        lmes = "12";
                    ldataEstreia.setDate(lano.toInt(), lmes.toInt(), 31);
                }
                else if(lano != "?"){
                    if(lmes == "?")
                        lmes = "12";
                    ldataEstreia.setDate(2050, lmes.toInt(), 31);
                }
                lnovoAnime->vlista = fbuscaAnimePorIDERetornaLista(lid);
                lnovoAnime->vnumEpisodiosAssistidos = fbuscaAnimePorIDERetornaEpisodio(lid);
                lnovoAnime->vnotaMediaPessoal = fbuscaAnimePorIDERetornaNota(lid);
                lnovoAnime->vLinkImagemMedia = lLinkImagemMedia;
                lnovoAnime->vstatus = lstatus;
                lnovoAnime->vsinopse = lsinopse;
                lnovoAnime->vid = lid;
                lnovoAnime->vseason = lseason + " " + lano;
                lnovoAnime->vSeasonSemAno = lseason;
                lnovoAnime->vnumProximoEpisodioLancado = lproximoEpisodio;
                lnovoAnime->vformato = lformato;
                lnovoAnime->vdataEstreia = ldataEstreia;
                lnovoAnime->vdataEpisodio = ldataEpisodioFinal;
                lnovoAnime->vsiteAnilist = lsiteAnilist;
                lnovoAnime->vstreamCrunchyroll = lstreamCrunchyroll;
                lnovoAnime->vtemporada = ltemporadaAnime;
                if(ldataEstreia.year() == ano){
                    vetorTemporario.append(lnovoAnime);
                    sAnimeAdicionadoNaLista(lid);
                }
                QStringList tempNomeAnime;
                if(!vHashNomeAnimesPorId.contains(lnovoAnime->vid)){
                    tempNomeAnime.append(lnovoAnime->vnome);
                    tempNomeAnime.append(lnovoAnime->vnomeIngles);
                    tempNomeAnime.append(lnovoAnime->vnomeAlternativo);
                    vHashNomeAnimesPorId.insert(lnovoAnime->vid, tempNomeAnime);
                }
                if(!vHashPosicaoAnimesPorId.contains(lnovoAnime->vid))
                    vHashPosicaoAnimesPorId.insert(lnovoAnime->vid, vetorTemporario.size()-1);
                if(!vHashListaAnimesPorId.contains(lnovoAnime->vid))
                    vHashListaAnimesPorId.insert(lnovoAnime->vid, QString::number(ano));

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
                lstreamCrunchyroll.clear();
                lsiteAnilist.clear();
            }
        }
        lleJson.close();
    }
    return vetorTemporario;
}

QVector<anime *> leitorlistaanimes::fleListaAnoSeason(int ano, const QString &seasonEscolhida)
{
    QVector<anime*> vetorTemporario;
    QFile lleJson("Configurações/Temp/Lists/animeList"+QString::number(ano)+".txt");
    if(lleJson.size() == 0)
        return vetorTemporario;
    QTextStream json(&lleJson);
    json.setCodec("UTF-8");
    QString lid;
    QString lnome;
    QString lnomeIngles;
//    QString lnomeJapones;
    QStringList lnomeAlternativo; //StringList por que pode ter lários nomes alternativos
    QString lnumEpisodiosTotais;
    QString lvnumEpisodiosAssistidos;
    QString lnotaMediaSite;
    QString lLinkImagemMedia;
    QString lnotaMediaPessoal;
    QString lsinopse;
    QString lseason;
    QString lstatus;
//    QString llista;
    QString lformato;
    QString lstreamCrunchyroll;
    QString lsiteAnilist;
    QString lproximoEpisodio;
    QString lano;
    QString lmes;
    QString lchapters;
    QString lvolumes;
    QString ldataEpisodioFinal;
    int ltemporadaAnime = 1;
    QDateTime ldataEpisodio;
    QTime lhoraLancamentoEpisodio;
    QDate ldataEpisodioConvertida;
    QDate ldataEstreia;///Data de estréia do próximo episódio

    if(lleJson.open(QIODevice::ReadOnly)){
        while(!json.atEnd()){
            QString llinha = json.readLine();
            if(llinha.contains("null")){
                llinha.replace("null", "?");
                llinha.replace("null,", "?");
            }
            //Procura pelo primeiro dado do anime, a capa
            if(llinha.contains("\"large\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("large: ");
                lLinkImagemMedia = llinha.trimmed();
            }
            else if(llinha.contains("\"description\":")){
                llinha.remove("\",");
                llinha.remove("\\n");
                llinha.remove("\\r");
                llinha.remove("\\");
                llinha.remove("\"description\": \"");
                llinha = llinha.trimmed();
                lsinopse = llinha.toUtf8();
            }
            else if(llinha.contains("\"episodes\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("episodes: ");
                lnumEpisodiosTotais = llinha.trimmed();
            }
            else if(llinha.contains("\"format\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.replace("_", " ");
                llinha.remove("format: ");
                lformato = llinha.trimmed();
            }
            else if(llinha.contains("\"id\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("id: ");
                lid = llinha.trimmed();
            }
            else if(llinha.contains("\"url\":")){
                if(llinha.contains("crunchyroll")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("url: ");
                    if(llinha.contains("?"))
                        llinha.replace("?", "null");
                    lstreamCrunchyroll = llinha.left(llinha.lastIndexOf("episode-")).trimmed();
                }
            }
            else if(llinha.contains("\"chapters\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("chapters: ");
                lchapters = llinha.trimmed();
            }
            else if(llinha.contains("\"volumes\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("volumes: ");
                lvolumes = llinha.trimmed();
            }
            else if(llinha.contains("\"siteUrl\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("siteUrl: ");
                lsiteAnilist = llinha.trimmed();
            }
            else if(llinha.contains("\"nextAiringEpisode\":")){
                if(llinha.contains("?"))
                    ldataEpisodioFinal = "Not      Airing";
            }
            else if(llinha.contains("\"airingAt\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("airingAt: ");
                llinha = llinha.trimmed();
                ldataEpisodio.setSecsSinceEpoch(llinha.toInt());
                ldataEpisodioConvertida = ldataEpisodio.date();
                lhoraLancamentoEpisodio = ldataEpisodio.time();
                if(ldataEpisodioConvertida.dayOfWeek() == 1)
                    ldataEpisodioFinal = "Monday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 2)
                    ldataEpisodioFinal = "Tuesday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 3)
                    ldataEpisodioFinal = "Wednesday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 4)
                    ldataEpisodioFinal = "Thursday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 5)
                    ldataEpisodioFinal = "Friday  ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 6)
                    ldataEpisodioFinal = "Saturday ";
                else if(ldataEpisodioConvertida.dayOfWeek() == 7)
                    ldataEpisodioFinal = "Sunday ";
                ldataEpisodioFinal.append(lhoraLancamentoEpisodio.toString("hh:mm"));
            }
            else if(llinha.contains("\"episode\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("episode: ");
                lproximoEpisodio = llinha.trimmed();
            }
            else if(llinha.contains("\"season\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("season: ");
                llinha = llinha.trimmed();
                llinha = llinha.toLower();
                llinha[0] = llinha.at(0).toUpper();
                if(llinha.contains("?"))
                    lseason = "-";
                else
                    lseason = llinha;
            }
            else if(llinha.contains("\"month\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("month: ");
                lmes = llinha.trimmed();
            }
            else if(llinha.contains("\"year\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("year: ");
                if(lseason == "-")
                    lano = "";
                else
                    lano = llinha.trimmed();
            }
            else if(llinha.contains("\"status\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("status: ");
                llinha = llinha.trimmed();
                if(llinha == "FINISHED")
                    lstatus = "Finished Airing";
                else if(llinha == "RELEASING")
                    lstatus = "Ongoing";
                else
                    lstatus = "Not Aired Yet";
            }
            else if(llinha.contains("\"synonyms\":")){
                llinha = json.readLine();
                while(!llinha.contains("],")){
                    llinha.remove("\",");
                    llinha.remove("\"");
                    lnomeAlternativo.append(llinha.trimmed());
                    llinha = json.readLine();
                }
            }
            else if(llinha.contains("\"progress\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("progress: ");
                lvnumEpisodiosAssistidos = llinha.trimmed();
            }
            else if(llinha.contains("\"averageScore\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("averageScore: ");
                lnotaMediaSite = llinha.trimmed();
            }
            else if(llinha.contains("\"english\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("english:");
                lnomeIngles = llinha.trimmed();
            }
            else if(llinha.contains("\"romaji\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("romaji:");
                lnome = llinha.trimmed();
                if(lnome.endsWith("2"))
                    ltemporadaAnime = 2;
                else if(lnome.endsWith("3"))
                    ltemporadaAnime = 3;
                else if(lnome.endsWith("4"))
                    ltemporadaAnime = 4;
                else
                    ltemporadaAnime = 1;

                 lnotaMediaPessoal = "";
//                QPointer<anime> lnovoAnime(new anime);
                anime* lnovoAnime = new anime;
                if(lnome == "?")
                    lnome = lnomeIngles;
                else if (lnomeIngles == "?")
                    lnomeIngles = lnome;
                lnovoAnime->vnome = lnome;
                lnovoAnime->vnomeIngles = lnomeIngles;
                if(!lnomeAlternativo.isEmpty()){
                    lnovoAnime->vnomeAlternativo = lnomeAlternativo;
                    lnomeAlternativo.empty();
                }
                lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
                if(!lnotaMediaSite.isEmpty())
                    lnovoAnime->vnotaMediaSite = lnotaMediaSite;
                else
                    lnovoAnime->vnotaMediaSite = "-";
                if(lano != "?" && lmes != "?"){
                    ldataEstreia.setDate(lano.toInt(), lmes.toInt(), 1);
                }
                else if(lmes != "?"){
                    if(lseason == "Winter")
                        lmes = "3";
                    else if(lseason == "Spring")
                        lmes = "6";
                    else if(lseason == "Summer")
                        lmes = "9";
                    else
                        lmes = "12";
                    if(lmes == "?")
                        lmes = "12";
                    ldataEstreia.setDate(lano.toInt(), lmes.toInt(), 31);
                }
                else if(lano != "?"){
                    if(lmes == "?")
                        lmes = "12";
                    ldataEstreia.setDate(2050, lmes.toInt(), 31);
                }
                lnovoAnime->vlista = fbuscaAnimePorIDERetornaLista(lid);
                lnovoAnime->vnumEpisodiosAssistidos = fbuscaAnimePorIDERetornaEpisodio(lid);
                lnovoAnime->vnotaMediaPessoal = fbuscaAnimePorIDERetornaNota(lid);
                lnovoAnime->vLinkImagemMedia = lLinkImagemMedia;
                lnovoAnime->vstatus = lstatus;
                lnovoAnime->vsinopse = lsinopse;
                lnovoAnime->vid = lid;
                lnovoAnime->vseason = lseason + " " + lano;
                lnovoAnime->vnumProximoEpisodioLancado = lproximoEpisodio;
                lnovoAnime->vformato = lformato;
                lnovoAnime->vdataEstreia = ldataEstreia;
                lnovoAnime->vdataEpisodio = ldataEpisodioFinal;
                lnovoAnime->vsiteAnilist = lsiteAnilist;
                lnovoAnime->vstreamCrunchyroll = lstreamCrunchyroll;
                lnovoAnime->vtemporada = ltemporadaAnime;
                if(lseason.compare(seasonEscolhida, Qt::CaseInsensitive) == 0){
                    vetorTemporario.append(lnovoAnime);
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
                lstreamCrunchyroll.clear();
                lsiteAnilist.clear();
            }
        }
        lleJson.close();
    }
    return vetorTemporario;
}

anime* leitorlistaanimes::fbuscaAnimeNoAno(int ano, const QString &rid)
{
//    QVector<anime*> vetorTemporario;
    QFile lleJson("Configurações/Temp/Lists/animeList"+QString::number(ano)+".txt");
    if(lleJson.size() == 0)
        return nullptr;
    QTextStream json(&lleJson);
    json.setCodec("UTF-8");
    QString lid;
    QString lnome;
    QString lnomeIngles;
//    QString lnomeJapones;
    QStringList lnomeAlternativo; //StringList por que pode ter lários nomes alternativos
    QString lnumEpisodiosTotais;
    QString lvnumEpisodiosAssistidos;
    QString lnotaMediaSite;
    QString lLinkImagemMedia;
    QString lnotaMediaPessoal;
    QString lsinopse;
    QString lseason;
    QString lstatus;
//    QString llista;
    QString lformato;
    QString lstreamCrunchyroll;
    QString lsiteAnilist;
    QString lproximoEpisodio;
    QString lano;
    QString lmes;
    QString lchapters;
    QString lvolumes;
    QString ldataEpisodioFinal;
    int ltemporadaAnime = 1;
    QDateTime ldataEpisodio;
    QTime lhoraLancamentoEpisodio;
    QDate ldataEpisodioConvertida;
    QDate ldataEstreia;///Data de estréia do próximo episódio
    bool encontrou = false;

    if(lleJson.open(QIODevice::ReadOnly)){
        while(!json.atEnd()){
            QString llinha = json.readLine();
            if(llinha.contains("null")){
                llinha.replace("null", "?");
                llinha.replace("null,", "?");
            }
            //Procura pelo primeiro dado do anime, o score
            if(llinha.contains("\"averageScore\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("averageScore: ");
                lnotaMediaSite = llinha.trimmed();
            }
            else if(llinha.contains("\"large\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.remove("large: ");
                lLinkImagemMedia = llinha.trimmed();
            }
            else if(llinha.contains("\"description\":")){
                llinha.remove("\",");
                llinha.remove("\\n");
                llinha.remove("\\r");
                llinha.remove("\\");
                llinha.remove("\"description\": \"");
                llinha = llinha.trimmed();
                lsinopse = llinha.toUtf8();
            }
            else if(llinha.contains("\"episodes\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("episodes: ");
                lnumEpisodiosTotais = llinha.trimmed();
            }
            else if(llinha.contains("\"format\":")){
                llinha.remove("\",");
                llinha.remove("\"");
                llinha.replace("_", " ");
                llinha.remove("format: ");
                lformato = llinha.trimmed();
            }
            else if(llinha.contains("\"id\":")){
                llinha.remove(",");
                llinha.remove("\"");
                llinha.remove("id: ");
                lid = llinha.trimmed();
                if(lid == rid)
                    encontrou = true;
            }
            if(encontrou){
                if(llinha.contains("\"url\":")){
                    if(llinha.contains("crunchyroll")){
                        llinha.remove(",");
                        llinha.remove("\"");
                        llinha.remove("url: ");
                        if(llinha.contains("?"))
                            llinha.replace("?", "null");
                        lstreamCrunchyroll = llinha.left(llinha.lastIndexOf("episode-")).trimmed();
                    }
                }
                else if(llinha.contains("\"chapters\":")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("chapters: ");
                    lchapters = llinha.trimmed();
                }
                else if(llinha.contains("\"volumes\":")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("volumes: ");
                    lvolumes = llinha.trimmed();
                }
                else if(llinha.contains("\"siteUrl\":")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("siteUrl: ");
                    lsiteAnilist = llinha.trimmed();
                }
                else if(llinha.contains("\"nextAiringEpisode\":")){
                    if(llinha.contains("?"))
                        ldataEpisodioFinal = "Not      Airing";
                }
                else if(llinha.contains("\"airingAt\":")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("airingAt: ");
                    llinha = llinha.trimmed();
                    ldataEpisodio.setSecsSinceEpoch(llinha.toInt());
                    ldataEpisodioConvertida = ldataEpisodio.date();
                    lhoraLancamentoEpisodio = ldataEpisodio.time();
                    if(ldataEpisodioConvertida.dayOfWeek() == 1)
                        ldataEpisodioFinal = "Monday ";
                    else if(ldataEpisodioConvertida.dayOfWeek() == 2)
                        ldataEpisodioFinal = "Tuesday ";
                    else if(ldataEpisodioConvertida.dayOfWeek() == 3)
                        ldataEpisodioFinal = "Wednesday ";
                    else if(ldataEpisodioConvertida.dayOfWeek() == 4)
                        ldataEpisodioFinal = "Thursday ";
                    else if(ldataEpisodioConvertida.dayOfWeek() == 5)
                        ldataEpisodioFinal = "Friday  ";
                    else if(ldataEpisodioConvertida.dayOfWeek() == 6)
                        ldataEpisodioFinal = "Saturday ";
                    else if(ldataEpisodioConvertida.dayOfWeek() == 7)
                        ldataEpisodioFinal = "Sunday ";
                    ldataEpisodioFinal.append(lhoraLancamentoEpisodio.toString("hh:mm"));
                }
                else if(llinha.contains("\"episode\":")){
                    llinha.remove("\",");
                    llinha.remove("\"");
                    llinha.remove("episode: ");
                    lproximoEpisodio = llinha.trimmed();
                }
                else if(llinha.contains("\"season\":")){
                    llinha.remove("\",");
                    llinha.remove("\"");
                    llinha.remove("season: ");
                    llinha = llinha.trimmed();
                    llinha = llinha.toLower();
                    llinha[0] = llinha.at(0).toUpper();
                    if(llinha.contains("?"))
                        lseason = "-";
                    else
                        lseason = llinha;
                }
                else if(llinha.contains("\"month\":")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("month: ");
                    lmes = llinha.trimmed();
                }
                else if(llinha.contains("\"year\":")){
                    llinha.remove("\",");
                    llinha.remove("\"");
                    llinha.remove("year: ");
                    if(lseason == "-")
                        lano = "";
                    else
                        lano = llinha.trimmed();
                }
                else if(llinha.contains("\"status\":")){
                    llinha.remove("\",");
                    llinha.remove("\"");
                    llinha.remove("status: ");
                    llinha = llinha.trimmed();
                    if(llinha == "FINISHED")
                        lstatus = "Finished Airing";
                    else if(llinha == "RELEASING")
                        lstatus = "Ongoing";
                    else
                        lstatus = "Not Aired Yet";
                }
                else if(llinha.contains("\"synonyms\":")){
                    llinha = json.readLine();
                    while(!llinha.contains("],")){
                        llinha.remove("\",");
                        llinha.remove("\"");
                        lnomeAlternativo.append(llinha.trimmed());
                        llinha = json.readLine();
                    }
                }
                else if(llinha.contains("\"progress\":")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("progress: ");
                    lvnumEpisodiosAssistidos = llinha.trimmed();
                }
                else if(llinha.contains("\"english\":")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("english:");
                    lnomeIngles = llinha.trimmed();
                }
                else if(llinha.contains("\"romaji\":")){
                    llinha.remove(",");
                    llinha.remove("\"");
                    llinha.remove("romaji:");
                    lnome = llinha.trimmed();
                    if(lnome.endsWith("2"))
                        ltemporadaAnime = 2;
                    else if(lnome.endsWith("3"))
                        ltemporadaAnime = 3;
                    else if(lnome.endsWith("4"))
                        ltemporadaAnime = 4;
                    else
                        ltemporadaAnime = 1;

                     lnotaMediaPessoal = "";
                    QPointer<anime> lnovoAnime(new anime);
    //                anime* lnovoAnime = new anime;
                    if(lnome == "?")
                        lnome = lnomeIngles;
                    else if (lnomeIngles == "?")
                        lnomeIngles = lnome;
                    lnovoAnime->vnome = lnome;
                    lnovoAnime->vnomeIngles = lnomeIngles;
                    if(!lnomeAlternativo.isEmpty()){
                        lnovoAnime->vnomeAlternativo = lnomeAlternativo;
                        lnomeAlternativo.empty();
                    }
                    lnovoAnime->vnumEpisodiosTotais = lnumEpisodiosTotais;
                    if(!lnotaMediaSite.isEmpty())
                        lnovoAnime->vnotaMediaSite = lnotaMediaSite;
                    else
                        lnovoAnime->vnotaMediaSite = "-";
                    if(lano != "?" && lmes != "?"){
                        ldataEstreia.setDate(lano.toInt(), lmes.toInt(), 1);
                    }
                    else if(lmes != "?"){
                        if(lseason == "Winter")
                            lmes = "3";
                        else if(lseason == "Spring")
                            lmes = "6";
                        else if(lseason == "Summer")
                            lmes = "9";
                        else
                            lmes = "12";
                        if(lmes == "?")
                            lmes = "12";
                        ldataEstreia.setDate(lano.toInt(), lmes.toInt(), 31);
                    }
                    else if(lano != "?"){
                        if(lmes == "?")
                            lmes = "12";
                        ldataEstreia.setDate(2050, lmes.toInt(), 31);
                    }
                    if(fbuscaAnimePorIDERetornaLista(lid).isEmpty())
                        lnovoAnime->vlista = fbuscaAnimePorIDERetornaLista(lid)+" "+ lseason;
                    else
                        lnovoAnime->vlista = fbuscaAnimePorIDERetornaLista(lid);
                    lnovoAnime->vnumEpisodiosAssistidos = fbuscaAnimePorIDERetornaEpisodio(lid);
                    lnovoAnime->vnotaMediaPessoal = fbuscaAnimePorIDERetornaNota(lid);
                    lnovoAnime->vLinkImagemMedia = lLinkImagemMedia;
                    lnovoAnime->vstatus = lstatus;
                    lnovoAnime->vsinopse = lsinopse;
                    lnovoAnime->vid = lid;
                    lnovoAnime->vseason = lseason + " " + lano;
                    lnovoAnime->vnumProximoEpisodioLancado = lproximoEpisodio;
                    lnovoAnime->vformato = lformato;
                    lnovoAnime->vdataEstreia = ldataEstreia;
                    lnovoAnime->vdataEpisodio = ldataEpisodioFinal;
                    lnovoAnime->vsiteAnilist = lsiteAnilist;
                    lnovoAnime->vstreamCrunchyroll = lstreamCrunchyroll;
                    lnovoAnime->vtemporada = ltemporadaAnime;
                    return lnovoAnime;
                }
            }
        }
        lleJson.close();
    }
    return nullptr;
}

void leitorlistaanimes::fleListaIdsNomesAnos()
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
                vHashListaAnimesPorId.insert(id,lista);
                vHashPosicaoAnimesPorId.insert(id,posicaoLista.toInt());
                vHashNomeAnimesPorId.insert(id,lidListaNome);
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
                vHashListaMangasPorId.insert(id,lista);
                vHashPosicaoMangasPorId.insert(id,posicaoLista.toInt());
                vHashNomeMangasPorId.insert(id,lidListaNome);
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
                vHashListaNovelsPorId.insert(id,lista);
                vHashPosicaoNovelsPorId.insert(id,posicaoLista.toInt());
                vHashNomeNovelsPorId.insert(id,lidListaNome);
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
                vHashSizeListasPorAno.insert(ano.toInt(),size);
            }
            larquivoSizePorAno.close();
        }
    }
}

void leitorlistaanimes::fsalvaListaIdsNomesAnos()
{
    QFile larquivo("Configurações/idListaNome.txt");
    if(larquivo.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivo);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vHashNomeAnimesPorId.keys()){
            lstreamTexto << key << ";" << vHashListaAnimesPorId[key] << ";" << vHashPosicaoAnimesPorId[key] << ";" <<
                            vHashNomeAnimesPorId[key].join(";").trimmed() << endl;
        }
        larquivo.close();
    }
    QFile larquivoManga("Configurações/idListaNomeManga.txt");
    if(larquivoManga.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivoManga);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vHashNomeMangasPorId.keys()){
            lstreamTexto << key << ";" << vHashListaMangasPorId[key] << ";" << vHashPosicaoMangasPorId[key] << ";" <<
                            vHashNomeMangasPorId[key].join(";").trimmed() << endl;
        }
        larquivoManga.close();
    }
    QFile larquivoNovel("Configurações/idListaNomeNovel.txt");
    if(larquivoNovel.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivoNovel);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vHashNomeNovelsPorId.keys()){
            lstreamTexto << key << ";" << vHashListaNovelsPorId[key] << ";" << vHashPosicaoNovelsPorId[key] << ";" <<
                            vHashNomeNovelsPorId[key].join(";").trimmed() << endl;
        }
        larquivoNovel.close();
    }
    QFile larquivoSizePorAno("Configurações/Temp/sizePerYear.txt");
    if(larquivoSizePorAno.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&larquivoSizePorAno);
        lstreamTexto.setCodec("UTF-8");
        foreach(int key, vHashSizeListasPorAno.keys()){
            lstreamTexto << QString::number(key) << ";" << vHashSizeListasPorAno[key] << endl;
        }
        larquivoSizePorAno.close();
    }
}

void leitorlistaanimes::fcarregaListaAnoEmThread()
{
    //Essa função serve para saber se os animes do ano x já estão salvos na lista de ids.
    //Quando eles já estão salvos, o ano vai ser salvo no hash.
    //Isso é importante para a função de download de torrents, onde eles vão buscar o id dos animes.
    for(int i = 0; i < QDate::currentDate().year() - 1999; i++){
        if(!vHashSizeListasPorAno.contains(2000+i)){
            fleListaAno(2000+i);
        }
    }
}


void leitorlistaanimes::fdeletaListaAnimes(){
    if(!vlistaWatching.isEmpty()){
        qDeleteAll(vlistaWatching.begin(),vlistaWatching.end()); //Deletar todas as listas
        vlistaWatching.clear();
    }
    if(!vlistaCompleted.isEmpty()){
        qDeleteAll(vlistaCompleted.begin(),vlistaCompleted.end()); //Deletar todas as listas
        vlistaCompleted.clear();
    }
    if(!vlistaDropped.isEmpty()){
        qDeleteAll(vlistaDropped.begin(),vlistaDropped.end()); //Deletar todas as listas
        vlistaDropped.clear();
    }
    if(!vlistaOnHold.isEmpty()){
        qDeleteAll(vlistaOnHold.begin(),vlistaOnHold.end()); //Deletar todas as listas
        vlistaOnHold.clear();
    }
    if(!vlistaPlanToWatch.isEmpty()){
        qDeleteAll(vlistaPlanToWatch.begin(),vlistaPlanToWatch.end()); //Deletar todas as listas
        vlistaPlanToWatch.clear();
    }
//    if(!vlistaBusca.isEmpty()){
//        qDeleteAll(vlistaBusca.begin(),vlistaBusca.end()); //Deletar todas as listas
//        vlistaBusca.clear();
//    }
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

QString leitorlistaanimes::fprocuraAnimeNasListas(const QString &rnomeAnime)
{
    //Essa função é importante por que se ela não encontrar o nome certo logo de cara, vai fazer uma busca mais detalhada e adicionar
    //o nome procurado nas informações do anime, quando encontrar.
    foreach(QString key, vHashNomeAnimesPorId.keys()){
        for(int i = 0; i < vHashNomeAnimesPorId[key].size(); i++){
            if(vHashNomeAnimesPorId[key].at(i) == rnomeAnime){
                qDebug() << vHashNomeAnimesPorId[key].at(i);
                return key;
            }
        }
    }
    QStringList tempNomeAnime;
    for(int i = 0; i < vlistaWatching.size(); i++){
        if(formatador.fcomparaNomes(vlistaWatching[i]->vnome, rnomeAnime) ||
                formatador.fcomparaNomes(vlistaWatching[i]->vnomeIngles, rnomeAnime)){
            tempNomeAnime.append(rnomeAnime);
            qDebug() << vlistaWatching[i]->vnome;
            if(!vHashNomeAnimesPorId.contains(vlistaWatching[i]->vid))
                vHashNomeAnimesPorId.insert(vlistaWatching[i]->vid, tempNomeAnime);
            else
                vHashNomeAnimesPorId[vlistaWatching[i]->vid].append(tempNomeAnime);
            return vlistaWatching[i]->vid;
        }
        else if(!vlistaWatching[i]->vnomeAlternativo.isEmpty()){
            for(int w = 0; w < vlistaWatching[i]->vnomeAlternativo.size(); w++){
                if(formatador.fcomparaNomes(vlistaWatching[i]->vnomeAlternativo.at(w), rnomeAnime)){
                    tempNomeAnime.append(rnomeAnime);
                    if(!vHashNomeAnimesPorId.contains(vlistaWatching[i]->vid))
                        vHashNomeAnimesPorId.insert(vlistaWatching[i]->vid, tempNomeAnime);
                    else
                        vHashNomeAnimesPorId[vlistaWatching[i]->vid].append(tempNomeAnime);
                    return vlistaWatching[i]->vid;
                }
            }
        }
    }
    for(int i = 0; i < vlistaPlanToWatch.size(); i++){
        if(formatador.fcomparaNomes(vlistaPlanToWatch[i]->vnome, rnomeAnime) ||
                formatador.fcomparaNomes(vlistaPlanToWatch[i]->vnomeIngles, rnomeAnime)){
            tempNomeAnime.append(rnomeAnime);
            if(!vHashNomeAnimesPorId.contains(vlistaPlanToWatch[i]->vid))
                vHashNomeAnimesPorId.insert(vlistaPlanToWatch[i]->vid, tempNomeAnime);
            else
                vHashNomeAnimesPorId[vlistaPlanToWatch[i]->vid].append(tempNomeAnime);
            return vlistaPlanToWatch[i]->vid;
        }
        else if(!vlistaPlanToWatch[i]->vnomeAlternativo.isEmpty()){
            for(int w = 0; w < vlistaPlanToWatch[i]->vnomeAlternativo.size(); w++){
                if(formatador.fcomparaNomes(vlistaPlanToWatch[i]->vnomeAlternativo.at(w), rnomeAnime)){
                    tempNomeAnime.append(rnomeAnime);
                    if(!vHashNomeAnimesPorId.contains(vlistaPlanToWatch[i]->vid))
                        vHashNomeAnimesPorId.insert(vlistaPlanToWatch[i]->vid, tempNomeAnime);
                    else
                        vHashNomeAnimesPorId[vlistaPlanToWatch[i]->vid].append(tempNomeAnime);
                    return vlistaPlanToWatch[i]->vid;
                }
            }
        }
    }
    for(int i = 0; i < vlistaOnHold.size(); i++){
        if(formatador.fcomparaNomes(vlistaOnHold[i]->vnome, rnomeAnime) ||
                formatador.fcomparaNomes(vlistaOnHold[i]->vnomeIngles, rnomeAnime)){
            tempNomeAnime.append(rnomeAnime);
            if(!vHashNomeAnimesPorId.contains(vlistaOnHold[i]->vid))
                vHashNomeAnimesPorId.insert(vlistaOnHold[i]->vid, tempNomeAnime);
            else
                vHashNomeAnimesPorId[vlistaOnHold[i]->vid].append(tempNomeAnime);
            return vlistaOnHold[i]->vid;
        }
        else if(!vlistaOnHold[i]->vnomeAlternativo.isEmpty()){
            for(int w = 0; w < vlistaOnHold[i]->vnomeAlternativo.size(); w++){
                if(formatador.fcomparaNomes(vlistaOnHold[i]->vnomeAlternativo.at(w), rnomeAnime)){
                    tempNomeAnime.append(rnomeAnime);
                    if(!vHashNomeAnimesPorId.contains(vlistaOnHold[i]->vid))
                        vHashNomeAnimesPorId.insert(vlistaOnHold[i]->vid, tempNomeAnime);
                    else
                        vHashNomeAnimesPorId[vlistaOnHold[i]->vid].append(tempNomeAnime);
                    return vlistaOnHold[i]->vid;
                }
            }
        }
    }
    for(int i = 0; i < vlistaDropped.size(); i++){
        if(formatador.fcomparaNomes(vlistaDropped[i]->vnome, rnomeAnime) ||
                formatador.fcomparaNomes(vlistaDropped[i]->vnomeIngles, rnomeAnime)){
            tempNomeAnime.append(rnomeAnime);
            if(!vHashNomeAnimesPorId.contains(vlistaDropped[i]->vid))
                vHashNomeAnimesPorId.insert(vlistaDropped[i]->vid, tempNomeAnime);
            else
                vHashNomeAnimesPorId[vlistaDropped[i]->vid].append(tempNomeAnime);
            return vlistaDropped[i]->vid;
        }
        else if(!vlistaDropped[i]->vnomeAlternativo.isEmpty()){
            for(int w = 0; w < vlistaDropped[i]->vnomeAlternativo.size(); w++){
                if(formatador.fcomparaNomes(vlistaDropped[i]->vnomeAlternativo.at(w), rnomeAnime)){
                    tempNomeAnime.append(rnomeAnime);
                    if(!vHashNomeAnimesPorId.contains(vlistaDropped[i]->vid))
                        vHashNomeAnimesPorId.insert(vlistaDropped[i]->vid, tempNomeAnime);
                    else
                        vHashNomeAnimesPorId[vlistaDropped[i]->vid].append(tempNomeAnime);
                    return vlistaDropped[i]->vid;
                }
            }
        }
    }
    for(int i = 0; i < vlistaCompleted.size(); i++){
        if(formatador.fcomparaNomes(vlistaCompleted[i]->vnome, rnomeAnime) ||
                formatador.fcomparaNomes(vlistaCompleted[i]->vnomeIngles, rnomeAnime)){
            tempNomeAnime.append(rnomeAnime);
            if(!vHashNomeAnimesPorId.contains(vlistaCompleted[i]->vid))
                vHashNomeAnimesPorId.insert(vlistaCompleted[i]->vid, tempNomeAnime);
            else
                vHashNomeAnimesPorId[vlistaCompleted[i]->vid].append(tempNomeAnime);
            return vlistaCompleted[i]->vid;
        }
        else if(!vlistaCompleted[i]->vnomeAlternativo.isEmpty()){
            for(int w = 0; w < vlistaCompleted[i]->vnomeAlternativo.size(); w++){
                if(formatador.fcomparaNomes(vlistaCompleted[i]->vnomeAlternativo.at(w), rnomeAnime)){
                    tempNomeAnime.append(rnomeAnime);
                    if(!vHashNomeAnimesPorId.contains(vlistaCompleted[i]->vid))
                        vHashNomeAnimesPorId.insert(vlistaCompleted[i]->vid, tempNomeAnime);
                    else
                        vHashNomeAnimesPorId[vlistaCompleted[i]->vid].append(tempNomeAnime);
                    return vlistaCompleted[i]->vid;
                }
            }
        }
    }
    return "";
}

QString leitorlistaanimes::fprocuraIdNasListasRapido(const QString &rnomeAnime)
{
    foreach(QString key, vHashNomeAnimesPorId.keys()){
        for(int i = 0; i < vHashNomeAnimesPorId[key].size(); i++){
            if(vHashNomeAnimesPorId[key].at(i) == rnomeAnime)
                return key;
        }
    }
    return "";
}

void leitorlistaanimes::finsereNomeAlternativo(const QString &rid, const QStringList &rlistaNomesAlternativos)
{
    QString llista = fbuscaAnimePorIDERetornaLista(rid);
    int lposicao = fbuscaAnimePorIDERetornaPosicao(rid);
    if(lposicao == -1)
        return;

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        vlistaWatching[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        vlistaPlanToWatch[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        vlistaOnHold[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        vlistaDropped[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        vlistaCompleted[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    if(llista.compare("Reading(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaReading[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Plan to Read(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaPlanToRead[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("On Hold(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaOnHold[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Dropped(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaDropped[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Completed(Manga)", Qt::CaseInsensitive) == 0){
        vlistaMangaCompleted[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    if(llista.compare("Reading(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelReading[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Plan to Read(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelPlanToRead[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("On Hold(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelOnHold[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Dropped(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelDropped[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
    else if(llista.compare("Completed(Novel)", Qt::CaseInsensitive) == 0){
        vlistaNovelCompleted[lposicao]->vnomeAlternativo.append(rlistaNomesAlternativos);
        vHashNomeAnimesPorId[rid].append(rlistaNomesAlternativos);
    }
}

bool leitorlistaanimes::fdeletedaLista(const QString &rid)
{
    QString llista = fbuscaAnimePorIDERetornaLista(rid);
    int lposicao = fbuscaAnimePorIDERetornaPosicao(rid);
    if(lposicao == -1)
        return false;

    if(llista == "Watching"){
        vHashListaAnimesPorId.insert(rid, QString::number(vlistaWatching[lposicao]->vdataEstreia.year()));
        vHashPosicaoAnimesPorId.insert(rid,0);
        vlistaWatching.remove(lposicao);
        return true;
    }
    else if(llista == "Completed"){
        vHashListaAnimesPorId.insert(rid, QString::number(vlistaCompleted[lposicao]->vdataEstreia.year()));
        vHashPosicaoAnimesPorId.insert(rid,0);
        vlistaCompleted.remove(lposicao);
        return true;
    }
    else if(llista == "On Hold"){
        vHashListaAnimesPorId.insert(rid, QString::number(vlistaOnHold[lposicao]->vdataEstreia.year()));
        vHashPosicaoAnimesPorId.insert(rid,0);
        vlistaOnHold.remove(lposicao);
        return true;
    }
    else if(llista == "Dropped"){
        vHashListaAnimesPorId.insert(rid, QString::number(vlistaDropped[lposicao]->vdataEstreia.year()));
        vHashPosicaoAnimesPorId.insert(rid,0);
        vlistaDropped.remove(lposicao);
        return true;
    }
    else if(llista == "Plan to Watch"){
        vHashListaAnimesPorId.insert(rid, QString::number(vlistaPlanToWatch[lposicao]->vdataEstreia.year()));
        vHashPosicaoAnimesPorId.insert(rid,0);
        vlistaPlanToWatch.remove(lposicao);
        return true;
    }
    else if(llista == "Reading(Manga)"){
        vHashListaMangasPorId.insert(rid, QString::number(vlistaMangaReading[lposicao]->vdataEstreia.year()));
        vHashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaReading.remove(lposicao);
        return true;
    }
    else if(llista == "Completed(Manga)"){
        vHashListaMangasPorId.insert(rid, QString::number(vlistaMangaCompleted[lposicao]->vdataEstreia.year()));
        vHashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaCompleted.remove(lposicao);
        return true;
    }
    else if(llista == "On Hold(Manga)"){
        vHashListaMangasPorId.insert(rid, QString::number(vlistaMangaOnHold[lposicao]->vdataEstreia.year()));
        vHashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaOnHold.remove(lposicao);
        return true;
    }
    else if(llista == "Dropped(Manga)"){
        vHashListaMangasPorId.insert(rid, QString::number(vlistaMangaDropped[lposicao]->vdataEstreia.year()));
        vHashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaDropped.remove(lposicao);
        return true;
    }
    else if(llista == "Plan to Read(Manga)"){
        vHashListaMangasPorId.insert(rid, QString::number(vlistaMangaPlanToRead[lposicao]->vdataEstreia.year()));
        vHashPosicaoMangasPorId.insert(rid,0);
        vlistaMangaPlanToRead.remove(lposicao);
        return true;
    }
    else if(llista == "Reading(Novel)"){
        vHashListaNovelsPorId.insert(rid, QString::number(vlistaNovelReading[lposicao]->vdataEstreia.year()));
        vHashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelReading.remove(lposicao);
        return true;
    }
    else if(llista == "Completed(Novel)"){
        vHashListaNovelsPorId.insert(rid, QString::number(vlistaNovelCompleted[lposicao]->vdataEstreia.year()));
        vHashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelCompleted.remove(lposicao);
        return true;
    }
    else if(llista == "On Hold(Novel)"){
        vHashListaNovelsPorId.insert(rid, QString::number(vlistaNovelOnHold[lposicao]->vdataEstreia.year()));
        vHashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelOnHold.remove(lposicao);
        return true;
    }
    else if(llista == "Dropped(Novel)"){
        vHashListaNovelsPorId.insert(rid, QString::number(vlistaNovelDropped[lposicao]->vdataEstreia.year()));
        vHashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelDropped.remove(lposicao);
        return true;
    }
    else if(llista == "Plan to Read(Novel)"){
        vHashListaNovelsPorId.insert(rid, QString::number(vlistaNovelPlanToRead[lposicao]->vdataEstreia.year()));
        vHashPosicaoNovelsPorId.insert(rid,0);
        vlistaNovelPlanToRead.remove(lposicao);
        return true;
    }
    return false;

}

QVector<anime *> leitorlistaanimes::sortLista(const QString &rordem, QString rlista){
    if(rlista == "watching")
        llistaTemp = vlistaWatching;
    else if(rlista == "completed")
        llistaTemp = vlistaCompleted;
    else if(rlista == "onhold")
        llistaTemp = vlistaOnHold;
    else if(rlista == "dropped")
        llistaTemp = vlistaDropped;
    else if(rlista == "plantowatch")
        llistaTemp = vlistaPlanToWatch;
    else if(rlista == "busca")
        llistaTemp = vlistaBusca;
    else if(rlista == "mangareading")
        llistaTemp = vlistaMangaReading;
    else if(rlista == "mangacompleted")
        llistaTemp = vlistaMangaCompleted;
    else if(rlista == "mangaonhold")
        llistaTemp = vlistaMangaOnHold;
    else if(rlista == "mangadropped")
        llistaTemp = vlistaMangaDropped;
    else if(rlista == "mangaplantoread")
        llistaTemp = vlistaMangaPlanToRead;
    else if(rlista == "novelreading")
        llistaTemp = vlistaNovelReading;
    else if(rlista == "novelcompleted")
        llistaTemp = vlistaNovelCompleted;
    else if(rlista == "novelonhold")
        llistaTemp = vlistaNovelOnHold;
    else if(rlista == "noveldropped")
        llistaTemp = vlistaNovelDropped;
    else if(rlista == "novelplantoread")
        llistaTemp = vlistaNovelPlanToRead;
    else if(rlista.contains("season", Qt::CaseInsensitive)){
        rlista = rlista.remove("season");
        if(rlista.contains("Winter", Qt::CaseInsensitive)){
            rlista = rlista.remove("Winter");
            llistaTemp = fleListaAnoSeason(rlista.toInt(), "winter");
        }
        else if(rlista.contains("Spring", Qt::CaseInsensitive)){
            rlista = rlista.remove("Spring");
            llistaTemp = fleListaAnoSeason(rlista.toInt(), "spring");
        }
        else if(rlista.contains("Summer", Qt::CaseInsensitive)){
            rlista = rlista.remove("Summer");
            llistaTemp = fleListaAnoSeason(rlista.toInt(), "summer");
        }
        else if(rlista.contains("Fall", Qt::CaseInsensitive)){
            rlista = rlista.remove("Fall");
            llistaTemp = fleListaAnoSeason(rlista.toInt(), "fall");
        }
        else
            llistaTemp = fleListaAno(rlista.toInt());
    }
    else
        llistaTemp.clear();


    if(!rordem.isEmpty())
    {
        if(rordem.startsWith("c")){
            if(rordem.contains("nome", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vnome < b->vnome ;});
            }
            else if(rordem.contains("data", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vdataEstreia < b->vdataEstreia ;});
            }
            else if(rordem.contains("progresso", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{return
                            a->vnumEpisodiosAssistidos.toInt()
                            < b->vnumEpisodiosAssistidos.toInt() ;});
            }
            else if(rordem.contains("nota", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vnotaMediaPessoal.toInt()
                            < b->vnotaMediaPessoal.toInt() ;});
            }
            else if(rordem.contains("formato", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vformato < b->vformato ;});
            }
        }
        else{
            if(rordem.contains("nome", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vnome > b->vnome ;});
            }
            else if(rordem.contains("data", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vdataEstreia > b->vdataEstreia ;});
            }
            else if(rordem.contains("progresso", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{return
                            static_cast<float>(a->vnumEpisodiosAssistidos.toInt())
                            > static_cast<float>(b->vnumEpisodiosAssistidos.toInt()) ;});
            }
            else if(rordem.contains("nota", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vnotaMediaPessoal.toInt()
                            > b->vnotaMediaPessoal.toInt() ;});
            }
            else if(rordem.contains("formato", Qt::CaseInsensitive)){
                std::sort(llistaTemp.begin(),llistaTemp.end(),[](anime* a, anime* b)->bool{
                    return a->vformato > b->vformato ;});
            }
        }
    }
    return llistaTemp;
}

QVector<anime *> leitorlistaanimes::fbuscaLista(const QString &rnome, const QString &rtipoMidia){
    vlistaBusca.clear();
    if(rtipoMidia.compare("anime", Qt::CaseInsensitive) == 0){
        foreach(QString key, vHashNomeAnimesPorId.keys()){
            for(int i = 0; i < vHashNomeAnimesPorId[key].size(); i++){
                if(vHashNomeAnimesPorId[key].at(i).contains(rnome, Qt::CaseInsensitive)){
                    QString lista = vHashListaAnimesPorId[key];
                    int pos = vHashPosicaoAnimesPorId[key];
                    if(pos == -1)
                        break;
                    else if(lista.compare("Watching", Qt::CaseInsensitive) == 0 && vlistaWatching.size() > pos){
                        vlistaBusca.append(vlistaWatching[pos]);
                        break;
                    }
                    else if(lista.compare("Plan to Watch", Qt::CaseInsensitive) == 0 && vlistaPlanToWatch.size() > pos){
                        vlistaBusca.append(vlistaPlanToWatch[pos]);
                        break;
                    }
                    else if(lista.compare("On Hold", Qt::CaseInsensitive) == 0 && vlistaOnHold.size() > pos){
                        vlistaBusca.append(vlistaOnHold[pos]);
                        break;
                    }
                    else if(lista.compare("Completed", Qt::CaseInsensitive) == 0 && vlistaCompleted.size() > pos){
                        vlistaBusca.append(vlistaCompleted[pos]);
                        break;
                    }
                    else if(lista.compare("Dropped", Qt::CaseInsensitive) == 0 && vlistaDropped.size() > pos){
                        vlistaBusca.append(vlistaDropped[pos]);
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
    else if(rtipoMidia.compare("manga", Qt::CaseInsensitive) == 0){
        foreach(QString key, vHashNomeMangasPorId.keys()){
            for(int i = 0; i < vHashNomeMangasPorId[key].size(); i++){
                if(vHashNomeMangasPorId[key].at(i).contains(rnome, Qt::CaseInsensitive)){
                    QString lista = vHashListaMangasPorId[key];
                    int pos = vHashPosicaoMangasPorId[key];
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
    else if(rtipoMidia.compare("novel", Qt::CaseInsensitive) == 0){
        foreach(QString key, vHashNomeNovelsPorId.keys()){
            for(int i = 0; i < vHashNomeNovelsPorId[key].size(); i++){
                if(vHashNomeNovelsPorId[key].at(i).contains(rnome, Qt::CaseInsensitive)){
                    QString lista = vHashListaNovelsPorId[key];
                    int pos = vHashPosicaoNovelsPorId[key];
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

QString leitorlistaanimes::fbuscaAnimePorIDERetornaEpisodio(const QString &rid)
{   
    QString llista = fbuscaAnimePorIDERetornaLista(rid);
    int lposicao = fbuscaAnimePorIDERetornaPosicao(rid);
    if(lposicao == -1)
        return "0";

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        if(vlistaWatching.size() > lposicao)
            return vlistaWatching[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        if(vlistaPlanToWatch.size() > lposicao)
            return vlistaPlanToWatch[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        if(vlistaOnHold.size() > lposicao)
            return vlistaOnHold[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        if(vlistaCompleted.size() > lposicao)
            return vlistaCompleted[lposicao]->vnumEpisodiosAssistidos;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        if(vlistaDropped.size() > lposicao)
            return vlistaDropped[lposicao]->vnumEpisodiosAssistidos;
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

QString leitorlistaanimes::fbuscaAnimePorIDERetornaNota(const QString &rid)
{
    QString llista = fbuscaAnimePorIDERetornaLista(rid);
    int lposicao = fbuscaAnimePorIDERetornaPosicao(rid);
    if(lposicao == -1)
        return "0";

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        if(vlistaWatching.size() > lposicao)
            return vlistaWatching[lposicao]->vnotaMediaPessoal;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        if(vlistaPlanToWatch.size() > lposicao)
            return vlistaPlanToWatch[lposicao]->vnotaMediaPessoal;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        if(vlistaOnHold.size() > lposicao)
            return vlistaOnHold[lposicao]->vnotaMediaPessoal;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        if(vlistaCompleted.size() > lposicao)
            return vlistaCompleted[lposicao]->vnotaMediaPessoal;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        if(vlistaDropped.size() > lposicao)
            return vlistaDropped[lposicao]->vnotaMediaPessoal;
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

QString leitorlistaanimes::fbuscaAnimePorIDERetornaTitulo(const QString &rid)
{
    QString llista = fbuscaAnimePorIDERetornaLista(rid);
    int lposicao = fbuscaAnimePorIDERetornaPosicao(rid);
    if(lposicao == -1)
        return "0";

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        if(vlistaWatching.size() > lposicao)
            return vlistaWatching[lposicao]->vnome;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        if(vlistaPlanToWatch.size() > lposicao)
            return vlistaPlanToWatch[lposicao]->vnome;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        if(vlistaOnHold.size() > lposicao)
            return vlistaOnHold[lposicao]->vnome;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        if(vlistaCompleted.size() > lposicao)
            return vlistaCompleted[lposicao]->vnome;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        if(vlistaDropped.size() > lposicao)
            return vlistaDropped[lposicao]->vnome;
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

QString leitorlistaanimes::fbuscaAnimePorIDERetornaLista(const QString &ridAnime)
{
    if(vHashListaAnimesPorId.contains(ridAnime))
        return vHashListaAnimesPorId[ridAnime];
    else if(vHashListaMangasPorId.contains(ridAnime))
        return vHashListaMangasPorId[ridAnime];
    else if(vHashListaNovelsPorId.contains(ridAnime))
        return vHashListaNovelsPorId[ridAnime];
    return "";
}

int leitorlistaanimes::fbuscaAnimePorIDERetornaPosicao(const QString &ridAnime)
{
    QString llista = fbuscaAnimePorIDERetornaLista(ridAnime);
    if(vHashPosicaoAnimesPorId.contains(ridAnime)){
        int lposicao = vHashPosicaoAnimesPorId[ridAnime];
        if(llista.compare("Watching", Qt::CaseInsensitive) == 0 && (vlistaWatching.size() <= lposicao ||
                vlistaWatching[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaWatching.size(); i++){
                if(vlistaWatching[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoAnimesPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0 && (vlistaPlanToWatch.size() <= lposicao ||
                    vlistaPlanToWatch[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaPlanToWatch.size(); i++){
                if(vlistaPlanToWatch[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoAnimesPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0 && (vlistaOnHold.size() <= lposicao ||
                    vlistaOnHold[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaOnHold.size(); i++){
                if(vlistaOnHold[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoAnimesPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Completed", Qt::CaseInsensitive) == 0 && (vlistaCompleted.size() <= lposicao ||
                    vlistaCompleted[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaCompleted.size(); i++){
                if(vlistaCompleted[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoAnimesPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0 && (vlistaDropped.size() <= lposicao ||
                    vlistaDropped[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaDropped.size(); i++){
                if(vlistaDropped[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoAnimesPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        return vHashPosicaoAnimesPorId[ridAnime];
    }
    else if(vHashPosicaoMangasPorId.contains(ridAnime)){
        int lposicao = vHashPosicaoMangasPorId[ridAnime];
        if(llista.compare("Reading(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaReading.size() <= lposicao ||
                    vlistaMangaReading[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaMangaReading.size(); i++){
                if(vlistaMangaReading[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoMangasPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Completed(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaCompleted.size() <= lposicao ||
                    vlistaMangaCompleted[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaMangaCompleted.size(); i++){
                if(vlistaMangaCompleted[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoMangasPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("On Hold(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaOnHold.size() <= lposicao ||
                    vlistaMangaOnHold[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaMangaOnHold.size(); i++){
                if(vlistaMangaOnHold[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoMangasPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Dropped(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaDropped.size() <= lposicao ||
                    vlistaMangaDropped[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaMangaDropped.size(); i++){
                if(vlistaMangaDropped[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoMangasPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Plan to Read(Manga)", Qt::CaseInsensitive) == 0 && (vlistaMangaPlanToRead.size() <= lposicao ||
                    vlistaMangaPlanToRead[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaMangaPlanToRead.size(); i++){
                if(vlistaMangaPlanToRead[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoMangasPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        return vHashPosicaoMangasPorId[ridAnime];
    }
    else if(vHashPosicaoNovelsPorId.contains(ridAnime)){
        int lposicao = vHashPosicaoNovelsPorId[ridAnime];
        if(llista.compare("Reading(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelReading.size() <= lposicao ||
                    vlistaNovelReading[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaNovelReading.size(); i++){
                if(vlistaNovelReading[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoNovelsPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Completed(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelCompleted.size() <= lposicao ||
                    vlistaNovelCompleted[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaNovelCompleted.size(); i++){
                if(vlistaNovelCompleted[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoNovelsPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("On Hold(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelOnHold.size() <= lposicao ||
                    vlistaNovelOnHold[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaNovelOnHold.size(); i++){
                if(vlistaNovelOnHold[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoNovelsPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Plan to Read(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelDropped.size() <= lposicao ||
                    vlistaNovelDropped[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaNovelDropped.size(); i++){
                if(vlistaNovelDropped[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoNovelsPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        else if(llista.compare("Dropped(Novel)", Qt::CaseInsensitive) == 0 && (vlistaNovelPlanToRead.size() <= lposicao ||
                    vlistaNovelPlanToRead[lposicao]->vid.compare(ridAnime) != 0)){
            for(int i = 0; i < vlistaNovelPlanToRead.size(); i++){
                if(vlistaNovelPlanToRead[i]->vid.compare(ridAnime) == 0){
                    vHashPosicaoNovelsPorId.insert(ridAnime,i);
                    return i;
                }
            }
        }
        return vHashPosicaoNovelsPorId[ridAnime];
    }
    return -1;
}

QVector<anime*> leitorlistaanimes::retornaListaWatching()
{
    return vlistaWatching;
}

QVector<anime *> leitorlistaanimes::retornaListaCompleted()
{
    return vlistaCompleted;
}

QVector<anime *> leitorlistaanimes::retornaListaOnHold()
{
    return vlistaOnHold;
}

QVector<anime *> leitorlistaanimes::retornaListaDropped()
{
    return vlistaDropped;
}

QVector<anime *> leitorlistaanimes::retornaListaPlanToWatch()
{
    return vlistaPlanToWatch;
}

QVector<anime*> leitorlistaanimes::retornaListaMangaReading()
{
    return vlistaMangaReading;
}

QVector<anime *> leitorlistaanimes::retornaListaMangaCompleted()
{
    return vlistaMangaCompleted;
}

QVector<anime *> leitorlistaanimes::retornaListaMangaOnHold()
{
    return vlistaMangaOnHold;
}

QVector<anime *> leitorlistaanimes::retornaListaMangaDropped()
{
    return vlistaMangaDropped;
}

QVector<anime *> leitorlistaanimes::retornaListaMangaPlanToRead()
{
    return vlistaMangaPlanToRead;
}

QVector<anime*> leitorlistaanimes::retornaListaNovelReading()
{
    return vlistaNovelReading;
}

QVector<anime *> leitorlistaanimes::retornaListaNovelCompleted()
{
    return vlistaNovelCompleted;
}

QVector<anime *> leitorlistaanimes::retornaListaNovelOnHold()
{
    return vlistaNovelOnHold;
}

QVector<anime *> leitorlistaanimes::retornaListaNovelDropped()
{
    return vlistaNovelDropped;
}

QVector<anime *> leitorlistaanimes::retornaListaNovelPlanToRead(){
    return vlistaNovelPlanToRead;
}

bool leitorlistaanimes::fmudaNota(const QString &rid, const QString &rnota){
    QString llista = fbuscaAnimePorIDERetornaLista(rid);
    int lposicao = fbuscaAnimePorIDERetornaPosicao(rid);
    if(lposicao == -1)
        return false;
    bool ok;
    //Checa se a lista é um número válido
    llista.toInt(&ok);

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        vlistaWatching[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        vlistaCompleted[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        vlistaOnHold[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        vlistaDropped[lposicao]->vnotaMediaPessoal = rnota;
        return true;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        vlistaPlanToWatch[lposicao]->vnotaMediaPessoal = rnota;
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

bool leitorlistaanimes::fmudaProgresso(const QString &rid, const QString &rprogresso){
    QString llista = fbuscaAnimePorIDERetornaLista(rid);
    int lposicao = fbuscaAnimePorIDERetornaPosicao(rid);
    if(lposicao == -1)
        return false;
    bool ok;
    //Checa se a lista é um número válido
    llista.toInt(&ok);

    if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
        vlistaWatching[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
        vlistaCompleted[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
        vlistaOnHold[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
        vlistaDropped[lposicao]->vnumEpisodiosAssistidos = rprogresso;
        return true;
    }
    else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
        vlistaPlanToWatch[lposicao]->vnumEpisodiosAssistidos = rprogresso;
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

bool leitorlistaanimes::fmudaLista(const QString &rid, const QString &rlista, const QString &rtipo){
    QString llista = fbuscaAnimePorIDERetornaLista(rid);
    int lposicao = fbuscaAnimePorIDERetornaPosicao(rid);
    if(lposicao == -1)
        return false;
    bool ok;
    //Checa se a lista é um número válido
    llista.toInt(&ok);

    if(rtipo.compare("anime", Qt::CaseInsensitive) == 0){
        if(llista.compare("Watching", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaWatching[lposicao];
            if(rlista.compare("Completed") == 0){
                lnovoAnime->vlista = "Completed";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                vlistaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaCompleted.size()-1);
            }
            else if(rlista.compare("Dropped") == 0){
                lnovoAnime->vlista = "Dropped";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                vlistaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaDropped.size()-1);
            }
            else if(rlista.compare("On Hold") == 0){
                lnovoAnime->vlista = "On Hold";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                vlistaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Watch") == 0){
                lnovoAnime->vlista = "Plan to Watch";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                vlistaPlanToWatch.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaPlanToWatch.size()-1);
            }
            vlistaWatching.remove(lposicao);
            return true;
        }
        else if(llista.compare("Completed", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaCompleted[lposicao];
            if(rlista.compare("Watching") == 0){
                lnovoAnime->vlista = "Watching";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Watching");
                vlistaWatching.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaWatching.size()-1);
            }
            else if(rlista.compare("Dropped") == 0){
                lnovoAnime->vlista = "Dropped";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                vlistaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaDropped.size()-1);
            }
            else if(rlista.compare("On Hold") == 0){
                lnovoAnime->vlista = "On Hold";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                vlistaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Watch") == 0){
                lnovoAnime->vlista = "Plan to Watch";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                vlistaPlanToWatch.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaPlanToWatch.size()-1);
            }
            vlistaCompleted.remove(lposicao);
            return true;
        }
        else if(llista.compare("On Hold", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaOnHold[lposicao];
            if(rlista.compare("Watching") == 0){
                lnovoAnime->vlista = "Watching";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Watching");
                vlistaWatching.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaWatching.size()-1);
            }
            else if(rlista.compare("Completed") == 0){
                lnovoAnime->vlista = "Completed";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                vlistaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaCompleted.size()-1);
            }
            else if(rlista.compare("Dropped") == 0){
                lnovoAnime->vlista = "Dropped";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                vlistaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaDropped.size()-1);
            }
            else if(rlista.compare("Plan to Watch") == 0){
                lnovoAnime->vlista = "Plan to Watch";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                vlistaPlanToWatch.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaPlanToWatch.size()-1);
            }
            vlistaOnHold.remove(lposicao);
            return true;
        }
        else if(llista.compare("Dropped", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaDropped[lposicao];
            if(rlista.compare("Watching") == 0){
                lnovoAnime->vlista = "Watching";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Watching");
                vlistaWatching.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaWatching.size()-1);
            }
            else if(rlista.compare("Completed") == 0){
                lnovoAnime->vlista = "Completed";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                vlistaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaCompleted.size()-1);
            }
            else if(rlista.compare("On Hold") == 0){
                lnovoAnime->vlista = "On Hold";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                vlistaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Watch") == 0){
                lnovoAnime->vlista = "Plan to Watch";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                vlistaPlanToWatch.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaPlanToWatch.size()-1);
            }
            vlistaDropped.remove(lposicao);
            return true;
        }
        else if(llista.compare("Plan to Watch", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaPlanToWatch[lposicao];
            if(rlista.compare("Watching") == 0){
                lnovoAnime->vlista = "Watching";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Watching");
                vlistaWatching.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaWatching.size()-1);
            }
            else if(rlista.compare("Completed") == 0){
                lnovoAnime->vlista = "Completed";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                vlistaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaCompleted.size()-1);
            }
            else if(rlista.compare("On Hold") == 0){
                lnovoAnime->vlista = "On Hold";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                vlistaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaOnHold.size()-1);
            }
            else if(rlista.compare("Dropped") == 0){
                lnovoAnime->vlista = "Dropped";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                vlistaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaDropped.size()-1);
            }
            vlistaPlanToWatch.remove(lposicao);
            return true;
        }
        else if(ok){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = fbuscaAnimeNoAno(llista.toInt(), rid);
            if(rlista.compare("Watching") == 0){
                lnovoAnime->vlista = "Watching";
                qDebug() << vHashListaAnimesPorId[rid];
                vHashListaAnimesPorId.insert(rid, "Watching");
                qDebug() << vHashListaAnimesPorId[rid];
                vlistaWatching.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaWatching.size()-1);
            }
            else if(rlista.compare("Completed") == 0){
                lnovoAnime->vlista = "Completed";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed");
                vlistaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaCompleted.size()-1);
            }
            else if(rlista.compare("Dropped") == 0){
                lnovoAnime->vlista = "Dropped";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped");
                vlistaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaDropped.size()-1);
            }
            else if(rlista.compare("On Hold") == 0){
                lnovoAnime->vlista = "On Hold";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold");
                vlistaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Watch") == 0){
                lnovoAnime->vlista = "Plan to Watch";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Watch");
                vlistaPlanToWatch.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaPlanToWatch.size()-1);
            }
            return true;
        }
    }
    else if(rtipo.compare("novel", Qt::CaseInsensitive) == 0){
        if(llista.compare("Reading(Novel)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaNovelReading[lposicao];
            if(rlista.compare("Completed(Novel)") == 0){
                lnovoAnime->vlista = "Completed(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Novel)");
                vlistaNovelCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelCompleted.size()-1);
            }
            else if(rlista.compare("Dropped(Novel)") == 0){
                lnovoAnime->vlista = "Dropped(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Novel)");
                vlistaNovelDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelDropped.size()-1);
            }
            else if(rlista.compare("On Hold(Novel)") == 0){
                lnovoAnime->vlista = "On Hold(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Novel)");
                vlistaNovelOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Read(Novel)") == 0){
                lnovoAnime->vlista = "Plan to Read(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Novel)");
                vlistaNovelPlanToRead.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelPlanToRead.size()-1);
            }
            vlistaNovelReading.remove(lposicao);
            return true;
        }
        else if(llista.compare("Completed(Novel)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaNovelCompleted[lposicao];
            if(rlista.compare("Reading(Novel)") == 0){
                lnovoAnime->vlista = "Reading(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Novel)");
                vlistaNovelReading.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelReading.size()-1);
            }
            else if(rlista.compare("Dropped(Novel)") == 0){
                lnovoAnime->vlista = "Dropped(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Novel)");
                vlistaNovelDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelDropped.size()-1);
            }
            else if(rlista.compare("On Hold(Novel)") == 0){
                lnovoAnime->vlista = "On Hold(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Novel)");
                vlistaNovelOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Read(Novel)") == 0){
                lnovoAnime->vlista = "Plan to Read(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Novel)");
                vlistaNovelPlanToRead.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelPlanToRead.size()-1);
            }
            vlistaNovelCompleted.remove(lposicao);
            return true;
        }
        else if(llista.compare("On Hold(Novel)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaNovelOnHold[lposicao];
            if(rlista.compare("Reading(Novel)") == 0){
                lnovoAnime->vlista = "Reading(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Novel)");
                vlistaNovelReading.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelReading.size()-1);
            }
            else if(rlista.compare("Completed(Novel)") == 0){
                lnovoAnime->vlista = "Completed(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Novel)");
                vlistaNovelCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelCompleted.size()-1);
            }
            else if(rlista.compare("Dropped(Novel)") == 0){
                lnovoAnime->vlista = "Dropped(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Novel)");
                vlistaNovelDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelDropped.size()-1);
            }
            else if(rlista.compare("Plan to Read(Novel)") == 0){
                lnovoAnime->vlista = "Plan to Read(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Novel)");
                vlistaNovelPlanToRead.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelPlanToRead.size()-1);
            }
            vlistaNovelOnHold.remove(lposicao);
            return true;
        }
        else if(llista.compare("Dropped(Novel)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaNovelDropped[lposicao];
            if(rlista.compare("Reading(Novel)") == 0){
                lnovoAnime->vlista = "Reading(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Novel)");
                vlistaNovelReading.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelReading.size()-1);
            }
            else if(rlista.compare("Completed(Novel)") == 0){
                lnovoAnime->vlista = "Completed(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Novel)");
                vlistaNovelCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelCompleted.size()-1);
            }
            else if(rlista.compare("On Hold(Novel)") == 0){
                lnovoAnime->vlista = "On Hold(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Novel)");
                vlistaNovelOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Read(Novel)") == 0){
                lnovoAnime->vlista = "Plan to Read(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Novel)");
                vlistaNovelPlanToRead.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelPlanToRead.size()-1);
            }
            vlistaNovelDropped.remove(lposicao);
            return true;
        }
        else if(llista.compare("Plan to Read(Novel)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaNovelPlanToRead[lposicao];
            if(rlista.compare("Reading(Novel)") == 0){
                lnovoAnime->vlista = "Reading(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Novel)");
                vlistaNovelReading.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelReading.size()-1);
            }
            else if(rlista.compare("Completed(Novel)") == 0){
                lnovoAnime->vlista = "Completed(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Novel)");
                vlistaNovelCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelCompleted.size()-1);
            }
            else if(rlista.compare("On Hold(Novel)") == 0){
                lnovoAnime->vlista = "On Hold(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Novel)");
                vlistaNovelOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelOnHold.size()-1);
            }
            else if(rlista.compare("Dropped(Novel)") == 0){
                lnovoAnime->vlista = "Dropped(Novel)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Novel)");
                vlistaNovelDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaNovelDropped.size()-1);
            }
            vlistaNovelPlanToRead.remove(lposicao);
            return true;
        }
    }
    else if(rtipo.compare("manga", Qt::CaseInsensitive) == 0){
        if(llista.compare("Reading(Manga)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaMangaReading[lposicao];
            if(rlista.compare("Completed(Manga)") == 0){
                lnovoAnime->vlista = "Completed(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Manga)");
                vlistaMangaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaCompleted.size()-1);
            }
            else if(rlista.compare("Dropped(Manga)") == 0){
                lnovoAnime->vlista = "Dropped(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Manga)");
                vlistaMangaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaDropped.size()-1);
            }
            else if(rlista.compare("On Hold(Manga)") == 0){
                lnovoAnime->vlista = "On Hold(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Manga)");
                vlistaMangaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Read(Manga)") == 0){
                lnovoAnime->vlista = "Plan to Read(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Manga)");
                vlistaMangaPlanToRead.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaPlanToRead.size()-1);
            }
            vlistaMangaReading.remove(lposicao);
            return true;
        }
        else if(llista.compare("Completed(Manga)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaMangaCompleted[lposicao];
            if(rlista.compare("Reading(Manga)") == 0){
                lnovoAnime->vlista = "Reading(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Manga)");
                vlistaMangaReading.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaReading.size()-1);
            }
            else if(rlista.compare("Dropped(Manga)") == 0){
                lnovoAnime->vlista = "Dropped(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Manga)");
                vlistaMangaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaDropped.size()-1);
            }
            else if(rlista.compare("On Hold(Manga)") == 0){
                lnovoAnime->vlista = "On Hold(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Manga)");
                vlistaMangaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Read(Manga)") == 0){
                lnovoAnime->vlista = "Plan to Read(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Manga)");
                vlistaMangaPlanToRead.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaPlanToRead.size()-1);
            }
            vlistaMangaCompleted.remove(lposicao);
            return true;
        }
        else if(llista.compare("On Hold(Manga)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaMangaOnHold[lposicao];
            if(rlista.compare("Reading(Manga)") == 0){
                lnovoAnime->vlista = "Reading(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Manga)");
                vlistaMangaReading.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaReading.size()-1);
            }
            else if(rlista.compare("Completed(Manga)") == 0){
                lnovoAnime->vlista = "Completed(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Manga)");
                vlistaMangaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaCompleted.size()-1);
            }
            else if(rlista.compare("Dropped(Manga)") == 0){
                lnovoAnime->vlista = "Dropped(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Manga)");
                vlistaMangaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaDropped.size()-1);
            }
            else if(rlista.compare("Plan to Read(Manga)") == 0){
                lnovoAnime->vlista = "Plan to Read(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Manga)");
                vlistaMangaPlanToRead.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaPlanToRead.size()-1);
            }
            vlistaMangaOnHold.remove(lposicao);
            return true;
        }
        else if(llista.compare("Dropped(Manga)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaMangaDropped[lposicao];
            if(rlista.compare("Reading(Manga)") == 0){
                lnovoAnime->vlista = "Reading(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Manga)");
                vlistaMangaReading.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaReading.size()-1);
            }
            else if(rlista.compare("Completed(Manga)") == 0){
                lnovoAnime->vlista = "Completed(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Manga)");
                vlistaMangaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaCompleted.size()-1);
            }
            else if(rlista.compare("On Hold(Manga)") == 0){
                lnovoAnime->vlista = "On Hold(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Manga)");
                vlistaMangaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaOnHold.size()-1);
            }
            else if(rlista.compare("Plan to Read(Manga)") == 0){
                lnovoAnime->vlista = "Plan to Read(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Plan to Read(Manga)");
                vlistaMangaPlanToRead.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaPlanToRead.size()-1);
            }
            vlistaMangaDropped.remove(lposicao);
            return true;
        }
        else if(llista.compare("Plan to Read(Manga)", Qt::CaseInsensitive) == 0){
            QPointer<anime> lnovoAnime(new anime);
            lnovoAnime = vlistaMangaPlanToRead[lposicao];
            if(rlista.compare("Reading(Manga)") == 0){
                lnovoAnime->vlista = "Reading(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Reading(Manga)");
                vlistaMangaReading.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaReading.size()-1);
            }
            else if(rlista.compare("Completed(Manga)") == 0){
                lnovoAnime->vlista = "Completed(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Completed(Manga)");
                vlistaMangaCompleted.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaCompleted.size()-1);
            }
            else if(rlista.compare("On Hold(Manga)") == 0){
                lnovoAnime->vlista = "On Hold(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "On Hold(Manga)");
                vlistaMangaOnHold.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaOnHold.size()-1);
            }
            else if(rlista.compare("Dropped(Manga)") == 0){
                lnovoAnime->vlista = "Dropped(Manga)";
                vHashListaAnimesPorId.insert(lnovoAnime->vid, "Dropped(Manga)");
                vlistaMangaDropped.append(lnovoAnime);
                vHashPosicaoAnimesPorId.insert(rid,vlistaMangaDropped.size()-1);
            }
            vlistaMangaPlanToRead.remove(lposicao);
            return true;
        }
    }
    return false;
}
