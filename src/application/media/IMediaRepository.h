#ifndef HAIKENANIME_IMEDIAREPOSITORY_H
#define HAIKENANIME_IMEDIAREPOSITORY_H

#include "IMediaReader.h"
#include "IMediaWriter.h"

/** Provides persistence operations required by media use cases. */
class IMediaRepository : public IMediaReader, public IMediaWriter {
public:
    virtual ~IMediaRepository() = default;
};

#endif // HAIKENANIME_IMEDIAREPOSITORY_H
