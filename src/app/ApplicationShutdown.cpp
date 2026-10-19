#include "ApplicationShutdown.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QThread>

namespace {
constexpr qint64 AdaptiveSyncShutdownTimeoutMs = 5'000;

void LogShutdownStep(const ApplicationContext &context, const QString &message) {
    if (context.logger) context.logger->info(LogCategory::Application, message);
}

bool WaitForAdaptiveSyncStop(ApplicationContext &context) {
    if (!context.adaptiveSync || context.adaptiveSync->isStopped()) return true;

    context.adaptiveSync->shutdown();
    QElapsedTimer elapsed;
    elapsed.start();
    while (!context.adaptiveSync->isStopped() && !elapsed.hasExpired(AdaptiveSyncShutdownTimeoutMs)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }
    return context.adaptiveSync->isStopped();
}

void ShutdownServices(ApplicationContext &context, const QElapsedTimer &total) {
    if (context.servicesShutdown) return;
    context.servicesShutdown = true;
    QElapsedTimer step;
    if (context.localLibraryScan) {
        step.start();
        context.localLibraryScan->shutdown();
        LogShutdownStep(context, QStringLiteral("Local library scan stopped in %1 ms.").arg(step.elapsed()));
    }
    if (context.localLibraryRecognition) {
        step.start();
        context.localLibraryRecognition->shutdown();
        LogShutdownStep(context, QStringLiteral("Local library recognition stopped in %1 ms.").arg(step.elapsed()));
    }
    if (context.recognition && context.recognition->recognition) {
        step.start();
        context.recognition->recognition->stop();
        *context.recognitionProgressListener = {};
        LogShutdownStep(context, QStringLiteral("Episode recognition stopped in %1 ms.").arg(step.elapsed()));
    }
    if (context.coverCoordinator) {
        context.coverCoordinator->disconnect();
    }
    // Logged before the logger stops, otherwise the last line would be dropped.
    LogShutdownStep(context, QStringLiteral("Application shutdown completed in %1 ms.").arg(total.elapsed()));
    if (context.logger) {
        context.logger->stop();
    }
}
}

void ShutdownApplication(ApplicationContext &context) {
    // On a timeout, ApplicationContext destruction still owns the runtime and joins its thread;
    // dependent services intentionally remain alive until that has happened.
    QElapsedTimer total;
    total.start();
    LogShutdownStep(context, QStringLiteral("Application shutdown started."));
    if (!WaitForAdaptiveSyncStop(context)) {
        if (context.logger) {
            context.logger->warning(LogCategory::Sync,
                                    QStringLiteral("Adaptive synchronization did not stop before shutdown timeout "
                                                   "(elapsed %1 ms).").arg(total.elapsed()));
        }
        // Later services stay alive for the context destructor; still record the total before exit.
        LogShutdownStep(context, QStringLiteral("Application shutdown completed in %1 ms.").arg(total.elapsed()));
        return;
    }
    LogShutdownStep(context, QStringLiteral("Adaptive synchronization stopped in %1 ms.").arg(total.elapsed()));
    ShutdownServices(context, total);
}
