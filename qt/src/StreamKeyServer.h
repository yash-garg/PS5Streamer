#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

/// Tiny HTTP server on :9988 for nginx-rtmp on_publish callbacks.
class StreamKeyServer : public QObject
{
    Q_OBJECT

public:
    explicit StreamKeyServer(QObject *parent = nullptr);

    bool start(quint16 port = 9988);
    void stop();

signals:
    void keyDetected(const QString &app, const QString &key);

private:
    void onNewConnection();
    void handleSocket(QTcpSocket *socket);
    static bool parseStreamInfo(const QByteArray &request, QString *app, QString *key);

    QTcpServer m_server;
};
