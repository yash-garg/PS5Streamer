#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class ConfigGenerator;
class DnsInterceptor;
class ProcessManager;
class StreamKeyServer;

enum class ServiceStatus {
    Stopped,
    Starting,
    Running,
    Error
};

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(QString localIP READ localIP NOTIFY localIPChanged)
    Q_PROPERTY(QString streamKey READ streamKey NOTIFY streamKeyChanged)
    Q_PROPERTY(QString obsURL READ obsURL NOTIFY streamKeyChanged)
    Q_PROPERTY(QString dnsStatusLabel READ dnsStatusLabel NOTIFY dnsStatusChanged)
    Q_PROPERTY(QString rtmpStatusLabel READ rtmpStatusLabel NOTIFY rtmpStatusChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    bool isRunning() const { return m_running; }
    QString localIP() const { return m_localIP; }
    QString streamKey() const { return m_streamKey; }
    QString obsURL() const;
    QString streamApp() const { return m_streamApp; }
    ServiceStatus dnsStatus() const { return m_dnsStatus; }
    ServiceStatus rtmpStatus() const { return m_rtmpStatus; }
    QString dnsStatusLabel() const;
    QString rtmpStatusLabel() const;
    QStringList logs() const { return m_logs; }

public slots:
    void start();
    void stop();
    void clearLogs();

signals:
    void runningChanged();
    void localIPChanged();
    void streamKeyChanged();
    void dnsStatusChanged();
    void rtmpStatusChanged();
    void logAdded(const QString &line);
    void logsCleared();

private:
    void setDnsStatus(ServiceStatus status, const QString &error = {});
    void setRtmpStatus(ServiceStatus status, const QString &error = {});
    void log(const QString &message);
    static QString statusLabel(ServiceStatus status, const QString &error);

    bool m_running = false;
    QString m_localIP;
    QString m_streamKey;
    QString m_streamApp = QStringLiteral("app");
    ServiceStatus m_dnsStatus = ServiceStatus::Stopped;
    ServiceStatus m_rtmpStatus = ServiceStatus::Stopped;
    QString m_dnsError;
    QString m_rtmpError;
    QStringList m_logs;

    ProcessManager *m_processManager = nullptr;
    ConfigGenerator *m_configGenerator = nullptr;
    StreamKeyServer *m_streamKeyServer = nullptr;
    DnsInterceptor *m_dnsInterceptor = nullptr;
};
