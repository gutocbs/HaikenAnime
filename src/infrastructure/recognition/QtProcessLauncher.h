#ifndef HAIKENANIME_QTPROCESSLAUNCHER_H
#define HAIKENANIME_QTPROCESSLAUNCHER_H

#include "mpv/MpvPlugin.h"

/// Starts detached processes with QProcess; the player keeps running if HaikenAnime closes.
class QtProcessLauncher final : public IProcessLauncher {
public:
    bool start(const QString &program, const QStringList &arguments, QString &error) override;
    [[nodiscard]] bool canStart(const QString &program) const override;
};

#endif // HAIKENANIME_QTPROCESSLAUNCHER_H
