#ifndef DIRECTORYENUMS_H
#define DIRECTORYENUMS_H
#include <QObject>
#include <QMetaEnum>
#include <QVariant>

class DirectoryEnums : public QObject{
    Q_OBJECT
public:
    enum directoryType{IMAGE, CONFIGURATIONFILE, LIST, TORRENT};
    Q_ENUM(directoryType)
    enum directoryImageType{SMALL, MEDIUM, BIG,AVATAR, STAPLE = 0};
    Q_ENUM(directoryImageType)
    virtual ~DirectoryEnums() = 0;

    static directoryType QStringToDirectoryType(QString enumName){
        if(enumName.compare(enumToQString(directoryType::IMAGE), Qt::CaseInsensitive) == 0) return directoryType::IMAGE;
        else if(enumName.compare(enumToQString(directoryType::CONFIGURATIONFILE), Qt::CaseInsensitive) == 0) return directoryType::CONFIGURATIONFILE;
        else if(enumName.compare(enumToQString(directoryType::LIST), Qt::CaseInsensitive) == 0) return directoryType::LIST;
        else if(enumName.compare(enumToQString(directoryType::TORRENT), Qt::CaseInsensitive) == 0) return directoryType::TORRENT;
        return directoryType::IMAGE;
    }
    static directoryImageType QStringToImageType(QString enumName){
        if(enumName.compare(enumToQString(directoryImageType::SMALL), Qt::CaseInsensitive) == 0) return directoryImageType::SMALL;
        else if(enumName.compare(enumToQString(directoryImageType::MEDIUM), Qt::CaseInsensitive) == 0) return directoryImageType::MEDIUM;
        else if(enumName.compare(enumToQString(directoryImageType::BIG), Qt::CaseInsensitive) == 0) return directoryImageType::BIG;
        else if(enumName.compare(enumToQString(directoryImageType::AVATAR), Qt::CaseInsensitive) == 0) return directoryImageType::AVATAR;
        else if(enumName.compare(enumToQString(directoryImageType::STAPLE), Qt::CaseInsensitive) == 0) return directoryImageType::STAPLE;
        return directoryImageType::STAPLE;
    }

    static QString enumToQString(directoryType enumName){
        return QVariant::fromValue(enumName).toString();
    }
    static QString enumToQString(directoryImageType enumName){
        return QVariant::fromValue(enumName).toString();
    }

};
#endif // DIRECTORYENUMS_H
