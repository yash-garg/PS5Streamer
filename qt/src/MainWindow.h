#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>

class AppController;
class QLabel;
class QPushButton;
class QListWidget;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AppController *controller, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onToggle();
    void refreshUI();
    void onLogAdded(const QString &line);
    void onCopyUrl();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupTray();
    QWidget *makeDivider();
    QColor statusColor(int status) const;
    void setDotColor(QLabel *dot, int status);
    bool ensureElevated();

    AppController *m_controller = nullptr;
    QPushButton *m_toggleBtn = nullptr;
    QLabel *m_rtmpDot = nullptr;
    QLabel *m_dnsDot = nullptr;
    QLabel *m_ipValue = nullptr;
    QLabel *m_urlValue = nullptr;
    QPushButton *m_copyBtn = nullptr;
    QLabel *m_urlHint = nullptr;
    QWidget *m_urlRow = nullptr;
    QListWidget *m_logList = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    bool m_forceQuit = false;
};
