#ifndef PLAYERRECOGNITION_H
#define PLAYERRECOGNITION_H

#include <QObject>
#include "src/base/abaconfig.h"
#include "lib/anitomy/anitomy.h"
//REFEITO 50%
class playerRecognition : public QObject
{
    Q_OBJECT
public:
    explicit playerRecognition(QObject *parent = nullptr);
    void fchecaStream(const QString &player, const QString &janela);
    QString fretornaAnime();
    QString fretornaEpisodio();


signals:

private:
    void fchecaCrunchyroll();
    void fchecaAnimeLab();
    void fchecaFunimation();
    void fchecaKissAnime();
    void fchecaWinamp();
    void fchecaLocalPlayer();
    QString vanime;
    QString vepisodio;
    QString vnomeDaJanela;
    QStringList vStreamLanguages;
};

#endif // PLAYERRECOGNITION_H
