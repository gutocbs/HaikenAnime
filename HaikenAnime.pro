QT += quick network networkauth concurrent testlib

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


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
    lib/robotlib/Module.h \
    lib/robotlib/Point.h \
    lib/robotlib/Process.h \
    lib/robotlib/Size.h \
    lib/robotlib/Window.h \
    mainclass.h \
    src/DTO/media.h \
    src/base/Anime/animelistmanager.h \
    src/base/Anime/animemanager.h \
    src/base/Anime/mediacontroller.h \
    src/base/Anime/medialoader.h \
    src/base/Anime/mediasaver.h \
    src/base/Anime/mediasearchmanager.h \
    src/base/FileManager/filemanager.h \
    src/base/FileManager/filemanagerloader.h \
    src/base/FileManager/filemanagersaver.h \
    src/base/FileManager/mediadirectories.h \
    src/base/abaconfig.h \
    src/base/abatorrent.h \
    src/base/anime.h \
    src/base/arquivos.h \
    src/base/confbase.h \
    src/base/confusuario.h \
    src/base/database.h \
    src/base/downloaderworker.h \
    src/base/leitorlistaanimes.h \
    src/base/torrentinfo.h \
    src/clients/anilist.h \
    src/utilities/Enums.h \
    src/utilities/Interfaces/IMediaListManager.h \
    src/utilities/Interfaces/IMediaManager.h \
    src/utilities/Interfaces/IMediaSearchManager.h \
    src/utilities/Media/mediautil.h \
    src/utilities/Media/mediacomparer.h \
    src/utilities/arquivoanime.h \
    src/utilities/call_once.h \
    src/utilities/client.h \
    src/utilities/downloader.h \
    src/utilities/formatapalavras.h \
    src/utilities/logger.h \
    src/utilities/playerrecognition.h \
    src/utilities/singleton.h \
    tests/listmanagertests.h \
    tests/medialoadertests.h \
    tests/testmanager.h

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
    lib/robotlib/Module.cc \
    lib/robotlib/Point.cc \
    lib/robotlib/Process.cc \
    lib/robotlib/Size.cc \
    lib/robotlib/Window.cc \
    main.cpp \
    mainclass.cpp \
    src/base/Anime/animelistmanager.cpp \
    src/base/Anime/animemanager.cpp \
    src/base/Anime/mediacontroller.cpp \
    src/base/Anime/medialoader.cpp \
    src/base/Anime/mediasaver.cpp \
    src/base/Anime/mediasearchmanager.cpp \
    src/base/FileManager/filemanager.cpp \
    src/base/FileManager/filemanagerloader.cpp \
    src/base/FileManager/filemanagersaver.cpp \
    src/base/abaconfig.cpp \
    src/base/abatorrent.cpp \
    src/base/arquivos.cpp \
    src/base/confbase.cpp \
    src/base/confusuario.cpp \
    src/base/database.cpp \
    src/base/downloaderworker.cpp \
    src/base/leitorlistaanimes.cpp \
    src/clients/anilist.cpp \
    src/utilities/Media/mediautil.cpp \
    src/utilities/Media/mediacomparer.cpp \
    src/utilities/client.cpp \
    src/utilities/downloader.cpp \
    src/utilities/formatapalavras.cpp \
    src/utilities/logger.cpp \
    src/utilities/playerrecognition.cpp \
    tests/listmanagertests.cpp \
    tests/medialoadertests.cpp \
    tests/testmanager.cpp


RESOURCES += \
    Mocks.qrc \
    qml.qrc \
    queries.qrc
