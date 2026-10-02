#include "StreamKeyServer.h"

#include <QHostAddress>
#include <QUrlQuery>

StreamKeyServer::StreamKeyServer(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &StreamKeyServer::onNewConnection);
}

bool StreamKeyServer::start(quint16 port)
{
    stop();
    return m_server.listen(QHostAddress::LocalHost, port);
}

void StreamKeyServer::stop()
{
    m_server.close();
}

void StreamKeyServer::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        handleSocket(socket);
    }
}

void StreamKeyServer::handleSocket(QTcpSocket *socket)
{
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        const QByteArray data = socket->readAll();

        QString app;
        QString key;
        if (parseStreamInfo(data, &app, &key))
            emit keyDetected(app, key);

        const QByteArray response =
            "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        socket->write(response);
        socket->disconnectFromHost();
    });

    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

bool StreamKeyServer::parseStreamInfo(const QByteArray &request, QString *app, QString *key)
{
    const int sep = request.indexOf("\r\n\r\n");
    if (sep < 0)
        return false;

    const QByteArray body = request.mid(sep + 4);
    if (body.isEmpty())
        return false;

    QUrlQuery query(QString::fromUtf8(body));
    const QString name = query.queryItemValue(QStringLiteral("name"));
    if (name.isEmpty())
        return false;

    *key = name;
    *app = query.queryItemValue(QStringLiteral("app"));
    if (app->isEmpty())
        *app = QStringLiteral("app");
    return true;
}
