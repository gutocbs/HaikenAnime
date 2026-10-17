#include "RecognitionSyncFeedback.h"

#include <utility>
#include <variant>

RecognitionSyncFeedback::RecognitionSyncFeedback(RecognitionHistoryService &history,
                                                 std::function<void(const QString &)> logger)
    : history_(history), logger_(std::move(logger)) {}

void RecognitionSyncFeedback::changesSent(const QList<AniListPendingChange> &changes) {
    settle(changes, std::nullopt);
}

void RecognitionSyncFeedback::changesFailed(const QList<AniListPendingChange> &changes,
                                            const AniListSyncErrorCategory category) {
    settle(changes, SafeError(category));
}

QString RecognitionSyncFeedback::SafeError(const AniListSyncErrorCategory category) {
    // Fixed texts only: the outbox error may echo request headers, so it never reaches the history.
    if (IsRetryable(category)) {
        return QStringLiteral("falha temporária de comunicação; nova tentativa automática");
    }
    switch (category) {
    case AniListSyncErrorCategory::Authentication:
    case AniListSyncErrorCategory::Authorization:
        return QStringLiteral("conta do AniList não autorizada; reconecte a conta");
    case AniListSyncErrorCategory::GraphQl:
    case AniListSyncErrorCategory::InvalidData:
        return QStringLiteral("o AniList recusou a alteração de progresso");
    case AniListSyncErrorCategory::Persistence:
        return QStringLiteral("falha ao registrar o envio localmente");
    default:
        return QStringLiteral("falha desconhecida ao enviar o progresso");
    }
}

void RecognitionSyncFeedback::settle(const QList<AniListPendingChange> &changes,
                                     const std::optional<QString> &safeError) {
    for (const auto &change : changes) {
        if (change.field != AniListField::Progress) continue;
        const auto *progress = std::get_if<int>(&change.newValue);
        if (progress == nullptr) continue;

        QString error;
        QList<RecognitionRecord> records;
        if (!history_.findAwaitingSync(change.mediaId, *progress, records, error)) {
            log(QStringLiteral("Recognition sync outcome not recorded for media %1: %2.").arg(change.mediaId).arg(error));
            continue;
        }
        for (const auto &record : records) {
            const bool saved = safeError.has_value() ? history_.markSyncFailure(record.id(), *safeError, error)
                                                     : history_.markSynced(record.id(), error);
            if (!saved) {
                log(QStringLiteral("Recognition sync outcome not recorded for media %1, record %2: %3.")
                        .arg(change.mediaId).arg(record.id()).arg(error));
            }
        }
    }
}

void RecognitionSyncFeedback::log(const QString &message) const {
    if (logger_) logger_(message);
}
