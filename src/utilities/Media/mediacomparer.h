#ifndef MEDIACOMPARER_H
#define MEDIACOMPARER_H

#include <QObject>
#include <QDebug>
#include <QRegExp>

#include "lib/anitomy/anitomy.h"
#include "src/DTO/media.h"
#include "src/utilities/Media/mediautil.h"


class MediaComparer : public QObject
{
    Q_OBJECT
public:
    //TODO - funções
    explicit MediaComparer(QObject *parent = nullptr);
    static bool compareMedia(Media *media, QString fileName);
    static bool compareName(QString rnome1, QString rnome2);
    static bool compareName(QStringList nameList, QString rnome2);
    static bool compareEpisodeNumber(Media *media, int episode);
    static bool isSeasonal(Enums::mediaList);



signals:
private:
    static QString fremoveTudo(QString);
    static QString fremovePontuacao(QString);
    static QString fremoveCaracteresEspeciais(QString);
    static QString fmudaCaracteresEspeciais(QString);
    static QString fremoveSeason(QString);
    static QString fmudaNumeracaoArabePraRomana(QString);
    static QString fremoveNumeracao(QString);
    static QString fmudaNumeracaoRomanaPraArabe(QString);
    static QString fmudaNumeracaoRomanaPraSX(QString);
    static QString fmudaNumeracaoRomanaPraSeason(QString);
    static QString fremoveNumeracaoRomana(QString);
    static QString fremoveCaracteresExtras(QString);
    static QString fmudaOVAPraSpecials(QString);
    static QString fmudaSpecialsPraOVA(QString);
    static QString fmudaOVAPraSpecial(QString);
    static QString fmudaSpecialPraOVA(QString);
    static QString fremoveOVAEspecial(QString);
    static QString fremoveTV(QString);
};

#endif // MEDIACOMPARER_H
