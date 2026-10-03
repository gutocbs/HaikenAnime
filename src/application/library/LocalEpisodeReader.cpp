#include "LocalEpisodeReader.h"

#include <QSqlError>
#include <QSqlQuery>
#include "../../infrastructure/database/SqlQueryStore.h"

LocalEpisodeReader::LocalEpisodeReader(QSqlDatabase database, QString nextEpisodeQuery,
                                       QString availableEpisodeCountQuery)
    : database_(std::move(database)), nextEpisodeQuery_(std::move(nextEpisodeQuery)),
      availableEpisodeCountQuery_(std::move(availableEpisodeCountQuery)) {}

bool LocalEpisodeReader::readAvailableEpisodeCount(int mediaId, int &count, QString &error) {
    count = 0; error.clear();
    QString querySource;
    if (mediaId <= 0 || !SqlQueryStore::loadSource(availableEpisodeCountQuery_, querySource, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(querySource)) { error = query.lastError().text(); return false; }
    query.bindValue(QStringLiteral(":media_id"), mediaId);
    if (!query.exec() || !query.next()) { error = query.lastError().text(); return false; }
    count = query.value(0).toInt(); return true;
}

bool LocalEpisodeReader::readNextEpisode(int mediaId, int consumedEpisode,
                                         LocalEpisode &episode, QString &error) {
    error.clear();
    episode = {};
    if (mediaId <= 0 || consumedEpisode < 0 || !database_.isValid() || !database_.isOpen()) {
        error = QStringLiteral("Invalid local episode reader input or database connection.");
        return false;
    }
    QString querySource;
    if (!SqlQueryStore::loadSource(nextEpisodeQuery_, querySource, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(querySource)) {
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
