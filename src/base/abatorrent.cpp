#include "abatorrent.h"

abaTorrent::abaTorrent(QObject *parent) : QObject(parent)
{
    downloaderTorrent = new Downloader;
    downloaderTorrent->fsetWorker();
}

void abaTorrent::fRefreshControl()
{
    //Primeiramente, devemos checar se a lista está vazia. Caso não esteja, devemos esvaziá-la
    if(!vlistaTorrents.isEmpty())
        vlistaTorrents.clear();
//    fgetTorrentList();
}

void abaTorrent::fgetTorrentList()
{
//    QPointer<Downloader> downloadTorrentXML(new Downloader);
//    downloadTorrentXML->fsetWorker();
    downloaderTorrent->workXML(1);
    connect(downloaderTorrent,&Downloader::sfinishedXML,this, &abaTorrent::freadTorrentList);
}

void abaTorrent::fgetSpecificTorrentList(QString search)
{
//    QPointer<Downloader> downloadTorrentXML(new Downloader);
//    downloadTorrentXML->fsetWorker();
    downloaderTorrent->workSpecificXML(1, search);
    connect(downloaderTorrent,&Downloader::sfinishedXML,this, &abaTorrent::freadTorrentList);
}

void abaTorrent::freadTorrentList()
{
    if(!vlistaTorrents.isEmpty())
        vlistaTorrents.clear();
    QFile file(QDir::currentPath() + "/Configurações/Temp/torrents.xml");
    if(!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Não foi possível encontrar os torrents!";
        qCritical() << file.errorString();
        return;
    }

    QByteArray ldata = file.readAll();
    file.close();

    QString lnomeTorrent;
    QString lnomeAnime;
    QString lfansub;
    QString lresolucao;
    QString lepisodioAnime;
    QString llinkTorrent;
    QString ldescricaoTorrent;
    QString llinkInfoTorrent;
    int lprioridade = 0;

    QString lid;
    QString lepisodiosAssistidos;
    QString lista;
    int lposicaoAnimeNaLista;
    QStringList filtros;

    QXmlStreamReader stream(ldata);
    while (!stream.atEnd()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        switch (token) {
        case QXmlStreamReader::Comment:
            break;
        case QXmlStreamReader::DTD:
            break;
        case QXmlStreamReader::Characters:
            break;
        case QXmlStreamReader::ProcessingInstruction:
            break;
        case QXmlStreamReader::EntityReference:
            break;
        case QXmlStreamReader::NoToken:
            break;
        case QXmlStreamReader::Invalid:
            break;
        case QXmlStreamReader::StartDocument:
            break;
        case QXmlStreamReader::EndDocument:
            break;
        case QXmlStreamReader::StartElement:
            if(stream.name() == "title") {
                QString name = stream.readElementText();
                if(name.contains("Tokyo Toshokan"))
                    break;
                anitomy.Parse(name.toStdWString());
                const auto& elements = anitomy.elements();
                //Nome do arquivo
                lnomeTorrent = name;
                //Episodio do anime do arquivo
                lepisodioAnime = QString::fromStdWString(elements.get(anitomy::kElementEpisodeNumber));
                //Nome do anime no arquivo
                lnomeAnime = QString::fromStdWString(elements.get(anitomy::kElementAnimeTitle));
                //Temporada do anime no arquivo
                QString vtemporada = QString::fromStdWString(elements.get(anitomy::kElementAnimeSeason));
                if(!vtemporada.isEmpty())
                    lnomeAnime.append(QString(" " + vtemporada));
                //Fansub do anime
                lfansub = QString::fromStdWString(elements.get(anitomy::kElementReleaseGroup));
                //Resolução do anime
                lresolucao = QString::fromStdWString(elements.get(anitomy::kElementVideoResolution));
                if(lresolucao.compare("720p") == 0 || lresolucao.compare("480p") == 0 || lresolucao.compare("360p") == 0)
                    lresolucao.insert(0,"0");


                //Tento achar o id do anime de forma rápida, pra pegar informações com mais facilidade
                if(vHashDeNomeEId.contains(lnomeAnime))
                    lid = vHashDeNomeEId[lnomeAnime];
                else
//                    lid = cleitor->fprocuraAnimeNasListas(lnomeAnime);
                    lid = cleitorListaAnimes->instance()->fprocuraIdNasListasRapido(lnomeAnime);
                if(!vHashDeNomeEId.contains(lnomeAnime))
                    vHashDeNomeEId.insert(lnomeAnime,lid);

                lepisodiosAssistidos = cleitorListaAnimes->instance()->fbuscaAnimePorIDERetornaEpisodio(lid);
                lista = cleitorListaAnimes->instance()->fbuscaAnimePorIDERetornaLista(lid);
                lposicaoAnimeNaLista = cleitorListaAnimes->instance()->fbuscaAnimePorIDERetornaPosicao(lid);
            }
            if(stream.name() == "link") {
                QString link = stream.readElementText();
                if(link.contains("https://www.tokyotosho.info"))
                    break;
                link.remove("<![CDATA[");
                link.remove("]]>");
                llinkTorrent = link;
            }
            if(stream.name() == "description") {
                QString description = stream.readElementText();
                if(description.contains("Torrent Listing"))
                    break;
                QStringList comment = description.split("/>");
                if(comment.size() > 2) {
                    llinkInfoTorrent = comment.at(2);
                    llinkInfoTorrent.remove(0,1);
                    llinkInfoTorrent.remove("<a href=\"");
                    llinkInfoTorrent.remove("\">Tokyo Tosho</a><br ");
                    description = comment.at(comment.size()-1);
                    description.remove("]]>");
                    description.remove(0,10);
                    ldescricaoTorrent = description;
                }
                else {
                    comment = description.split(" | ");
                    llinkInfoTorrent = comment.at(0);
                    llinkInfoTorrent = llinkInfoTorrent.split("\"").at(1);
                    comment.removeAt(0);
                    description = comment.join(" | ");
                    description.remove("]]>");
                    description.remove("</a>");
                    ldescricaoTorrent = description;
                }

                QPointer<torrentinfo> ltorrentAux(new torrentinfo);
                ltorrentAux->vnomeTorrent = lnomeTorrent;
                ltorrentAux->vdescricaoTorrent = ldescricaoTorrent;
                ltorrentAux->vlinkTorrent = llinkTorrent;
                ltorrentAux->vnomeAnime = lnomeAnime;
                ltorrentAux->vfansub = lfansub;
                ltorrentAux->vresolucao = lresolucao;
                ltorrentAux->vepisodioAnime = lepisodioAnime;
                ltorrentAux->vtorrentInfoLink = llinkInfoTorrent;
                ltorrentAux->vlista = lista;

                //A prioridade é dividida em 4 níveis
                //0 -> não baixar
                //1 -> Não é a qualidade ou sub certo, mas pode ser baixado na falta de melhores opções
                //2 -> Qualidade ou sub certo, pode ser baixado na falta de melhores opções
                //3 -> Arquivo perfeito, qualidade e sub certo, será baixado.
                lprioridade = fcheckPriority(ltorrentAux);
                ltorrentAux->vprioridade = lprioridade;

                //Checa se o anime existe na lista de downloads, separando cada episódio como uma entrada diferente
                //Caso já contenha uma entrada, checa se a prioridade da entrada na lista é menor do que a atual
                //A ideia é baixar sempre os que tem maior prioridade
                if(vdownloadList.contains(lnomeAnime+lepisodioAnime)) {
                    //As entradas são formadas por uma combinação de prioridade e posição do torrent na lista.
                    if(vdownloadList.value(lnomeAnime+lepisodioAnime).at(0).toInt() < lprioridade)
                        //A entrada atual, caso tenha menos prioridade do que o torrent checado, será sobrescrita
                        vdownloadList.insert(lnomeAnime+lepisodioAnime,
                                             (QString::number(lprioridade)+":"+QString::number(vlistaTorrents.size())).split(":"));
                }
                //Caso o anime não exista na lista de downloads, será inserido na lista independente de sua prioridade
                //Desde que seja maior que 0
                else if(lprioridade > 0)
                    vdownloadList.insert(lnomeAnime+lepisodioAnime,
                                         (QString::number(lprioridade)+":"+QString::number(vlistaTorrents.size())).split(":"));
                //Logo em seguida, o torrent é adicionado à lista geral
                if(!ltorrentAux->vnomeTorrent.contains("Torrent File RSS")) {
                    ltorrentAux->vposicao = vlistaTorrents.size();
                    vlistaTorrents.append(ltorrentAux);
                }

            }
            break;
        case QXmlStreamReader::EndElement:
            break;
        }
    }
    //Após ter lido todos os torrents, checamos quais devem ser baixados
    foreach(QString key, vdownloadList.keys()) {
        if(vlistaTorrents.size() > vdownloadList[key].at(1).toInt()) {
            vlistaTorrents[vdownloadList[key].at(1).toInt()]->vbaixar = true;
            //Caso ele seja marcado pra download, sua prioridade aumenta
            //Isso acontece com o propósito de facilitar a ordenação ao montar a tabela
            vlistaTorrents[vdownloadList[key].at(1).toInt()]->vprioridade++;
        }
    }
    emit sfimXML();
}

void abaTorrent::fchangeTorrentState(int posicaTabela, bool checkState)
{
    if(vinfoTorrent.isEmpty() || vinfoTorrent.at(posicaTabela).toStringList().isEmpty())
        return;
    QList<QVariant> listaOrdenada = fgetTorrentInfo(vordemLista);
    QStringList tempList = vinfoTorrent.at(posicaTabela).toStringList();
    //-3 por que a posição na lista está no terceiro elemento de frente pra trás na QStringList
    int posicaoListaTorrents = tempList.at(tempList.size()-3).toInt();
    if(vlistaTorrents.size() > posicaoListaTorrents)
        vlistaTorrents[posicaoListaTorrents]->vbaixar = checkState;
}

void abaTorrent::fdownloadAnimes()
{
//    QPointer<Downloader> downloadTorrent(new Downloader);
    int idDownloader = 0;
    if(vlistaAnimesBaixados.isEmpty()) {
        for(int i = 0; i < vlistaTorrents.size(); i++) {
            if(vlistaTorrents[i]->vbaixar) {
                vlistaAnimesBaixados.append(i);
//                downloadTorrent->fsetWorker();
                qDebug() << "Baixando " + vlistaTorrents[i]->vnomeTorrent;
                downloaderTorrent->workAnimeTorrent(idDownloader++, vlistaTorrents[i]->vlinkTorrent, vlistaTorrents[i]->vnomeTorrent);
                vlistaTorrents[i]->vbaixar = false;
                connect(downloaderTorrent,&Downloader::sfinishedAnimeTorrent,this, &abaTorrent::fdownloadAnimes);
                return;
            }
        }
    }
    else {
        for(int i = vlistaAnimesBaixados.last(); i < vlistaTorrents.size(); i++) {
            if(vlistaTorrents[i]->vbaixar) {
                vlistaAnimesBaixados.append(i);
//                downloaderTorrent->fsetWorker();
                qDebug() << "Baixando " + vlistaTorrents[i]->vnomeTorrent;
                downloaderTorrent->workAnimeTorrent(idDownloader++, vlistaTorrents[i]->vlinkTorrent, vlistaTorrents[i]->vnomeTorrent);
                vlistaTorrents[i]->vbaixar = false;
                connect(downloaderTorrent,&Downloader::sfinishedAnimeTorrent,this, &abaTorrent::fdownloadAnimes);
                //Só vai retornar se achar um novo anime para baixar
                return;
            }
        }
    }
    //Caso ele rode o vetor todo e não tenha mais nenhum download, ele sai e começa a carregar os torrents no programa
    QString processo = cabaConfig->instance()->fgetPreferredTorrentPath();
    QProcess lprocesso;
    lprocesso.setProgram(cabaConfig->instance()->fgetPreferredTorrentPath());
    if(!lprocesso.startDetached())
        qDebug() << "Error: Could not open torrent's software";
    else
        fdownloadTorrents();
}

void abaTorrent::fdownloadTorrents()
{
    if(cabaConfig->instance()->fgetSaveFolder().isEmpty()) {
//        ui->labelMensagem->setText("Error: Trying to download without setting a download folder");
        qWarning() << "Error: Trying to download without setting a download folder";
//        emit error("No download folder");
        return;
    }
    if(vlistaAnimesBaixados.isEmpty())
        return;
    int tamanhoLista = vlistaAnimesBaixados.size();
    for(int i = 0; i < tamanhoLista; i++) {
        //É necessário ter um delay entre os inputs de comando
        this->thread()->wait(700);
        QPointer<QProcess> lprocesso(new QProcess);
        QStringList argumentos;
        //    if(cconfig->fretornaTorrentEscolhido() == "uTorrent"){
        //        lprocesso->setProgram(QDir::homePath() + "/AppData/Roaming/uTorrent/uTorrent.exe");
        //        argumentos.append("/DIRECTORY");
        //        argumentos.append(cconfig->fretornaPastaSalvarAnimes() + "/" + torrent[vlistaDownload[0]]->vnomeAnime);
        //        argumentos.append(QDir::currentPath() + "/Configurações/Temp/Torrents/" + torrent[vlistaDownload[0]]->vnomeTorrent +
        //                ".torrent");
        //    }
        if(cabaConfig->instance()->fgetPreferredTorrentPath().contains("qbittorrent")) {
            lprocesso->setProgram(cabaConfig->instance()->fgetPreferredTorrentPath());
            argumentos.append("--add-paused=false");
            argumentos.append("--skip-dialog=true");
            argumentos.append("--save-path=" + cabaConfig->instance()->fgetSaveFolder() + "/" +
                              vlistaTorrents[vlistaAnimesBaixados[0]]->vnomeAnime);
            argumentos.append(QDir::currentPath() + "/Configurações/Temp/Torrents/" + vlistaTorrents[vlistaAnimesBaixados[0]]->vnomeTorrent
                              + ".torrent");
        }
        lprocesso->setArguments(argumentos);
        if(lprocesso->startDetached()) {
            QString id = cleitorListaAnimes->instance()->fprocuraNomeRetornaID(vlistaTorrents[vlistaAnimesBaixados[0]]->vnomeAnime);
            cconfPastasAnimes->instance()->fselecionaPastaEspecificaAnime(id, cabaConfig->instance()->fgetSaveFolder() +
                    "/" + vlistaTorrents[vlistaAnimesBaixados[0]]->vnomeAnime);
            qDebug() << "Download started successfully!";
        }
        vlistaTorrents[vlistaAnimesBaixados[0]]->vbaixar = false;
        vlistaAnimesBaixados.remove(0);
    }
}

QVariant abaTorrent::fgetSingleTorrentInfo(int posicaTabela)
{
    if(vinfoTorrent.isEmpty() || vinfoTorrent.size() < posicaTabela)
        return 0;
    return vinfoTorrent.at(posicaTabela).toStringList();
}

int abaTorrent::fcheckPriority(torrentinfo* rtorrent)
{
    int prioridade = 0;

    //Checa se o anime está em uma lista que deve ser baixada. Se não estiver, a prioridade é 0
    QStringList listas = cabaConfig->instance()->fgetDownloadLists();
    if((rtorrent->vlista.compare("Watching", Qt::CaseInsensitive) == 0 && listas.contains("WATCHING")) ||
            (rtorrent->vlista.compare("Plan to Watch", Qt::CaseInsensitive) == 0 && listas.contains("PLANNING")) ||
            (rtorrent->vlista.compare("On Hold", Qt::CaseInsensitive) == 0 && listas.contains("PAUSED")) ||
            (rtorrent->vlista.compare("Dropped", Qt::CaseInsensitive) == 0 && listas.contains("DROPPED")))
        prioridade++;
    else
        return 0;

    //Checa se o episódio já foi assistido, e caso já tenha a prioridade é 0
    if(cleitorListaAnimes->instance()->fbuscaAnimePorIDERetornaEpisodio(vHashDeNomeEId[rtorrent->vnomeAnime]) >=
            rtorrent->vepisodioAnime)
        return 0;

    //Checa se o episódio já foi baixado, e caso já tiver sido, a prioridade é 0
    //Se ele não tiver sido baixado, a função irá retornar uma string vazia
    QPointer<arquivos> carquivos(new arquivos);
    if(!carquivos->fprocuraEpisodioEspecifico(cleitorListaAnimes->instance()->fretornaAnimePorID
            (vHashDeNomeEId[rtorrent->vnomeAnime]), rtorrent->vepisodioAnime.toInt()).isEmpty()) {
        return 0;
    }

    //Checa se o episódio tem as legendas do sub escolhido e, se tiver, aumenta a prioridade
    if(rtorrent->vfansub.compare(cabaConfig->instance()->fgetPreferredSub(), Qt::CaseInsensitive) == 0)
        prioridade++;
    //Checa se o episódio tem a qualidade escolhida e, se tiver, aumenta a prioridade
    if(rtorrent->vresolucao.compare(cabaConfig->instance()->fgetPreferredQuality(), Qt::CaseInsensitive) == 0)
        prioridade++;

    return prioridade;
}


QList<QVariant> abaTorrent::fgetTorrentInfo(QString ordem)
{
    vordemLista = ordem;
    //Aqui é onde eu devo dar sort na lista, pra deixar ela do jeito que preciso.
    vinfoTorrent.clear();
    for(int i = 0; i < vlistaTorrents.size(); i++)
        vinfoTorrent.append(vlistaTorrents[i]->fconverteLista());
    if(ordem.startsWith("c")) {
        if(ordem.contains("nomeAnime")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(1) < b.toStringList().at(1) ;});
        }
        else if(ordem.contains("nomeSub")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(2) < b.toStringList().at(2) ;});
        }
        else if(ordem.contains("resolucao")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(3) < b.toStringList().at(3) ;});
        }
        else if(ordem.contains("episodio")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(4) < b.toStringList().at(4) ;});
        }
        else if(ordem.contains("nomeArquivo")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(5) < b.toStringList().at(5) ;});
        }
        else if(ordem.contains("descricao")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(6) < b.toStringList().at(6) ;});
        }
        else if(ordem.contains("check")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(0) < b.toStringList().at(0) ;});
        }
    }
    else {
        if(ordem.contains("prioridade")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().last() > b.toStringList().last() ;});
        }
        else if(ordem.contains("nomeAnime")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(1) > b.toStringList().at(1) ;});
        }
        else if(ordem.contains("nomeSub")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(2) > b.toStringList().at(2) ;});
        }
        else if(ordem.contains("resolucao")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(3) > b.toStringList().at(3) ;});
        }
        else if(ordem.contains("episodio")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(4) > b.toStringList().at(4) ;});
        }
        else if(ordem.contains("nomeArquivo")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(5) > b.toStringList().at(5) ;});
        }
        else if(ordem.contains("descricao")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(6) > b.toStringList().at(6) ;});
        }
        else if(ordem.contains("check")) {
            std::sort(vinfoTorrent.begin(),vinfoTorrent.end(),[](QVariant &a, QVariant &b)->bool{
                return a.toStringList().at(0) < b.toStringList().at(0) ;});
        }
    }
    return vinfoTorrent;
}
//return QString("true§"+vnomeAnime+"§"+vfansub+"§"+vresolucao+"§"+vepisodioAnime+"§"+vnomeTorrent+"§"
//           +vdescricaoTorrent+"§"+QString::number(vposicao)+"§"+QString::number(vprioridade)).split("§");
