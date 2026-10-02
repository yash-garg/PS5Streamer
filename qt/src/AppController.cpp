#include "AppController.h"

#include "ConfigGenerator.h"
#include "DnsInterceptor.h"
#include "NetworkService.h"
#include "Paths.h"
#include "ProcessManager.h"
#include "StreamKeyServer.h"

#include <QDateTime>
#include <QHostAddress>

namespace {
QString u8(const char *s)
{
    return QString::fromUtf8(s);
}
} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_localIP(NetworkService::getLANIP())
    , m_processManager(new ProcessManager(this))
    , m_configGenerator(new ConfigGenerator())
    , m_streamKeyServer(new StreamKeyServer(this))
    , m_dnsInterceptor(new DnsInterceptor(this))
{
    if (m_localIP.isEmpty())
        m_localIP = QStringLiteral("Not found");

    connect(m_processManager, &ProcessManager::nginxStarted, this, [this]() {
        setRtmpStatus(ServiceStatus::Running);
        log(u8("✅ RTMP server running on :1935"));

        setDnsStatus(ServiceStatus::Starting);
        QString dnsError;
        if (!m_dnsInterceptor->start(QHostAddress(m_localIP), &dnsError)) {
            setDnsStatus(ServiceStatus::Error, dnsError);
            log(u8("❌ DNS: ") + dnsError);
            m_processManager->stopNginx();
            setRtmpStatus(ServiceStatus::Stopped);
            m_streamKeyServer->stop();
            return;
        }
        setDnsStatus(ServiceStatus::Running);
        log(u8("✅ DNS interceptor running on :53"));

        m_running = true;
        emit runningChanged();
        log(u8("🚀 Ready — broadcast from PS5 via Twitch"));
    });

    connect(m_processManager, &ProcessManager::nginxFailed, this, [this](const QString &msg) {
        setRtmpStatus(ServiceStatus::Error, msg);
        log(u8("❌ RTMP: ") + msg);
        m_streamKeyServer->stop();
    });

    connect(m_processManager, &ProcessManager::nginxCrashed, this, [this]() {
        if (!m_running)
            return;
        setRtmpStatus(ServiceStatus::Error, QStringLiteral("nginx crashed"));
        m_streamKey.clear();
        emit streamKeyChanged();
        log(u8("❌ nginx crashed — check ") + Paths::nginxErrLog());
        log(u8("ℹ️ Click Stop then Start to recover"));
    });

    connect(m_streamKeyServer, &StreamKeyServer::keyDetected, this,
            [this](const QString &app, const QString &key) {
                m_streamApp = app;
                m_streamKey = key;
                emit streamKeyChanged();
                log(u8("🎮 PS5 connected via /") + app + u8("/ — stream key detected"));
            });

    connect(m_dnsInterceptor, &DnsInterceptor::queryLogged, this, [this](const QString &host) {
        log(u8("🔍 DNS spoof: ") + host + u8(" → ") + m_localIP);
    });
}

AppController::~AppController()
{
    stop();
    delete m_configGenerator;
}

QString AppController::obsURL() const
{
    if (m_streamKey.isEmpty())
        return {};
    return QStringLiteral("rtmp://127.0.0.1/%1/%2").arg(m_streamApp, m_streamKey);
}

QString AppController::dnsStatusLabel() const
{
    return statusLabel(m_dnsStatus, m_dnsError);
}

QString AppController::rtmpStatusLabel() const
{
    return statusLabel(m_rtmpStatus, m_rtmpError);
}

QString AppController::statusLabel(ServiceStatus status, const QString &error)
{
    switch (status) {
    case ServiceStatus::Stopped:
        return QStringLiteral("Stopped");
    case ServiceStatus::Starting:
        return u8("Starting…");
    case ServiceStatus::Running:
        return QStringLiteral("Running");
    case ServiceStatus::Error:
        return QStringLiteral("Error: %1").arg(error);
    }
    return {};
}

void AppController::start()
{
    if (m_running)
        return;

    m_localIP = NetworkService::getLANIP();
    if (m_localIP.isEmpty())
        m_localIP = QStringLiteral("Not found");
    emit localIPChanged();

    const GeneratedConfigs configs = m_configGenerator->generate(m_localIP);

    setRtmpStatus(ServiceStatus::Starting);
    m_streamKeyServer->start();
    m_processManager->startNginx(configs.nginxConfig);
}

void AppController::stop()
{
    m_dnsInterceptor->stop();
    m_processManager->stopAll();
    m_streamKeyServer->stop();

    m_running = false;
    m_streamKey.clear();
    setDnsStatus(ServiceStatus::Stopped);
    setRtmpStatus(ServiceStatus::Stopped);
    emit runningChanged();
    emit streamKeyChanged();
    log(u8("⏹ Stopped — all services cleaned up"));
}

void AppController::clearLogs()
{
    m_logs.clear();
    emit logsCleared();
}

void AppController::setDnsStatus(ServiceStatus status, const QString &error)
{
    m_dnsStatus = status;
    m_dnsError = error;
    emit dnsStatusChanged();
}

void AppController::setRtmpStatus(ServiceStatus status, const QString &error)
{
    m_rtmpStatus = status;
    m_rtmpError = error;
    emit rtmpStatusChanged();
}

void AppController::log(const QString &message)
{
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    const QString line = QStringLiteral("[%1] %2").arg(ts, message);
    m_logs.prepend(line);
    while (m_logs.size() > 200)
        m_logs.removeLast();
    emit logAdded(line);
}
