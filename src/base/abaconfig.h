#ifndef ABACONFIG_H
#define ABACONFIG_H

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QDebug>
#include <QMetaEnum>

#include "src/utilities/singleton.h"
class abaConfig : public QObject
{
    Q_OBJECT
    static abaConfig *createInstance();

public:
    explicit abaConfig(QObject *parent = nullptr);
    static abaConfig* instance();
    enum clients{ANILIST, MYANIMELIST, KITSU};
    Q_ENUM(clients)

    QVariant fgetService();
    QVariant fgetUsername();
    QVariant fgetAuthCode();
    void fsaveAuthcode();
    void fsetAuthCode(QVariant);
    void fsaveSettings();
    void fsetService(QVariant);
    void fsetUsername(QVariant);

private:

    QSettings vsettings;
    QMetaEnum vmetaEnum;

    clients vclientEscolhido;
    QString vuser;
    QVariant vauthCode;

    //torrents
    //Filtros
    //Recognition

};

#endif // ABACONFIG_H
