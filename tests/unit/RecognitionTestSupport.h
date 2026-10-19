#ifndef HAIKENANIME_RECOGNITIONTESTSUPPORT_H
#define HAIKENANIME_RECOGNITIONTESTSUPPORT_H

#include <QList>
#include <QString>

#include <optional>

#include "../../src/application/media/IMediaReader.h"
#include "../../src/application/recognition/ProgressDecisionService.h"

/// Progress reader over a fake library: finds the media in readAll() (production reads it from SQLite).
class LibraryProgressReader final : public IMediaProgressReader {
public:
    explicit LibraryProgressReader(IMediaReader &reader) : reader_(reader) {}

    bool find(const int mediaId, std::optional<Media> &media, QString &error) override {
        media.reset();
        QList<Media> catalog;
        if (!reader_.readAll(catalog, error)) return false;
        for (const auto &item : catalog) {
            if (item.Id == mediaId) {
                media = item;
                break;
            }
        }
        return true;
    }

private:
    IMediaReader &reader_;
};

#endif // HAIKENANIME_RECOGNITIONTESTSUPPORT_H
