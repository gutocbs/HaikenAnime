QT += quick network networkauth concurrent

CONFIG += c++14

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        lib/anitomy/anitomy.cpp \
        lib/anitomy/element.cpp \
        lib/anitomy/keyword.cpp \
        lib/anitomy/parser.cpp \
        lib/anitomy/parser_helper.cpp \
        lib/anitomy/parser_number.cpp \
        lib/anitomy/string.cpp \
        lib/anitomy/token.cpp \
        lib/anitomy/tokenizer.cpp \
        lib/robotlib/Bounds.cc \
#        lib/robotlib/Clipboard.cc \
#        lib/robotlib/Color.cc \
#        lib/robotlib/Hash.cc \
#        lib/robotlib/Image.cc \
#        lib/robotlib/Keyboard.cc \
#        lib/robotlib/Memory.cc \
        lib/robotlib/Module.cc \
#        lib/robotlib/Mouse.cc \
        lib/robotlib/Point.cc \
        lib/robotlib/Process.cc \
#        lib/robotlib/Range.cc \
#        lib/robotlib/Screen.cc \
        lib/robotlib/Size.cc \
#        lib/robotlib/Timer.cc \
        lib/robotlib/Window.cc \
        main.cpp \
        mainclass.cpp \
        src/base/abaconfig.cpp \
        src/base/abatorrent.cpp \
        src/base/arquivos.cpp \
        src/base/confbase.cpp \
        src/base/confusuario.cpp \
        src/base/downloaderworker.cpp \
        src/base/leitorlistaanimes.cpp \
        src/clients/anilist.cpp \
        src/utilities/client.cpp \
        src/utilities/downloader.cpp \
        src/utilities/formatapalavras.cpp \
        src/utilities/logger.cpp \
    src/utilities/playerrecognition.cpp

RESOURCES += qml.qrc \
    queries.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += libpsapi

HEADERS += \
    lib/anitomy/anitomy.h \
    lib/anitomy/element.h \
    lib/anitomy/keyword.h \
    lib/anitomy/options.h \
    lib/anitomy/parser.h \
    lib/anitomy/string.h \
    lib/anitomy/token.h \
    lib/anitomy/tokenizer.h \
    lib/robotlib/Bounds.h \
#    lib/robotlib/Clipboard.h \
#    lib/robotlib/Color.h \
#    lib/robotlib/Enum.h \
#    lib/robotlib/Global.h \
#    lib/robotlib/Hash.h \
#    lib/robotlib/Image.h \
#    lib/robotlib/Keyboard.h \
#    lib/robotlib/Memory.h \
    lib/robotlib/Module.h \
#    lib/robotlib/Mouse.h \
    lib/robotlib/Point.h \
    lib/robotlib/Process.h \
#    lib/robotlib/Range.h \
#    lib/robotlib/Robot.h \
#    lib/robotlib/Screen.h \
    lib/robotlib/Size.h \
#    lib/robotlib/Timer.h \
#    lib/robotlib/Types.h \
    lib/robotlib/Window.h \
    mainclass.h \
    src/base/abatorrent.h \
    src/base/abaconfig.h \
    src/base/anime.h \
    src/base/arquivos.h \
    src/base/confbase.h \
    src/base/confusuario.h \
    src/base/downloaderworker.h \
    src/base/leitorlistaanimes.h \
    src/base/torrentinfo.h \
    src/clients/anilist.h \
    src/utilities/call_once.h \
    src/utilities/client.h \
    src/utilities/downloader.h \
    src/utilities/formatapalavras.h \
    src/utilities/logger.h \
    src/utilities/playerrecognition.h \
    src/utilities/singleton.h \
