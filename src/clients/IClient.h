#ifndef ICLIENT_H
#define ICLIENT_H

#include <QObject>
#include <QVariant>
#include <QThread>
#include "src/utilities/Enums.h"
class IClient : public QObject{
public:
    //TODO - Transformar todas as funções voids em bool ou int
    using QObject::QObject;
    virtual bool getAvatarAndMediaList() = 0;
    virtual void setThread(QThread &cThread) = 0;
    virtual void setAuthCode(const QString &user, QVariant authCode) = 0;
    virtual bool updateList(int mediaId, Enums::mediaList mediaList) = 0;
    virtual bool updateScore(int mediaId, int mediaScore) = 0;
    virtual bool updateProgress(int mediaId, int mediaProgress) = 0;
    virtual bool deleteMediaFromList(int mediaId) = 0;
    virtual bool getMediaList() = 0;
    virtual bool getYearlyLists() = 0;
    virtual QString getAvatar() = 0;
protected:
};
#endif // ICLIENT_H
