#ifndef FORMATAPALAVRAS_H
#define FORMATAPALAVRAS_H

#include <QObject>
#include <QDebug>

#include "anitomy/anitomy.h"

class FormataPalavras : public QObject
{
    Q_OBJECT
public:
    explicit FormataPalavras(QObject *parent = nullptr);
    bool fcomparaNomes(const QString &rnome1, const QString &rnome2);
    QString fremoveTudo(QString);
    QString fremovePontuacao(QString);
    QString fremoveCaracteresEspeciais(QString);
    QString fmudaCaracteresEspeciais(QString);
    QString fremoveSeason(QString);
    QString fmudaNumeracaoArabePraRomana(QString);
    QString fremoveNumeracao(QString);
    QString fmudaNumeracaoRomanaPraArabe(QString);
    QString fmudaNumeracaoRomanaPraSX(QString);
    QString fmudaNumeracaoRomanaPraSeason(QString);
    QString fremoveNumeracaoRomana(QString);
    QString fremoveCaracteresExtras(QString);
    QString fmudaOVAPraSpecials(QString);
    QString fmudaSpecialsPraOVA(QString);
    QString fmudaOVAPraSpecial(QString);
    QString fmudaSpecialPraOVA(QString);
    QString fremoveOVAEspecial(QString);
    QString fremoveTV(QString);

    //Tentar fazer meu próprio anitomy?

signals:

};

#endif // FORMATAPALAVRAS_H
