#ifndef TORRENTINFO_H
#define TORRENTINFO_H

#include <QObject>

class torrentinfo : public QObject
{
    Q_OBJECT
public:
    explicit torrentinfo(QObject *parent = nullptr){
        Q_UNUSED(parent)
        vbaixar = false;
    }
    QString vnomeTorrent;
    QString vnomeAnime;
    QString vlinkTorrent;
    QString vdescricaoTorrent;
    QString vepisodioAnime;
    QString vfansub;
    QString vresolucao;
    QString vsize;
    QString vtorrentInfoLink;
    QString vlista;
    bool vbaixar;
    int vprioridade;
    int vposicao;

    QStringList fconverteLista(){
        if(vbaixar)
            return QString("true§"+vnomeAnime+"§"+vfansub+"§"+vresolucao+"§"+vepisodioAnime+"§"+vnomeTorrent+"§"
                       +vdescricaoTorrent+"§"+QString::number(vposicao)+"§"+vlinkTorrent+"§"+QString::number(vprioridade)).split("§");

        return QString("false§"+vnomeAnime+"§"+vfansub+"§"+vresolucao+"§"+vepisodioAnime+"§"+vnomeTorrent+"§"
                   +vdescricaoTorrent+"§"+QString::number(vposicao)+"§"+vtorrentInfoLink+"§"+QString::number(vprioridade)).split("§");
    }

    ///0 - Check
    ///1 - Nome
    ///2 - Fansub
    ///3 - Resolução
    ///4 - Episódio
    ///5 - Nome do Torrent
    ///6 - Descrição do torrent
    ///7 - Posição na lista original
    ///8 - Link da página do torrent
    ///9 - Prioridade do torrent
public slots:
};

#endif // TORRENTINFO_H
