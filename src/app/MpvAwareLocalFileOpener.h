#ifndef HAIKENANIME_MPVAWARELOCALFILEOPENER_H
#define HAIKENANIME_MPVAWARELOCALFILEOPENER_H

#include "../application/library/ILocalFileOpener.h"
#include "RecognitionComposition.h"

/// Opens library episodes in mpv (with IPC, so playback is recognized) when mpv is enabled;
/// otherwise, or if mpv cannot start, falls back to the system default application.
class MpvAwareLocalFileOpener final : public ILocalFileOpener {
public:
    MpvAwareLocalFileOpener(ILocalFileOpener &fallback, RecognitionContext &recognition);

    bool open(const QString &path, QString &error) override;
    bool openEpisode(const LocalEpisode &episode, QString &error) override;

private:
    ILocalFileOpener &fallback_;
    RecognitionContext &recognition_;
};

#endif // HAIKENANIME_MPVAWARELOCALFILEOPENER_H
