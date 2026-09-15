#ifndef FILTROS_H
#define FILTROS_H

#include <QObject>
#include <QVector>

class Filtros : public QObject
{
    Q_OBJECT
public:
    explicit Filtros(QObject *parent = nullptr){Q_UNUSED(parent)}
    QString nome;
    QString filtroDeCondicao;
    QString filtroEspecifico;
    QString lista;
    QStringList idAnimesAfetados;
    bool Ativo;

signals:

public slots:
};

#endif // FILTROS_H
