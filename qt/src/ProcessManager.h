#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit ProcessManager(QObject *parent = nullptr);
    ~ProcessManager() override;

    void startNginx(const QString &configPath);
    void stopNginx();
    void stopAll();

    static QString resolveNginxBin();

signals:
    void nginxStarted();
    void nginxFailed(const QString &message);
    void nginxCrashed();

private:
    void killStrayNginx();

    QProcess *m_nginx = nullptr;
};
