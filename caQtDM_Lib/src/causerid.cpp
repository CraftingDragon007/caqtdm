#include "causerid.h"

#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <sddl.h>
#include <vector>
#else
#include <unistd.h>
#endif

QString getUniqueUserId()
{
#ifdef Q_OS_WIN
    // On Windows, we use the user's Security Identifier (SID)
    // converted to a string. This ensures uniqueness per user.

    // Get the current username
    DWORD usernameBufferSize = 0;
    // Call GetUserNameW once to get the required buffer size
    GetUserNameW(nullptr, &usernameBufferSize);

    if (usernameBufferSize == 0) {
        qWarning("getUserUniqueId: Failed to get username buffer size. Error: %d", GetLastError());
        return QString();
    }

    std::vector<WCHAR> usernameBuffer(usernameBufferSize);
    if (!GetUserNameW(usernameBuffer.data(), &usernameBufferSize)) {
        qWarning("getUserUniqueId: Failed to get username. Error: %d", GetLastError());
        return QString();
    }
    QString username = QString::fromWCharArray(usernameBuffer.data());

    // Look up the SID for the username
    DWORD sidBufferSize = 0;
    DWORD domainBufferSize = 0;
    SID_NAME_USE snu; // Placeholder, not used for size calculation

    // Call LookupAccountNameW once to get required buffer sizes for SID and domain
    LookupAccountNameW(
        nullptr, // system name (local machine)
        usernameBuffer.data(),
        nullptr, // SID buffer
        &sidBufferSize,
        nullptr, // Domain name buffer
        &domainBufferSize,
        &snu
        );

    // Check if LookupAccountNameW failed for reasons other than buffer size
    if (sidBufferSize == 0 || domainBufferSize == 0) {
        DWORD err = GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER) {
            qWarning("getUserUniqueId: LookupAccountNameW failed for buffer sizing. Error: %d", err);
            return QString();
        }
    }

    std::vector<BYTE> sidBuffer(sidBufferSize);
    std::vector<WCHAR> domainBuffer(domainBufferSize);

    // Call LookupAccountNameW a second time to get the actual SID and domain
    if (!LookupAccountNameW(
            nullptr, // system name (local machine)
            usernameBuffer.data(),
            reinterpret_cast<PSID>(sidBuffer.data()),
            &sidBufferSize,
            domainBuffer.data(),
            &domainBufferSize,
            &snu
            )) {
        qWarning("getUserUniqueId: LookupAccountNameW failed. Error: %d", GetLastError());
        return QString();
    }

    // Convert the binary SID to a human-readable string
    LPWSTR sidString = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<PSID>(sidBuffer.data()), &sidString)) {
        qWarning("getUserUniqueId: ConvertSidToStringSidW failed. Error: %d", GetLastError());
        return QString();
    }

    QString userSid = QString::fromWCharArray(sidString);
    LocalFree(sidString);

    return userSid;

#else // Q_OS_UNIX (Linux, macOS, etc.)
    uid_t uid = getuid();
    return QString::number(uid);
#endif
}
