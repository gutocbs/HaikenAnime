#include "logger.h"

QString logger::varquivo = QDir::currentPath() + QDir::separator() + "Configurações/Temp/" + QDir::separator() + "log.txt";
bool logger::vlogging = false;
static const QtMessageHandler QT_DEFAULT_MESSAGE_HANDLER = qInstallMessageHandler(nullptr);

logger::logger(QObject *parent) : QObject(parent)
{

}

void logger::fattachLogger(){
    QFile llog("Configurações/Temp/log.txt");
    if(llog.exists())
        llog.remove();

    vlogging = true;
    qInstallMessageHandler(fhandlerMensagem);
}

void logger::fhandlerMensagem(QtMsgType rtipo, const QMessageLogContext &rcontexto, const QString &rmensagem){
    if(vlogging){
        bool lcrash = false;
        QString lmensagemLog;
        switch (rtipo) {
            case QtInfoMsg:
                lmensagemLog = QString("Info: %1").arg(rmensagem);
            break;
            case QtDebugMsg:
                #ifdef QT_DEBUG
                    lmensagemLog = QString("Debug: %1").arg(rmensagem);
                #else
                    lmensagemLog = QString(rmensagem);
                #endif
            break;
            case QtCriticalMsg:
                lmensagemLog = QString("Critical Error: %1").arg(rmensagem);
            break;
            case QtWarningMsg:
                lmensagemLog = QString("Error: %1").arg(rmensagem);
            break;
            case QtFatalMsg:
                lmensagemLog = QString("FATAL ERROR: %1").arg(rmensagem);
                lcrash = true;
            break;
        }
        QFile lfile(logger::varquivo);
        if(lfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream lstreamTexto(&lfile);
            QString larquivo = rcontexto.file;
            larquivo.remove(larquivo.lastIndexOf('.'), 4);
            #ifdef QT_DEBUG
                lstreamTexto << lmensagemLog << " - " << larquivo.mid(10) << " - " << rcontexto.line << endl;
            #else
                lstreamTexto << lmensagemLog << endl;
            #endif
            lstreamTexto.flush();
            lfile.close();
        }
        if(lcrash)
            lfile.copy(QDir::currentPath() + QDir::separator() + "Configurações/Temp/" + QDir::separator() + "log-crash.txt");
    }
    (*QT_DEFAULT_MESSAGE_HANDLER)(rtipo, rcontexto, rmensagem);
}
