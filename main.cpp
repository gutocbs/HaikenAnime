#include "janeladeconfig.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    //Posso dar upscale na minha aplicação se fizer isso
//    QApplication* temp = new QApplication(argc, argv);
//    QPointer<QScreen> screen = QGuiApplication::primaryScreen();
//    QRect screenGeometry = screen->geometry();
//    double width = screenGeometry.width();
//    // assumes that the default desktop resolution is 720p (scale of 1)
//    int minWidth = 1920;
//    delete temp;

//    double scale = width / minWidth;
//    std::string scaleAsString = std::to_string(scale);
//    QByteArray scaleAsQByteArray(scaleAsString.c_str(), scaleAsString.length());
//    qputenv("QT_SCALE_FACTOR", scaleAsQByteArray);
//    qDebug() << scaleAsQByteArray;

    //Pòsso refazer tudo em 720p e só dar scale com isso
//    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("gutocbs");
    //    QCoreApplication::setOrganizationDomain("github.com");
    QCoreApplication::setApplicationName("Atom");
    MainWindow w;
//    w.setSizeIncrement(w.width()*scale, w.height()*scale);
    janeladeconfig jconfig;
    if(!jconfig.fretornaUsuario().isEmpty())
        w.setWindowTitle(jconfig.fretornaUsuario()+" - Atom");
    else
        w.setWindowTitle("Atom");
//    w.setWindowIcon();
    w.showMaximized();
    return a.exec();
}
