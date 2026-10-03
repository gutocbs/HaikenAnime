#include "LocalLibraryRecognitionCoordinator.h"

#include "../infrastructure/library/AnitomyLocalFileRecognizer.h"
#include "../infrastructure/logging/AsyncLogger.h"
#include <QMetaObject>
#include <QThread>

namespace {
QString StateName(LocalRecognitionState state) {
    switch (state) {
    case LocalRecognitionState::Recognized: return QStringLiteral("recognized");
    case LocalRecognitionState::Associated: return QStringLiteral("associated");
    case LocalRecognitionState::Ambiguous: return QStringLiteral("ambiguous");
    case LocalRecognitionState::Unsupported: return QStringLiteral("unsupported");
    case LocalRecognitionState::Unprocessed: return QStringLiteral("unprocessed");
    case LocalRecognitionState::Unrecognized: return QStringLiteral("unrecognized");
    }
    return QStringLiteral("unrecognized");
}
}

LocalLibraryRecognitionCoordinator::LocalLibraryRecognitionCoordinator(RepositoryFactory factory, QObject *parent)
    : QObject(parent), factory_(std::move(factory)) {}

LocalLibraryRecognitionCoordinator::~LocalLibraryRecognitionCoordinator() { shutdown(); }

void LocalLibraryRecognitionCoordinator::setLogger(AsyncLogger *logger) {
    logger_ = logger;
}

bool LocalLibraryRecognitionCoordinator::start(const QString &rootPath) {
    if (active_ || stopping_ || rootPath.trimmed().isEmpty()) return false;
    active_ = true; stopping_ = false;
    thread_ = QThread::create([this, rootPath] {
        QString error;
        auto repository = factory_ ? factory_(error) : nullptr;
        qsizetype processed = 0, associated = 0, unrecognized = 0, ambiguous = 0;
        if (!repository) {
            if (error.isEmpty()) error = QStringLiteral("Cannot create local recognition repository.");
        } else {
            QList<LocalFileRecognitionRecord> pending;
            QList<Media> catalog;
            if (!repository->readPendingRecognition(rootPath, pending, error)
                || !repository->readRecognitionCatalog(catalog, error)) {
                // error already populated
            } else {
                AnitomyLocalFileRecognizer recognizer;
                LocalMediaResolver resolver;
                QList<LocalFileRecognitionRecord> updates;
                for (auto record : pending) {
                    const auto recognition = recognizer.recognize(record.fileName);
                    const auto match = resolver.resolve(recognition, catalog);
                    record.recognitionState = StateName(match.state);
                    record.extractedTitle = recognition.extractedTitle;
                    record.mediaKind = recognition.mediaKind == LocalMediaKind::Anime ? QStringLiteral("anime") : QStringLiteral("unsupported");
                    record.season = recognition.season;
                    record.episode = recognition.episode;
                    record.mediaId = match.mediaId > 0 ? std::optional<int>(match.mediaId) : std::nullopt;
                    record.diagnostic = match.diagnostic.isEmpty() ? recognition.diagnostic : match.diagnostic;
                    updates.append(record); ++processed;
                    if (record.recognitionState == QStringLiteral("associated")) ++associated;
                    else if (record.recognitionState == QStringLiteral("ambiguous")) ++ambiguous;
                    else ++unrecognized;
                    if (updates.size() == 200) {
                        if (!repository->saveRecognitionBatch(updates, error)) break;
                        QMetaObject::invokeMethod(this, [this] { emit batchPersisted(); }, Qt::QueuedConnection);
                        updates.clear();
                    }
                }
                if (error.isEmpty() && !updates.isEmpty()) {
                    if (repository->saveRecognitionBatch(updates, error)) {
                        QMetaObject::invokeMethod(this, [this] { emit batchPersisted(); }, Qt::QueuedConnection);
                    }
                }
            }
        }
        if (logger_) {
            const auto summary = QStringLiteral("Local library recognition completed: processed=%1, associated=%2, "
                                                "unrecognized=%3, ambiguous=%4, failed=%5")
                                     .arg(processed)
                                     .arg(associated)
                                     .arg(unrecognized)
                                     .arg(ambiguous)
                                     .arg(error.isEmpty() ? 0 : 1);
            if (error.isEmpty()) logger_->info(LogCategory::LocalLibrary, summary);
            else logger_->error(LogCategory::LocalLibrary,
                                 QStringLiteral("%1, error=%2").arg(summary, error));
        }
        QMetaObject::invokeMethod(this, [this, error, processed, associated, unrecognized, ambiguous] {
            active_ = false; delete thread_; thread_ = nullptr;
            if (error.isEmpty()) emit completed(processed, associated, unrecognized, ambiguous);
            else emit failed(error);
        }, Qt::QueuedConnection);
    });
    thread_->start(QThread::LowPriority);
    return true;
}

void LocalLibraryRecognitionCoordinator::shutdown() {
    stopping_ = true;
    if (thread_) { thread_->wait(); delete thread_; thread_ = nullptr; }
    active_ = false;
}
