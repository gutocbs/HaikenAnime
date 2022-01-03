#ifndef CONFBASE_H
#define CONFBASE_H

#include <QObject>
#include <QDir> //Cria diretorios
#include <QFile> //Cria arquivos
#include <QDebug>

#include "src/utilities/singleton.h"
//REFEITO
class confBase : public QObject
{
    Q_OBJECT
    static confBase *createInstance();

public:
    explicit confBase(QObject *parent = nullptr);
    static confBase* instance();

    QString vdiretorioImagensMedio;
    QString vdiretorioImagensGrandes;
    QString vdiretorioImagensPequenas;
    QString vimagemStaple;
    QString vimagemAvatar;
    void fcriaDiretoriosBase();
//    void fcriaArquivoLog();

signals:

public slots:
};

#endif // CONFBASE_H
