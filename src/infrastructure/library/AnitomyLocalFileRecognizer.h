#ifndef HAIKENANIME_ANITOMYLOCALFILERECOGNIZER_H
#define HAIKENANIME_ANITOMYLOCALFILERECOGNIZER_H

#include "../../application/library/ILocalFileRecognizer.h"

class AnitomyLocalFileRecognizer final : public ILocalFileRecognizer {
public:
    LocalFileRecognition recognize(const QString &fileName,
                                   LocalMediaKind requestedKind = LocalMediaKind::Anime) const override;
};

#endif
