#include "MpvAwareLocalFileOpener.h"

MpvAwareLocalFileOpener::MpvAwareLocalFileOpener(ILocalFileOpener &fallback, RecognitionContext &recognition)
    : fallback_(fallback), recognition_(recognition) {}

bool MpvAwareLocalFileOpener::open(const QString &path, QString &error) {
    return fallback_.open(path, error);
}

bool MpvAwareLocalFileOpener::openEpisode(const LocalEpisode &episode, QString &error) {
    if (recognition_.isMpvEnabled() && episode.mediaId > 0 && episode.episode > 0) {
        // The library already identified the episode, so the candidate is certain.
        const auto candidate = EpisodeCandidate::create({}, episode.mediaId, std::nullopt, episode.episode, 1.0);
        QString launchError;
        if (recognition_.mpv->launchEpisode(episode.path, {.candidate = candidate}, launchError)) {
            error.clear();
            return true;
        }
        recognition_.Warn(QStringLiteral("mpv could not be started; the default player was used."));
    }
    return fallback_.open(episode.path, error);
}
