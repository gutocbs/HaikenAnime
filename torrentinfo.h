#ifndef TORRENTINFO_H
#define TORRENTINFO_H

#include <QObject>
#include <QCheckBox> //Pra poder por as caixas e tickar elas

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
//    QCheckBox vbox;
    bool vbaixar = false;
signals:

public slots:
};

#endif // TORRENTINFO_H
