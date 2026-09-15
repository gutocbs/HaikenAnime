#include "confusuario.h"

confUsuario::confUsuario(QObject *parent) : QObject(parent)
{
    vterminouChecagem = false;
    vlista = 0;
//    vdiretorioAnimes.append("E:/Animes/");
//    vdiretorioAnimes.append("E:/Downloads/");
}


QString confUsuario::fretornaDiretorioEspecifico(int id){
    if(vterminouChecagem)
        return vdiretorioEspecificoAnime[QString::number(id)];
    return "";
}

QVector<QString> confUsuario::fretornaDiretoriosAnimes(){
    return vdiretorioAnimes;
}

void confUsuario::fbuscaDiretoriosAnimes(){
    anitomy::Anitomy lanitomy;
    if(this->thread()->isInterruptionRequested()){
        this->thread()->exit(0);
        return;
    }
//    qDebug() << "Searching for anime folders";
    //Busca cada diretorio existente nas configurações
    for(int i = 0; i < vdiretorioAnimes.size(); i++){
        //Procura apenas por diretorios e subdiretorios
        QDirIterator lit(vdiretorioAnimes[i], QDir::Dirs| QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while(lit.hasNext()){
            QFile lfile(lit.next());
            lanitomy.Parse(lit.fileName().toStdWString());
            const auto& lelements = lanitomy.elements();
            //Usamos isso para pegar o número do episódio e o nome do anime a partir do nome do arquivo
            QString lfileName = QString::fromStdWString(lelements.get(anitomy::kElementAnimeTitle));
            QString vtemporada = QString::fromStdWString(lelements.get(anitomy::kElementAnimeSeason));
            if(!vtemporada.isEmpty())
                lfileName.append(QString(" " + vtemporada));
            //Pega o nome de cada diretorio e divide, pegando apenas o nome do anime
            //Compara o nome do anime com os animes da lista
            for(int w = 0; w < vlistaAnimes.size(); w++){
                if(this->thread()->isInterruptionRequested()){
                    this->thread()->exit(0);
                    return;
                }
                //Anitomy, aqui, simplifica os nomes dos anime para garantir que a função vai comparar os nomes certos
                //Compara os nomes, sempre ignorando letras maiusculas e minusculas para garantir que o anime será encontrado.
                if(vdiretorioEspecificoAnime.contains(vlistaAnimes[w]->vid)){
                    if(!vdiretorioEspecificoAnime[vlistaAnimes[w]->vid].isEmpty()){
                        if(QFile::exists(vdiretorioEspecificoAnime[vlistaAnimes[w]->vid])){
                            vlistaAnimes.remove(w);
                            break;
                        }
                        else
                            vdiretorioEspecificoAnime.remove(vlistaAnimes[w]->vid);
                    }
                }
                if(this->thread()->isInterruptionRequested()){
                    this->thread()->exit(0);
                    return;
                }
                if(formatador.fcomparaNomes(lfileName,vlistaAnimes[w]->vnome)){
                    vdiretorioEspecificoAnime.insert(vlistaAnimes[w]->vid, lfile.fileName());
                    vlistaAnimes.remove(w);
                    break;
                }
                else if(formatador.fcomparaNomes(lfileName,vlistaAnimes[w]->vnomeIngles)){
                    vdiretorioEspecificoAnime.insert(vlistaAnimes[w]->vid, lfile.fileName());
                    vlistaAnimes.remove(w);
                    break;
                }
                else{
                    //Compara os nomes alternativos dos animes, pro caso de serem usados nos arquivos
                    //Ex: Okaa-san Online em vez de Tsuujou Kougeki ga Zentai Kougeki de Ni-kai Kougeki no Okaasan wa Suki Desu ka?
                    if(!vlistaAnimes[w]->vnomeAlternativo.isEmpty()){
                        for(int z = 0; z < vlistaAnimes[w]->vnomeAlternativo.size(); z++){
                            if(formatador.fcomparaNomes(lfileName,vlistaAnimes[w]->vnomeAlternativo.at(z))){
                                vdiretorioEspecificoAnime.insert(vlistaAnimes[w]->vid, lfile.fileName());
                                vlistaAnimes.remove(w);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    fsetupListasPraBusca();
}

void confUsuario::fbuscaPastasThread(QThread &dThread)
{
    vlista = 0;
    connect(&dThread, &QThread::started, this, &confUsuario::fsetupListasPraBusca, Qt::QueuedConnection);
    qDebug()<< "Buscando diretorios";
}

void confUsuario::fsalvaPastasArquivos()
{
    QFile t("Configurações/Temp/animeFoldersTemp.txt");
    if(t.open(QIODevice::WriteOnly)){
        QTextStream lstreamTexto(&t);
        lstreamTexto.setCodec("UTF-8");
        foreach(QString key, vdiretorioEspecificoAnime.keys()){
            lstreamTexto << key << ";" << vdiretorioEspecificoAnime[key] << endl;
        }
        t.close();
    }
    if(QFile::exists("Configurações/Temp/animeFolders.txt")){
        if(QFile::remove("Configurações/Temp/animeFolders.txt"))
            t.rename("Configurações/Temp/animeFolders.txt");
    }
    else
        t.rename("Configurações/Temp/animeFolders.txt");
}

void confUsuario::flePastasArquivos()
{
    QFile t("Configurações/Temp/animeFolders.txt");
    QTextStream lstreamTexto(&t);
    lstreamTexto.setCodec("UTF-8");
    if(t.open(QIODevice::ReadOnly)){
        while(!lstreamTexto.atEnd()){
            QStringList llinha = lstreamTexto.readLine().split(";");
            vdiretorioEspecificoAnime.insert(llinha.at(0),llinha.at(1));
        }
        vterminouChecagem = true;
        t.close();
    }
}

void confUsuario::fselecionaPastaEspecificaAnime(const QString &ridAnime, const QString &rdirAnime)
{
    if(vdiretorioEspecificoAnime.contains(ridAnime))
        vdiretorioEspecificoAnime.remove(ridAnime);
    vdiretorioEspecificoAnime.insert(ridAnime,rdirAnime);
    fsalvaPastasArquivos();
}

//Aparentemente nunca entra aqui
void confUsuario::fsetupListasPraBusca()
{
    if(vlista == 0){
        if(this->thread()->isInterruptionRequested()){
            this->thread()->exit(0);
            return;
        }
        vlistaAnimes = cleitorlistaanimes->retornaListaWatching();
        vlista++;
        qDebug() << "Searching for animes from list Watching";
        fbuscaDiretoriosAnimes();
    }
    else if(vlista == 1){
        if(this->thread()->isInterruptionRequested()){
            this->thread()->exit(0);
            return;
        }
        vlistaAnimes = cleitorlistaanimes->retornaListaCompleted();
        vlista++;
        qDebug() << "Searching for animes from list Completed";
        fbuscaDiretoriosAnimes();
    }
    else if(vlista == 2){
        if(this->thread()->isInterruptionRequested()){
            this->thread()->exit(0);
            return;
        }
        vlistaAnimes = cleitorlistaanimes->retornaListaDropped();
        vlista++;
        qDebug() << "Searching for animes from list Dropped";
        fbuscaDiretoriosAnimes();
    }
    else if(vlista == 3){
        if(this->thread()->isInterruptionRequested()){
            this->thread()->exit(0);
            return;
        }
        vlistaAnimes = cleitorlistaanimes->retornaListaOnHold();
        vlista++;
        qDebug() << "Searching for animes from list On Hold";
        fbuscaDiretoriosAnimes();
    }
    else if(vlista == 4){
        if(this->thread()->isInterruptionRequested()){
            this->thread()->exit(0);
            return;
        }
        vlistaAnimes = cleitorlistaanimes->retornaListaPlanToWatch();
        vlista++;
        qDebug() << "Searching for animes from list Plan to Watch";
        fbuscaDiretoriosAnimes();
    }
    else if(vlista == 5){
        if(this->thread()->isInterruptionRequested()){
            this->thread()->exit(0);
            return;
        }
        fsalvaPastasArquivos();
        cleitorlistaanimes->fcarregaListaAnoEmThread();
        emit schecouPastas();
        vterminouChecagem = true;
        qDebug() << "All animes in the computer were found";
        this->thread()->exit(0);
    }
}

void confUsuario::frecebeListaAnime(leitorlistaanimes *rlistaAnime)
{
    cleitorlistaanimes = rlistaAnime;
}

void confUsuario::frecebeConfigs(const QStringList &ldiretorios)
{
    vdiretorioAnimes = ldiretorios.toVector();
}
