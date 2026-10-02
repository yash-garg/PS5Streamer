#pragma once

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QObject>
#include <QStringList>
#include <QUdpSocket>

/// Minimal DNS server that spoofs Twitch ingest hostnames to the local LAN IP.
/// Other queries are forwarded to an upstream resolver (1.1.1.1).
/// Binds UDP :53 — requires elevated privileges on most systems.
class DnsInterceptor : public QObject
{
    Q_OBJECT

public:
    explicit DnsInterceptor(QObject *parent = nullptr);

    bool start(const QHostAddress &spoofIP, QString *errorMessage = nullptr);
    void stop();
    bool isRunning() const;

    static QStringList ingestHosts();

signals:
    void queryLogged(const QString &hostname);

private:
    struct PendingQuery {
        QHostAddress client;
        quint16 clientPort = 0;
    };

    void onClientReadyRead();
    void onUpstreamReadyRead();
    void forwardUpstream(const QByteArray &query, const QHostAddress &client, quint16 clientPort);
    QByteArray buildSpoofResponse(const QByteArray &query, const QHostAddress &spoofIP) const;
    QString parseQuestionName(const QByteArray &query, int &offset) const;
    bool isIngestHost(const QString &name) const;

    QUdpSocket m_socket;
    QUdpSocket m_upstream;
    QHostAddress m_spoofIP;
    QHostAddress m_upstreamAddr;
    QStringList m_hosts;
    QHash<quint16, PendingQuery> m_pending;
};
