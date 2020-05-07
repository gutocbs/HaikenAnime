#ifndef CONFBASE_H
#define CONFBASE_H

#include <QObject>
#include <QDir> //Cria diretorios
#include <QFile> //Cria arquivos
#include <QDebug>

class confBase : public QObject
{
    Q_OBJECT
public:
    explicit confBase(QObject *parent = nullptr);
    QString vdiretorioImagensMedio;
    QString vdiretorioImagensGrandes;
    QString vdiretorioImagensPequenas;
    QString vimagemStaple;
    void fcriaDiretoriosBase();
//    void fcriaArquivoLog();

signals:

public slots:
};

#endif // CONFBASE_H
