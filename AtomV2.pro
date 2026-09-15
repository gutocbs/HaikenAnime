QT       += core gui network networkauth concurrent qml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14 openssl openssl-linked

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    anilist.cpp \
    anitomy/anitomy.cpp \
    anitomy/element.cpp \
    anitomy/keyword.cpp \
    anitomy/parser.cpp \
    anitomy/parser_helper.cpp \
    anitomy/parser_number.cpp \
    anitomy/string.cpp \
    anitomy/token.cpp \
    anitomy/tokenizer.cpp \
    arquivos.cpp \
    checainfoplayer.cpp \
    confbase.cpp \
    confusuario.cpp \
    filedownloader.cpp \
    formatapalavras.cpp \
    janeladeconfig.cpp \
    janelatorrent.cpp \
    leitorlistaanimes.cpp \
    logger.cpp \
    main.cpp \
    mainwindow.cpp \
    robotlib/Bounds.cc \
#    robotlib/Clipboard.cc \
#    robotlib/Color.cc \
#    robotlib/Hash.cc \
#    robotlib/Image.cc \
#    robotlib/Keyboard.cc \
#    robotlib/Memory.cc \
    robotlib/Module.cc \
#    robotlib/Mouse.cc \
    robotlib/Point.cc \
    robotlib/Process.cc \
#    robotlib/Range.cc \
#    robotlib/Screen.cc \
    robotlib/Size.cc \
#    robotlib/Timer.cc \
    robotlib/Window.cc \

HEADERS += \
    anilist.h \
    anime.h \
    anitomy/anitomy.h \
    anitomy/element.h \
    anitomy/keyword.h \
    anitomy/options.h \
    anitomy/parser.h \
    anitomy/string.h \
    anitomy/token.h \
    anitomy/tokenizer.h \
    arquivos.h \
    checainfoplayer.h \
    confbase.h \
    confusuario.h \
    filedownloader.h \
    filtros.h \
    formatapalavras.h \
    janeladeconfig.h \
    janelatorrent.h \
    leitorlistaanimes.h \
    logger.h \
    mainwindow.h \
    robotlib/Bounds.h \
#    robotlib/Clipboard.h \
#    robotlib/Color.h \
#    robotlib/Enum.h \
#    robotlib/Global.h \
#    robotlib/Hash.h \
#    robotlib/Image.h \
#    robotlib/Keyboard.h \
#    robotlib/Memory.h \
    robotlib/Module.h \
#    robotlib/Mouse.h \
    robotlib/Point.h \
    robotlib/Process.h \
#    robotlib/Range.h \
#    robotlib/Robot.h \
#    robotlib/Screen.h \
    robotlib/Size.h \
#    robotlib/Timer.h \
#    robotlib/Types.h \
    robotlib/Window.h \
    torrentinfo.h \

FORMS += \
    janeladeconfig.ui \
    janelatorrent.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    visuais.qrc

LIBS += libpsapi
#unix|win32: LIBS += -L$$PWD/robotlib/ -lRobot

#INCLUDEPATH += $$PWD/robotlib
#DEPENDPATH += $$PWD/robotlib
