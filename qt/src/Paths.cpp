#include "Paths.h"

#include <QDir>

namespace Paths {

static QString nginxPath(const QString &relative)
{
    // nginx config always wants forward slashes, including on Windows
    return QDir::fromNativeSeparators(workDir() + QLatin1Char('/') + relative);
}

QString workDir()
{
    return QDir::fromNativeSeparators(QDir::homePath() + QStringLiteral("/.ps5streamer"));
}

QString nginxConf()
{
    return nginxPath(QStringLiteral("nginx.conf"));
}

QString nginxPid()
{
    return nginxPath(QStringLiteral("nginx.pid"));
}

QString nginxErrLog()
{
    return nginxPath(QStringLiteral("nginx-error.log"));
}

QString dnsmasqConf()
{
    return nginxPath(QStringLiteral("dnsmasq.conf"));
}

void createWorkDir()
{
    QDir().mkpath(workDir());
    QDir().mkpath(workDir() + QStringLiteral("/logs"));
    QDir().mkpath(workDir() + QStringLiteral("/temp"));
    QDir().mkpath(workDir() + QStringLiteral("/html"));
    QDir().mkpath(workDir() + QStringLiteral("/conf"));
}

} // namespace Paths
