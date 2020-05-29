#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QVariant>
#include <QThread>

#include "src/clients/anilist.h"
#include "src/base/abaconfig.h"

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);
    ~Client();

    bool fmudaLista(int, const QString &rNovaLista);
    bool fmudaNota(int, int);
    bool fmudaProgresso(int, int);
    bool fexcluiAnime(int);
    bool fgetListaPorAno();
    void frecebeAutorizacao(const QString &ruser, QVariant);
    QString fretornaAvatar();
public slots:
    void fselecionaClient(QVariant);
    void fbaixaListas();
    QString fgetAvatar();

signals:
    void sdownloadCompleted(bool);

private slots:
    void fdownloadSignal(bool);

private:
    void fpassaThread();
    void fconnections();

    abaConfig::clients clientEscolhido;
    anilist *clientAnilist;

    QThread tthreadClient;
};

#endif // CLIENT_H
