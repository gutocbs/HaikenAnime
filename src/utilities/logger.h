#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QDebug>
#include <QFile> //Para poder editar o arquivo
#include <QDir> //Criar o arquivo de log
#include <QDateTime> //Marcar horário no log
#include <QTextStream> //Escrever mensagem
#include <iostream> //Switch

class logger : public QObject
{
    Q_OBJECT
public:
    explicit logger(QObject *parent = nullptr);
    static bool vlogging;
    static QString varquivo;
    static void fattachLogger();
    static void fhandlerMensagem(QtMsgType rtipo, const QMessageLogContext &rcontexto, const QString &rmensagem);
signals:

public slots:
};

#endif // LOGGER_H
