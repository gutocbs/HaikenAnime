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
                       +vdescricaoTorrent+"§"+QString::number(vposicao)+"§"+QString::number(vprioridade)).split("§");

        return QString("false§"+vnomeAnime+"§"+vfansub+"§"+vresolucao+"§"+vepisodioAnime+"§"+vnomeTorrent+"§"
                   +vdescricaoTorrent+"§"+QString::number(vposicao)+"§"+QString::number(vprioridade)).split("§");
    }
signals:

public slots:
};

#endif // TORRENTINFO_H
