#include "DnsInterceptor.h"

DnsInterceptor::DnsInterceptor(QObject *parent)
    : QObject(parent)
    , m_upstreamAddr(QStringLiteral("1.1.1.1"))
    , m_hosts(ingestHosts())
{
    connect(&m_socket, &QUdpSocket::readyRead, this, &DnsInterceptor::onClientReadyRead);
    connect(&m_upstream, &QUdpSocket::readyRead, this, &DnsInterceptor::onUpstreamReadyRead);
}

QStringList DnsInterceptor::ingestHosts()
{
    return {
        QStringLiteral("contribute.live-video.net"),
        QStringLiteral("ingest.global-contribute.live-video.net"),
        QStringLiteral("live.twitch.tv"),
        QStringLiteral("live-sin.twitch.tv"),
        QStringLiteral("live-nrt.twitch.tv"),
        QStringLiteral("live-syd.twitch.tv"),
        QStringLiteral("live-fra.twitch.tv"),
        QStringLiteral("live-ams.twitch.tv"),
        QStringLiteral("live-lhr.twitch.tv"),
        QStringLiteral("live-jfk.twitch.tv"),
        QStringLiteral("live-lax.twitch.tv"),
        QStringLiteral("live-sea.twitch.tv"),
    };
}

bool DnsInterceptor::start(const QHostAddress &spoofIP, QString *errorMessage)
{
    stop();
    m_spoofIP = spoofIP;

    if (!m_socket.bind(QHostAddress(QHostAddress::AnyIPv4), quint16(53),
                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot bind UDP :53 (%1). Run as administrator / root.")
                                .arg(m_socket.errorString());
        }
        return false;
    }

    if (!m_upstream.bind(QHostAddress(QHostAddress::AnyIPv4), quint16(0))) {
        m_socket.close();
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot open upstream DNS socket (%1).")
                                .arg(m_upstream.errorString());
        }
        return false;
    }

    return true;
}

void DnsInterceptor::stop()
{
    m_pending.clear();
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.close();
    if (m_upstream.state() != QAbstractSocket::UnconnectedState)
        m_upstream.close();
}

bool DnsInterceptor::isRunning() const
{
    return m_socket.state() == QAbstractSocket::BoundState;
}

bool DnsInterceptor::isIngestHost(const QString &name) const
{
    const QString lower = name.toLower();
    for (const QString &host : m_hosts) {
        if (lower == host || lower.endsWith(QLatin1Char('.') + host))
            return true;
    }
    return false;
}

void DnsInterceptor::onClientReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket.pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        m_socket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        if (datagram.size() < 12)
            continue;

        int offset = 12;
        const QString name = parseQuestionName(datagram, offset);
        if (name.isEmpty()) {
            forwardUpstream(datagram, sender, senderPort);
            continue;
        }

        if (isIngestHost(name)) {
            emit queryLogged(name);
            const QByteArray response = buildSpoofResponse(datagram, m_spoofIP);
            if (!response.isEmpty())
                m_socket.writeDatagram(response, sender, senderPort);
            continue;
        }

        forwardUpstream(datagram, sender, senderPort);
    }
}

void DnsInterceptor::forwardUpstream(const QByteArray &query, const QHostAddress &client, quint16 clientPort)
{
    if (query.size() < 2)
        return;

    const quint16 id = (quint8(query.at(0)) << 8) | quint8(query.at(1));
    m_pending.insert(id, {client, clientPort});
    m_upstream.writeDatagram(query, m_upstreamAddr, 53);
}

void DnsInterceptor::onUpstreamReadyRead()
{
    while (m_upstream.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_upstream.pendingDatagramSize()));
        m_upstream.readDatagram(datagram.data(), datagram.size());

        if (datagram.size() < 2)
            continue;

        const quint16 id = (quint8(datagram.at(0)) << 8) | quint8(datagram.at(1));
        auto it = m_pending.find(id);
        if (it == m_pending.end())
            continue;

        m_socket.writeDatagram(datagram, it->client, it->clientPort);
        m_pending.erase(it);
    }
}

QString DnsInterceptor::parseQuestionName(const QByteArray &query, int &offset) const
{
    QStringList labels;
    while (offset < query.size()) {
        const quint8 len = quint8(query.at(offset));
        ++offset;
        if (len == 0)
            break;
        if ((len & 0xC0) == 0xC0) {
            offset += 1;
            break;
        }
        if (offset + len > query.size())
            return {};
        labels << QString::fromLatin1(query.constData() + offset, len);
        offset += len;
    }
    return labels.join(QLatin1Char('.'));
}

QByteArray DnsInterceptor::buildSpoofResponse(const QByteArray &query, const QHostAddress &spoofIP) const
{
    if (query.size() < 12)
        return {};

    int offset = 12;
    parseQuestionName(query, offset);
    offset += 4; // QTYPE + QCLASS
    if (offset > query.size())
        return {};

    QByteArray response = query.left(offset);

    response[2] = char(0x81);
    response[3] = char(0x80);
    response[6] = 0;
    response[7] = 1;
    response[8] = 0;
    response[9] = 0;
    response[10] = 0;
    response[11] = 0;

    response.append(char(0xC0));
    response.append(char(0x0C));
    response.append(char(0x00));
    response.append(char(0x01)); // TYPE A
    response.append(char(0x00));
    response.append(char(0x01)); // CLASS IN
    response.append(char(0x00));
    response.append(char(0x00));
    response.append(char(0x00));
    response.append(char(0x1E)); // TTL 30
    response.append(char(0x00));
    response.append(char(0x04));

    const quint32 ip = spoofIP.toIPv4Address();
    response.append(char((ip >> 24) & 0xFF));
    response.append(char((ip >> 16) & 0xFF));
    response.append(char((ip >> 8) & 0xFF));
    response.append(char(ip & 0xFF));

    return response;
}
