#include "NowWatchingController.h"

#include <QTimer>

#include "../../application/media/MediaTitleResolver.h"
#include "../../application/media/ProgressFormatter.h"

#include <algorithm>
#include <utility>

namespace {
QString ListStatusKey(const UserListStatus status) {
    switch (status) {
    case UserListStatus::Current: return QStringLiteral("current");
    case UserListStatus::Planning: return QStringLiteral("planning");
    case UserListStatus::OnHold: return QStringLiteral("on-hold");
    case UserListStatus::Dropped: return QStringLiteral("dropped");
    case UserListStatus::Completed: return QStringLiteral("completed");
    case UserListStatus::Unknown: break;
    }
    return {};
}

QString SourceName(const QString &pluginId) {
    return pluginId == QStringLiteral("plex") ? QStringLiteral("Plex") : pluginId;
}

void InsertText(QVariantMap &map, const QString &key, const QString &value) {
    if (!value.trimmed().isEmpty()) map.insert(key, value);
}
}

NowWatchingController::NowWatchingController(ActiveSessionRegistry &registry, IMediaReader &media,
                                             std::function<void(const QString &)> warning, QObject *parent)
    : QObject(parent), registry_(registry), media_(media), warning_(std::move(warning)) {
    // Registry changes arrive inside plugin callbacks; the UI refreshes once they return.
    registry_.setChangedCallback([this] { QTimer::singleShot(0, this, &NowWatchingController::Refresh); });
}

NowWatchingController::~NowWatchingController() {
    // The registry outlives the controller; never leave it calling into a destroyed object.
    registry_.setChangedCallback({});
}

void NowWatchingController::setPreferredTitleKey(const QString &key) {
    preferredTitleKey_ = key;
    Refresh();
}

void NowWatchingController::setCoverSource(std::function<QString(int)> coverSource) {
    coverSource_ = std::move(coverSource);
    Refresh();
}

void NowWatchingController::SelectSession(const QString &key) {
    for (const auto &entry : sessions_) {
        if (entry.toMap().value(QStringLiteral("key")).toString() == key) {
            selectedKey_ = key;
            Refresh();
            return;
        }
    }
}

void NowWatchingController::Refresh() {
    const auto active = registry_.sessions();
    QHash<int, Media> library;
    const bool needsLibrary = std::any_of(active.cbegin(), active.cend(),
                                          [](const ActiveSession &entry) { return entry.candidate.mediaId().has_value(); });
    if (needsLibrary) {
        QList<Media> media;
        QString error;
        if (media_.readAll(media, error)) {
            for (const auto &item : media) library.insert(item.Id, item);
        } else if (warning_) {
            warning_(QStringLiteral("Now watching could not read the local library: %1").arg(error));
        }
    }

    // The selected session survives while it is open; otherwise the most recent one is shown.
    const bool selectionAlive = std::any_of(active.cbegin(), active.cend(), [this](const ActiveSession &entry) {
        return entry.session.id.key() == selectedKey_;
    });
    if (!selectionAlive) selectedKey_ = active.isEmpty() ? QString() : active.last().session.id.key();

    sessions_.clear();
    current_.clear();
    for (const auto &entry : active) {
        auto card = buildCurrent(entry, library);
        const auto key = entry.session.id.key();
        const auto title = card.value(QStringLiteral("animeTitle"), card.value(QStringLiteral("sourceTitle"))).toString();
        card.insert(QStringLiteral("key"), key);
        card.insert(QStringLiteral("label"), QStringLiteral("%1 · %2 · %3").arg(title, card.value(QStringLiteral("user")).toString(),
                                                                                SourceName(entry.session.id.pluginId())));
        card.insert(QStringLiteral("selected"), key == selectedKey_);
        if (key == selectedKey_) current_ = card;
        sessions_.append(card);
    }
    emit changed();
}

QVariantMap NowWatchingController::buildCurrent(const ActiveSession &entry, const QHash<int, Media> &library) const {
    QVariantMap card;
    const auto &details = entry.session.details;
    const auto &candidate = entry.candidate;
    const auto mediaId = candidate.mediaId();
    const bool associated = mediaId.has_value() && library.contains(*mediaId);
    card.insert(QStringLiteral("associated"), associated);
    if (associated) {
        const auto &media = library.value(*mediaId);
        card.insert(QStringLiteral("animeTitle"), ResolveMediaTitle(media, preferredTitleKey_));
        card.insert(QStringLiteral("progress"), FormatProgress(media.ConsumedChapters, media.TotalChapters));
        if (coverSource_) InsertText(card, QStringLiteral("coverUrl"), coverSource_(media.Id));
        card.insert(QStringLiteral("mediaId"), media.Id);
        InsertText(card, QStringLiteral("animeSynopsis"), media.Synopsis);
        InsertText(card, QStringLiteral("listStatus"), ListStatusKey(media.ListStatus));
    }
    if (candidate.season()) card.insert(QStringLiteral("season"), *candidate.season());
    card.insert(QStringLiteral("episode"), candidate.episode());
    InsertText(card, QStringLiteral("sourceTitle"), details.sourceTitle.isEmpty() ? candidate.title() : details.sourceTitle);
    InsertText(card, QStringLiteral("episodeTitle"), details.title);
    InsertText(card, QStringLiteral("summary"), details.summary);
    if (details.durationSeconds && *details.durationSeconds > 0) {
        card.insert(QStringLiteral("duration"), QStringLiteral("%1 min").arg((*details.durationSeconds + 30) / 60));
    }
    if (details.airDate && details.airDate->isValid()) {
        card.insert(QStringLiteral("airDate"), details.airDate->toString(QStringLiteral("dd/MM/yyyy")));
    }
    card.insert(QStringLiteral("source"), SourceName(entry.session.id.pluginId()));
    // Reported by the player; the card stays active while paused.
    card.insert(QStringLiteral("paused"), entry.state == PlaybackState::Paused);
    // Names the player reported win; ids are only shown when no name exists.
    const auto &session = entry.session;
    InsertText(card, QStringLiteral("user"), session.userName.isEmpty() ? session.userId : session.userName);
    InsertText(card, QStringLiteral("device"), session.deviceName.isEmpty() ? session.deviceId : session.deviceName);
    return card;
}
