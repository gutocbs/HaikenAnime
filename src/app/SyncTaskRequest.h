#ifndef HAIKENANIME_SYNCTASKREQUEST_H
#define HAIKENANIME_SYNCTASKREQUEST_H

#include "../application/anilist/IAniListDataSource.h"
#include "../application/scheduling/SyncTaskTypes.h"

namespace SyncTaskRequest {
inline AniListDataSourceRequest ForState(const SyncTaskState &state) {
    auto request = AniListDataSourceRequest::ForPartition(state.partition);
    if (state.confirmedPage.has_value()) request.setPage(*state.confirmedPage + 1);
    return request;
}
}

#endif // HAIKENANIME_SYNCTASKREQUEST_H
