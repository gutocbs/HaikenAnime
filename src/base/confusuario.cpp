#include "confusuario.h"

confUsuario *confUsuario::createInstance()
{
    return new confUsuario();
}

confUsuario::confUsuario(QObject *parent) : QObject(parent)
{
    vterminouChecagem = false;
    vlista = 0;
    cdatabase = new Database();
//    vdiretorioAnimes.append("E:/Animes/");
    //    vdiretorioAnimes.append("E:/Downloads/");
}

confUsuario *confUsuario::instance()
{
    return  Singleton<confUsuario>::instance(confUsuario::createInstance);
}


QString confUsuario::fretornaDiretorioEspecifico(int id){
    if(vterminouChecagem)
        return vdiretorioEspecificoAnime[QString::number(id)];
    return "";
}

QVector<QString> confUsuario::fretornaDiretoriosAnimes(){
    return vdiretorioAnimes.toVector();
}

void confUsuario::fbuscaDiretoriosAnimes(){
    anitomy::Anitomy lanitomy;
    if(this->thread()->isInterruptionRequested() || vdiretorioAnimes.isEmpty()){
        this->thread()->exit(0);
        return;
    }

    QString idAnime;
    QString lfileName;
    QString vtemporada;

    vdiretorioAnimes.removeDuplicates();

    //Busca cada diretorio existente nas configurações
    for(int i = 0; i < vdiretorioAnimes.size(); i++){
        //Procura apenas por diretorios e subdiretorios
        QDirIterator lit(vdiretorioAnimes[i], QDir::Dirs| QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while(lit.hasNext()){
            QFile lfile(lit.next());
            lanitomy.Parse(lit.fileName().toStdWString());
            const auto& lelements = lanitomy.elements();

            //Usamos isso para pegar o número do episódio e o nome do anime a partir do nome do arquivo
            lfileName = QString::fromStdWString(lelements.get(anitomy::kElementAnimeTitle));
            vtemporada = QString::fromStdWString(lelements.get(anitomy::kElementAnimeSeason));
            if(!vtemporada.isEmpty())
                lfileName.append(QString(" " + vtemporada));

            //Checa se o diretório tem o nome de um anime das listas
            idAnime = cdatabase->instance()->fbuscaNomeRetornaID(lfileName);
            if(!idAnime.isEmpty())
                vdiretorioEspecificoAnime.insert(idAnime, lfile.fileName());
        }
    }
    fsetupListasPraBusca();
}

void confUsuario::fgetThread(QThread &dThread)
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
            lstreamTexto << key << ";" << vdiretorioEspecificoAnime[key] << Qt::endl;
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
        vlista++;
        qDebug() << "Searching for animes in your computer";
        fbuscaDiretoriosAnimes();
    }
    else if(vlista == 1){
        if(this->thread()->isInterruptionRequested()){
            this->thread()->exit(0);
            return;
        }
        vlista = 0;
        fsalvaPastasArquivos();
        cdatabase->instance()->fcarregaListaAnoEmThread();
        emit schecouPastas();
        vterminouChecagem = true;
        qDebug() << "All animes in the computer were found";
        this->thread()->exit(0);
    }
}

void confUsuario::frecebeConfigs(const QStringList &ldiretorios)
{
    QStringList filtroLista = ldiretorios;
    filtroLista.removeDuplicates();
    vdiretorioAnimes.append(filtroLista);
}
