#include "ProcessManager.h"
#include "Paths.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QTimer>

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
{
}

ProcessManager::~ProcessManager()
{
    stopAll();
}

QString ProcessManager::resolveNginxBin()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
#ifdef Q_OS_WIN
        appDir + QStringLiteral("/Binaries/nginx.exe"),
        appDir + QStringLiteral("/nginx.exe"),
        QStringLiteral("C:/nginx/nginx.exe"),
#else
        appDir + QStringLiteral("/Binaries/nginx"),
        appDir + QStringLiteral("/nginx"),
        QStringLiteral("/usr/sbin/nginx"),
        QStringLiteral("/usr/bin/nginx"),
        QStringLiteral("/usr/local/sbin/nginx"),
        QStringLiteral("/usr/local/bin/nginx"),
#endif
    };

    for (const QString &path : candidates) {
        if (QFileInfo::exists(path) && QFileInfo(path).isFile())
            return QFileInfo(path).absoluteFilePath();
    }

#ifdef Q_OS_WIN
    return QStringLiteral("nginx.exe");
#else
    return QStringLiteral("nginx");
#endif
}

void ProcessManager::startNginx(const QString &configPath)
{
    Paths::createWorkDir();

    const QString bin = resolveNginxBin();
    if (!QFileInfo::exists(bin)) {
        emit nginxFailed(
#ifdef Q_OS_WIN
            QStringLiteral("nginx not found. Expected Binaries/nginx.exe next to the app.")
#else
            QStringLiteral("nginx not found. Expected Binaries/nginx next to the app.")
#endif
        );
        return;
    }

    killStrayNginx();

    if (m_nginx) {
        m_nginx->kill();
        m_nginx->deleteLater();
        m_nginx = nullptr;
    }

    m_nginx = new QProcess(this);
    m_nginx->setProcessChannelMode(QProcess::MergedChannels);
    // Prefix = work dir so relative logs/temp resolve; config is absolute.
    m_nginx->setWorkingDirectory(Paths::workDir());

    connect(m_nginx, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
                if (exitCode != 0)
                    emit nginxCrashed();
            });

    const QStringList args = {
        QStringLiteral("-p"), Paths::workDir() + QLatin1Char('/'),
        QStringLiteral("-c"), configPath,
        QStringLiteral("-g"), QStringLiteral("daemon off;"),
    };
    m_nginx->start(bin, args);

    if (!m_nginx->waitForStarted(3000)) {
        const QString err = QStringLiteral("%1 (%2)").arg(m_nginx->errorString(), bin);
        m_nginx->deleteLater();
        m_nginx = nullptr;
        emit nginxFailed(err);
        return;
    }

    QTimer::singleShot(1000, this, [this, bin]() {
        if (!m_nginx)
            return;
        if (m_nginx->state() != QProcess::Running) {
            QString msg = QString::fromUtf8(m_nginx->readAll()).trimmed();
            if (msg.isEmpty())
                msg = QStringLiteral("nginx exited immediately (%1)").arg(bin);
            emit nginxFailed(msg);
            m_nginx->deleteLater();
            m_nginx = nullptr;
            return;
        }
        emit nginxStarted();
    });
}

void ProcessManager::stopNginx()
{
    if (!m_nginx)
        return;

    m_nginx->disconnect();
    m_nginx->terminate();
    if (!m_nginx->waitForFinished(3000))
        m_nginx->kill();
    m_nginx->deleteLater();
    m_nginx = nullptr;

    QFile::remove(Paths::nginxPid());
}

void ProcessManager::stopAll()
{
    stopNginx();
}

void ProcessManager::killStrayNginx()
{
#ifdef Q_OS_WIN
    QProcess::execute(QStringLiteral("taskkill"),
                      {QStringLiteral("/F"), QStringLiteral("/IM"), QStringLiteral("nginx.exe")});
#else
    // Only kill our bundled binary path if possible
    const QString bin = resolveNginxBin();
    if (QFileInfo::exists(bin))
        QProcess::execute(QStringLiteral("pkill"), {QStringLiteral("-x"), QFileInfo(bin).fileName()});
#endif
    QThread::msleep(300);
}
