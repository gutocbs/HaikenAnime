#ifndef HAIKENANIME_WINDOWSCREDENTIALMANAGER_H
#define HAIKENANIME_WINDOWSCREDENTIALMANAGER_H

#include <QByteArray>
#include <QString>

/// Generic credentials in Windows Credential Manager, persisted for the local machine. Errors carry only
/// the Windows error code, never the stored value.
namespace WindowsCredentialManager {
[[nodiscard]] bool exists(const QString &target);
/// Replaces blob with the stored value; fails (with the Windows error) when the credential is missing.
[[nodiscard]] bool read(const QString &target, QByteArray &blob, QString &error);
/// userName is stored with the credential when not empty.
[[nodiscard]] bool write(const QString &target, const QByteArray &blob, const QString &userName, QString &error);
/// A credential that does not exist counts as removed.
[[nodiscard]] bool remove(const QString &target, QString &error);
}

#endif // HAIKENANIME_WINDOWSCREDENTIALMANAGER_H
