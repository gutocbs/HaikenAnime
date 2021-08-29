#ifndef TESTMANAGER_H
#define TESTMANAGER_H

#include <QObject>
#include <QTest>
#include "tests/listmanagertests.h"
#include "tests/medialoadertests.h"

class TestManager : public QObject
{
    Q_OBJECT
public:
    explicit TestManager(QObject *parent = nullptr);
    void executeAllTests();

signals:
private:
    void listManagerTests();
    void mediaLoaderTests();
};

#endif // TESTMANAGER_H
