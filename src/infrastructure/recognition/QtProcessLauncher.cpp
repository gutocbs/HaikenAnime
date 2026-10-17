#include "QtProcessLauncher.h"

#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

bool QtProcessLauncher::start(const QString &program, const QStringList &arguments, QString &error) {
    error.clear();
    if (!QProcess::startDetached(program, arguments)) {
        error = QStringLiteral("Could not start %1.").arg(QFileInfo(program).fileName());
        return false;
    }
    return true;
}

bool QtProcessLauncher::canStart(const QString &program) const {
    const QFileInfo file(program);
    if (file.isAbsolute()) return file.isFile() && file.isExecutable();
    return !QStandardPaths::findExecutable(program).isEmpty();
}
