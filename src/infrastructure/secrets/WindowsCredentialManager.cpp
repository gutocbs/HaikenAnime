#include "WindowsCredentialManager.h"

#include <windows.h>
#include <wincred.h>

namespace {
QString WinError(const DWORD code) {
    return QStringLiteral("Windows Credential Manager error %1.").arg(code);
}

LPCWSTR Wide(const QString &value) {
    return reinterpret_cast<LPCWSTR>(value.utf16());
}
}

namespace WindowsCredentialManager {
bool exists(const QString &target) {
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(Wide(target), CRED_TYPE_GENERIC, 0, &credential)) return false;
    CredFree(credential);
    return true;
}

bool read(const QString &target, QByteArray &blob, QString &error) {
    blob.clear();
    error.clear();
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(Wide(target), CRED_TYPE_GENERIC, 0, &credential)) {
        error = WinError(GetLastError());
        return false;
    }
    blob = QByteArray(reinterpret_cast<const char *>(credential->CredentialBlob),
                      static_cast<qsizetype>(credential->CredentialBlobSize));
    CredFree(credential);
    return true;
}

bool write(const QString &target, const QByteArray &blob, const QString &userName, QString &error) {
    error.clear();
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = reinterpret_cast<LPWSTR>(const_cast<ushort *>(target.utf16()));
    credential.CredentialBlobSize = static_cast<DWORD>(blob.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char *>(blob.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    if (!userName.isEmpty()) {
        credential.UserName = reinterpret_cast<LPWSTR>(const_cast<ushort *>(userName.utf16()));
    }
    if (!CredWriteW(&credential, 0)) {
        error = WinError(GetLastError());
        return false;
    }
    return true;
}

bool remove(const QString &target, QString &error) {
    error.clear();
    if (CredDeleteW(Wide(target), CRED_TYPE_GENERIC, 0)) return true;
    const DWORD code = GetLastError();
    if (code == ERROR_NOT_FOUND) return true;
    error = WinError(code);
    return false;
}
}
