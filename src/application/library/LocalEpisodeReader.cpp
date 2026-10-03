#include "LocalEpisodeReader.h"

#include <QSqlError>
#include <QSqlQuery>

LocalEpisodeReader::LocalEpisodeReader(QSqlDatabase database, QString query)
    : database_(std::move(database)), query_(std::move(query)) {}

bool LocalEpisodeReader::readNextEpisode(int mediaId, int consumedEpisode,
                                         LocalEpisode &episode, QString &error) {
    error.clear();
    episode = {};
    if (mediaId <= 0 || consumedEpisode < 0 || !database_.isValid() || !database_.isOpen()) {
        error = QStringLiteral("Invalid local episode reader input or database connection.");
        return false;
    }
    QSqlQuery query(database_);
    if (!query.prepare(query_)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":media_id"), mediaId);
    query.bindValue(QStringLiteral(":consumed_episode"), consumedEpisode);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    if (!query.next()) return false;
    episode.mediaId = query.value(0).toInt();
    episode.episode = query.value(1).toInt();
    episode.path = query.value(2).toString();
    if (episode.mediaId <= 0 || episode.episode <= consumedEpisode || episode.path.isEmpty()) {
        error = QStringLiteral("The next local episode query returned invalid data.");
        episode = {};
        return false;
    }
    return true;
}
