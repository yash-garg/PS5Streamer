#include "ConfigGenerator.h"
#include "Paths.h"

#include <QFile>
#include <QTextStream>

#ifndef Q_OS_WIN
#include <grp.h>
#include <pwd.h>
#endif

GeneratedConfigs ConfigGenerator::generate(const QString & /*hostIP*/)
{
    Paths::createWorkDir();

    QFile nginxFile(Paths::nginxConf());
    if (nginxFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&nginxFile);
        out << nginxConfig();
        nginxFile.close();
    }

    GeneratedConfigs configs;
    configs.nginxConfig = Paths::nginxConf();
    return configs;
}

QString ConfigGenerator::nginxUserDirective() const
{
#ifdef Q_OS_WIN
    // Windows build ignores the Unix user directive
    return {};
#else
    // Ubuntu-built nginx defaults to group "nogroup", which does not exist on
    // Fedora/RHEL (they use "nobody"). Pick a pair that exists locally.
    const bool hasNobodyUser = getpwnam("nobody") != nullptr;
    const bool hasNobodyGroup = getgrnam("nobody") != nullptr;
    const bool hasNogroup = getgrnam("nogroup") != nullptr;

    if (hasNobodyUser && hasNobodyGroup)
        return QStringLiteral("user nobody nobody;\n");
    if (hasNobodyUser && hasNogroup)
        return QStringLiteral("user nobody nogroup;\n");
    if (getpwnam("nginx") && getgrnam("nginx"))
        return QStringLiteral("user nginx nginx;\n");
    // Local interceptor tool: keep workers as root if nothing else fits
    return QStringLiteral("user root;\n");
#endif
}

QString ConfigGenerator::nginxConfig() const
{
    const QString err = Paths::nginxErrLog();
    const QString pid = Paths::nginxPid();
    const QString userLine = nginxUserDirective();

    return userLine + QStringLiteral(
        "worker_processes 1;\n"
        "error_log \"%1\" warn;\n"
        "pid       \"%2\";\n"
        "\n"
        "events {\n"
        "    worker_connections 512;\n"
        "}\n"
        "\n"
        "rtmp {\n"
        "    server {\n"
        "        listen 1935;\n"
        "        chunk_size 4096;\n"
        "\n"
        "        application app {\n"
        "            live on;\n"
        "            record off;\n"
        "            sync 10ms;\n"
        "            on_publish http://127.0.0.1:9988/on_publish;\n"
        "            # push rtmp://live.twitch.tv/app/YOUR_TWITCH_KEY;\n"
        "        }\n"
        "\n"
        "        application live2 {\n"
        "            live on;\n"
        "            record off;\n"
        "            on_publish http://127.0.0.1:9988/on_publish;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "\n"
        "http {\n"
        "    access_log off;\n"
        "    server {\n"
        "        listen 8080;\n"
        "        location /stat { rtmp_stat all; }\n"
        "    }\n"
        "}\n"
    ).arg(err, pid);
}
