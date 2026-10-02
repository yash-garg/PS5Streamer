#include "Privilege.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif

namespace Privilege {

bool isElevated()
{
#ifdef Q_OS_WIN
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
#else
    return geteuid() == 0;
#endif
}

#ifdef Q_OS_WIN
bool relaunchElevated(QString *errorMessage)
{
    const QString app = QCoreApplication::applicationFilePath();
    const QByteArray appUtf = app.toUtf8();
    HINSTANCE result = ShellExecuteW(
        nullptr,
        L"runas",
        reinterpret_cast<LPCWSTR>(app.utf16()),
        nullptr,
        nullptr,
        SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(result) <= 32) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Administrator elevation was cancelled or failed.");
        return false;
    }
    return true;
}
#else
static QString findTool(const QStringList &names)
{
    for (const QString &name : names) {
        const QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty())
            return path;
    }
    return {};
}

bool relaunchElevated(QString *errorMessage)
{
    const QString app = QFileInfo(QCoreApplication::applicationFilePath()).absoluteFilePath();
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // Preserve GUI session variables for the elevated process
    const QStringList keep = {
        QStringLiteral("DISPLAY"),
        QStringLiteral("XAUTHORITY"),
        QStringLiteral("WAYLAND_DISPLAY"),
        QStringLiteral("XDG_RUNTIME_DIR"),
        QStringLiteral("XDG_SESSION_TYPE"),
        QStringLiteral("QT_QPA_PLATFORM"),
        QStringLiteral("QT_QPA_PLATFORMTHEME"),
        QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
        QStringLiteral("XDG_CURRENT_DESKTOP"),
        QStringLiteral("LANG"),
        QStringLiteral("LC_ALL"),
    };

    QStringList envArgs;
    for (const QString &key : keep) {
        if (env.contains(key) && !env.value(key).isEmpty())
            envArgs << QStringLiteral("%1=%2").arg(key, env.value(key));
    }

    const QString pkexec = findTool({QStringLiteral("pkexec")});
    if (!pkexec.isEmpty()) {
        QStringList args;
        args << QStringLiteral("env") << envArgs << app;
        if (QProcess::startDetached(pkexec, args))
            return true;
    }

    const QString kdesu = findTool({QStringLiteral("kdesu"), QStringLiteral("kdesudo")});
    if (!kdesu.isEmpty()) {
        // kdesu -c "env ... /path/app"
        QString command = QStringLiteral("env");
        for (const QString &e : envArgs)
            command += QLatin1Char(' ') + e;
        command += QLatin1Char(' ') + app;
        if (QProcess::startDetached(kdesu, {QStringLiteral("-c"), command}))
            return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "Could not open a password prompt (pkexec/kdesu missing). "
            "Run manually: sudo %1").arg(app);
    }
    return false;
}
#endif

} // namespace Privilege
