#include "NetworkService.h"

#include <QHostAddress>
#include <QNetworkInterface>

namespace NetworkService {

QString getLANIP()
{
    const auto interfaces = QNetworkInterface::allInterfaces();

    // Prefer common Wi-Fi / Ethernet names when present
    const QStringList preferred = {
        QStringLiteral("wlan0"), QStringLiteral("wlp"), QStringLiteral("wifi"),
        QStringLiteral("eth0"),  QStringLiteral("enp"), QStringLiteral("eno"),
        QStringLiteral("Ethernet"), QStringLiteral("Wi-Fi"), QStringLiteral("WiFi"),
    };

    QString fallback;

    for (const QNetworkInterface &iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp)
            || !(iface.flags() & QNetworkInterface::IsRunning)
            || (iface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress addr = entry.ip();
            if (addr.protocol() != QAbstractSocket::IPv4Protocol)
                continue;
            if (addr.isLoopback())
                continue;

            const QString ip = addr.toString();
            const QString name = iface.name() + iface.humanReadableName();

            for (const QString &pref : preferred) {
                if (name.contains(pref, Qt::CaseInsensitive))
                    return ip;
            }

            if (fallback.isEmpty())
                fallback = ip;
        }
    }

    return fallback;
}

} // namespace NetworkService
