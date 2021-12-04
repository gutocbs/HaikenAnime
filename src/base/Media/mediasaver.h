#ifndef MEDIASAVER_H
#define MEDIASAVER_H

#include <QObject>

class MediaSaver : public QObject
{
    Q_OBJECT
public:
    explicit MediaSaver(QObject *parent = nullptr);

signals:

};

#endif // MEDIASAVER_H
