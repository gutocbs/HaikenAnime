#ifndef HAIKENANIME_IMEDIAREPOSITORY_H
#define HAIKENANIME_IMEDIAREPOSITORY_H

#include "IMediaReader.h"
#include "IMediaWriter.h"
#include "IPersonalListMediaWriter.h"
#include "IMediaSnapshotReconciler.h"

/** Provides persistence operations required by media use cases. */
class IMediaRepository : public IMediaReader, public IMediaWriter,
                         public IPersonalListMediaWriter, public IMediaSnapshotReconciler {
public:
    virtual ~IMediaRepository() = default;
};

#endif // HAIKENANIME_IMEDIAREPOSITORY_H
