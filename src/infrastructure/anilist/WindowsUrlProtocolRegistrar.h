#ifndef HAIKENANIME_WINDOWSURLPROTOCOLREGISTRAR_H
#define HAIKENANIME_WINDOWSURLPROTOCOLREGISTRAR_H

#include <QString>
#include <QVariant>

#include <functional>

/** Registers the desktop custom URL protocol beneath the current Windows user. */
class WindowsUrlProtocolRegistrar final {
public:
    struct Write final {
        QString registryPath;
        QString key;
        QVariant value;
    };
    using RegistryWriter = std::function<bool(const Write &, QString &)>;

    explicit WindowsUrlProtocolRegistrar(RegistryWriter writer = {});
    [[nodiscard]] bool registerProtocol(const QString &scheme, const QString &applicationPath,
                                        QString &error) const;

private:
    RegistryWriter writer_;
};

#endif // HAIKENANIME_WINDOWSURLPROTOCOLREGISTRAR_H
