#ifndef HAIKENANIME_GRAPHQLQUERYSTORE_H
#define HAIKENANIME_GRAPHQLQUERYSTORE_H

#include <QString>

/** Loads GraphQL operations from external files instead of embedding them in C++ code. */
class GraphQlQueryStore final {
public:
    /** Creates a query store for the supplied query file path. */
    explicit GraphQlQueryStore(QString filePath);

    /** Reads the complete query and reports filesystem errors through error. */
    [[nodiscard]] bool load(QString &query, QString &error) const;

private:
    QString filePath_;
};

#endif // HAIKENANIME_GRAPHQLQUERYSTORE_H
