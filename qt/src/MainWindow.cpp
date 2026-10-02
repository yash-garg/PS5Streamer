#include "MainWindow.h"

#include "AppController.h"
#include "Privilege.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QLabel *sectionCaption(const QString &text)
{
    auto *label = new QLabel(text);
    QFont f = label->font();
    f.setPointSize(10);
    label->setFont(f);
    label->setStyleSheet(QStringLiteral("color: rgba(128,128,128,180);"));
    return label;
}

QFont monoFont(int pointSize, bool medium = false)
{
    QFont f(QStringLiteral("Menlo"));
    if (!f.exactMatch())
        f = QFont(QStringLiteral("Consolas"));
    if (!f.exactMatch())
        f = QFont(QStringLiteral("Cascadia Mono"));
    if (!f.exactMatch())
        f = QFont(QStringLiteral("monospace"));
    f.setPointSize(pointSize);
    f.setWeight(medium ? QFont::Medium : QFont::Normal);
    return f;
}

QString u8(const char *s)
{
    return QString::fromUtf8(s);
}

} // namespace

MainWindow::MainWindow(AppController *controller, QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
{
    setWindowTitle(QStringLiteral("PS5 Streamer"));
    resize(460, 480);
    setMinimumSize(420, 440);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *header = new QWidget;
    auto *headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(16, 14, 16, 14);
    headerLay->setSpacing(10);

    auto *icon = new QLabel;
    icon->setFixedSize(14, 14);
    icon->setStyleSheet(QStringLiteral("background: #AF52DE; border-radius: 7px;"));

    auto *title = new QLabel(QStringLiteral("PS5 Streamer"));
    QFont titleFont = title->font();
    titleFont.setPointSize(15);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);

    m_toggleBtn = new QPushButton(QStringLiteral("Start"));
    m_toggleBtn->setFixedWidth(72);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    connect(m_toggleBtn, &QPushButton::clicked, this, &MainWindow::onToggle);

    headerLay->addWidget(icon, 0, Qt::AlignVCenter);
    headerLay->addWidget(title);
    headerLay->addStretch();
    headerLay->addWidget(m_toggleBtn);
    root->addWidget(header);
    root->addWidget(makeDivider());

    auto *services = new QWidget;
    auto *servicesLay = new QHBoxLayout(services);
    servicesLay->setContentsMargins(16, 12, 16, 12);
    servicesLay->setSpacing(24);

    auto addStatus = [&](QLabel **dotOut, const QString &text) {
        auto *row = new QWidget;
        auto *lay = new QHBoxLayout(row);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(8);
        auto *dot = new QLabel;
        dot->setFixedSize(8, 8);
        dot->setStyleSheet(QStringLiteral("background: #8E8E93; border-radius: 4px;"));
        auto *lab = new QLabel(text);
        lab->setFont(monoFont(12));
        lab->setStyleSheet(QStringLiteral("color: gray;"));
        lay->addWidget(dot, 0, Qt::AlignVCenter);
        lay->addWidget(lab);
        servicesLay->addWidget(row);
        *dotOut = dot;
    };
    addStatus(&m_rtmpDot, QStringLiteral("RTMP :1935"));
    addStatus(&m_dnsDot, QStringLiteral("DNS :53"));
    servicesLay->addStretch();
    root->addWidget(services);
    root->addWidget(makeDivider());

    auto *dns = new QWidget;
    auto *dnsLay = new QVBoxLayout(dns);
    dnsLay->setContentsMargins(16, 12, 16, 12);
    dnsLay->setSpacing(2);
    dnsLay->addWidget(sectionCaption(QStringLiteral("PS5 DNS")));
    m_ipValue = new QLabel;
    m_ipValue->setFont(monoFont(13, true));
    m_ipValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dnsLay->addWidget(m_ipValue);
    root->addWidget(dns);
    root->addWidget(makeDivider());

    auto *url = new QWidget;
    auto *urlLay = new QVBoxLayout(url);
    urlLay->setContentsMargins(16, 12, 16, 12);
    urlLay->setSpacing(6);
    urlLay->addWidget(sectionCaption(QStringLiteral("mpv URL")));

    m_urlRow = new QWidget;
    auto *urlRowLay = new QHBoxLayout(m_urlRow);
    urlRowLay->setContentsMargins(0, 0, 0, 0);
    urlRowLay->setSpacing(8);
    m_urlValue = new QLabel;
    m_urlValue->setFont(monoFont(11));
    m_urlValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_urlValue->setWordWrap(false);
    m_urlValue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_copyBtn = new QPushButton(QStringLiteral("Copy"));
    m_copyBtn->setFixedHeight(24);
    m_copyBtn->setCursor(Qt::PointingHandCursor);
    connect(m_copyBtn, &QPushButton::clicked, this, &MainWindow::onCopyUrl);
    urlRowLay->addWidget(m_urlValue);
    urlRowLay->addWidget(m_copyBtn);
    urlLay->addWidget(m_urlRow);

    m_urlHint = new QLabel(QStringLiteral("Start to get URL"));
    QFont hintFont = m_urlHint->font();
    hintFont.setPointSize(12);
    m_urlHint->setFont(hintFont);
    m_urlHint->setStyleSheet(QStringLiteral("color: rgba(128,128,128,180);"));
    urlLay->addWidget(m_urlHint);
    root->addWidget(url);
    root->addWidget(makeDivider());

    auto *log = new QWidget;
    auto *logLay = new QVBoxLayout(log);
    logLay->setContentsMargins(16, 12, 16, 12);
    logLay->setSpacing(6);

    auto *logHeader = new QHBoxLayout;
    logHeader->addWidget(sectionCaption(QStringLiteral("Log")));
    logHeader->addStretch();
    auto *clearBtn = new QPushButton(QStringLiteral("Clear"));
    clearBtn->setFlat(true);
    clearBtn->setCursor(Qt::PointingHandCursor);
    QFont clearFont = clearBtn->font();
    clearFont.setPointSize(11);
    clearBtn->setFont(clearFont);
    clearBtn->setStyleSheet(QStringLiteral("color: rgba(128,128,128,180); border: none;"));
    connect(clearBtn, &QPushButton::clicked, m_controller, &AppController::clearLogs);
    logHeader->addWidget(clearBtn);
    logLay->addLayout(logHeader);

    m_logList = new QListWidget;
    m_logList->setFont(monoFont(12));
    m_logList->setFixedHeight(110);
    m_logList->setFrameShape(QFrame::NoFrame);
    m_logList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_logList->setWordWrap(true);
    m_logList->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; color: gray; }"
        "QListWidget::item { padding: 1px 0; }"));
    logLay->addWidget(m_logList);
    root->addWidget(log);
    root->addWidget(makeDivider());

    auto *footer = new QWidget;
    auto *footerLay = new QHBoxLayout(footer);
    footerLay->setContentsMargins(16, 12, 16, 12);
    footerLay->addStretch();
    auto *quitBtn = new QPushButton(QStringLiteral("Quit"));
    quitBtn->setFlat(true);
    quitBtn->setCursor(Qt::PointingHandCursor);
    QFont quitFont = quitBtn->font();
    quitFont.setPointSize(12);
    quitBtn->setFont(quitFont);
    quitBtn->setStyleSheet(QStringLiteral("color: rgba(128,128,128,180); border: none;"));
    connect(quitBtn, &QPushButton::clicked, this, [this]() {
        m_forceQuit = true;
        m_controller->stop();
        qApp->quit();
    });
    footerLay->addWidget(quitBtn);
    root->addWidget(footer);

    setCentralWidget(central);

    connect(m_controller, &AppController::runningChanged, this, &MainWindow::refreshUI);
    connect(m_controller, &AppController::localIPChanged, this, &MainWindow::refreshUI);
    connect(m_controller, &AppController::streamKeyChanged, this, &MainWindow::refreshUI);
    connect(m_controller, &AppController::dnsStatusChanged, this, &MainWindow::refreshUI);
    connect(m_controller, &AppController::rtmpStatusChanged, this, &MainWindow::refreshUI);
    connect(m_controller, &AppController::logAdded, this, &MainWindow::onLogAdded);
    connect(m_controller, &AppController::logsCleared, m_logList, &QListWidget::clear);

    setupTray();
    refreshUI();
}

QWidget *MainWindow::makeDivider()
{
    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setFixedHeight(1);
    line->setStyleSheet(QStringLiteral("background: rgba(128,128,128,60); border: none;"));
    return line;
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    m_tray->setToolTip(QStringLiteral("PS5 Streamer"));

    auto *menu = new QMenu(this);
    menu->addAction(QStringLiteral("Show"), this, &QWidget::showNormal);
    menu->addAction(QStringLiteral("Start / Stop"), this, &MainWindow::onToggle);
    menu->addSeparator();
    menu->addAction(QStringLiteral("Quit"), this, [this]() {
        m_forceQuit = true;
        m_controller->stop();
        qApp->quit();
    });
    m_tray->setContextMenu(menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
    m_tray->show();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_forceQuit) {
        m_controller->stop();
        QMainWindow::closeEvent(event);
        return;
    }

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("PS5 Streamer"));
    box.setIcon(QMessageBox::Question);
    box.setText(QStringLiteral("Close PS5 Streamer?"));
    box.setInformativeText(QStringLiteral(
        "Quit completely, or keep running in the background (system tray)."));

    QPushButton *backgroundBtn = nullptr;
    if (m_tray && QSystemTrayIcon::isSystemTrayAvailable()) {
        backgroundBtn = box.addButton(QStringLiteral("Keep in background"),
                                      QMessageBox::AcceptRole);
    }
    auto *quitBtn = box.addButton(QStringLiteral("Quit"), QMessageBox::DestructiveRole);
    box.addButton(QStringLiteral("Cancel"), QMessageBox::RejectRole);
    box.setDefaultButton(backgroundBtn ? backgroundBtn : quitBtn);
    box.exec();

    if (backgroundBtn && box.clickedButton() == backgroundBtn) {
        hide();
        m_tray->showMessage(QStringLiteral("PS5 Streamer"),
                            QStringLiteral("Running in the background"),
                            QSystemTrayIcon::Information, 2000);
        event->ignore();
        return;
    }

    if (box.clickedButton() == quitBtn) {
        m_forceQuit = true;
        m_controller->stop();
        event->accept();
        qApp->quit();
        return;
    }

    event->ignore();
}

bool MainWindow::ensureElevated()
{
    if (Privilege::isElevated())
        return true;

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("Administrator required"),
        QStringLiteral(
            "DNS interception needs port 53.\n\n"
            "Enter your password to relaunch PS5 Streamer with administrator rights."),
        QMessageBox::Ok | QMessageBox::Cancel,
        QMessageBox::Ok);

    if (answer != QMessageBox::Ok)
        return false;

    QString error;
    if (Privilege::relaunchElevated(&error)) {
        m_forceQuit = true;
        qApp->quit();
        return false;
    }

    QMessageBox::warning(this, QStringLiteral("Elevation failed"),
                         error.isEmpty() ? QStringLiteral("Could not elevate privileges.")
                                         : error);
    return false;
}

void MainWindow::onToggle()
{
    if (m_controller->isRunning()) {
        m_controller->stop();
        refreshUI();
        return;
    }

    if (!ensureElevated()) {
        refreshUI();
        return;
    }

    m_toggleBtn->setEnabled(false);
    m_toggleBtn->setText(u8("…"));
    m_controller->start();
}

void MainWindow::onCopyUrl()
{
    const QString url = m_controller->obsURL();
    if (!url.isEmpty())
        QApplication::clipboard()->setText(url);
}

void MainWindow::onLogAdded(const QString &line)
{
    auto *item = new QListWidgetItem(line);
    item->setToolTip(line);
    m_logList->insertItem(0, item);
    while (m_logList->count() > 200)
        delete m_logList->takeItem(m_logList->count() - 1);
}

QColor MainWindow::statusColor(int status) const
{
    switch (static_cast<ServiceStatus>(status)) {
    case ServiceStatus::Stopped:
        return QColor(QStringLiteral("#8E8E93"));
    case ServiceStatus::Starting:
        return QColor(QStringLiteral("#FFD60A"));
    case ServiceStatus::Running:
        return QColor(QStringLiteral("#30D158"));
    case ServiceStatus::Error:
        return QColor(QStringLiteral("#FF453A"));
    }
    return QColor(Qt::gray);
}

void MainWindow::setDotColor(QLabel *dot, int status)
{
    const QColor c = statusColor(status);
    dot->setStyleSheet(QStringLiteral(
        "background: %1; border-radius: 4px; min-width: 8px; max-width: 8px;"
        " min-height: 8px; max-height: 8px;").arg(c.name()));
}

void MainWindow::refreshUI()
{
    const bool running = m_controller->isRunning();
    const bool starting = m_controller->rtmpStatus() == ServiceStatus::Starting
        || m_controller->dnsStatus() == ServiceStatus::Starting;

    m_toggleBtn->setEnabled(!starting);
    m_toggleBtn->setText(starting ? u8("…")
                                  : (running ? QStringLiteral("Stop") : QStringLiteral("Start")));
    m_toggleBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: %1; color: white; border: none; border-radius: 6px;"
        "  padding: 6px 14px; font-weight: 600;"
        "}"
        "QPushButton:disabled { opacity: 0.6; }"
        ).arg(running ? QStringLiteral("#FF453A") : QStringLiteral("#AF52DE")));

    m_ipValue->setText(m_controller->localIP());
    setDotColor(m_rtmpDot, int(m_controller->rtmpStatus()));
    setDotColor(m_dnsDot, int(m_controller->dnsStatus()));
    m_rtmpDot->setToolTip(m_controller->rtmpStatusLabel());
    m_dnsDot->setToolTip(m_controller->dnsStatusLabel());

    const QString url = m_controller->obsURL();
    if (!url.isEmpty()) {
        m_urlValue->setText(url);
        m_urlRow->show();
        m_urlHint->hide();
    } else {
        m_urlRow->hide();
        m_urlHint->setText(running ? u8("Waiting for PS5…")
                                   : QStringLiteral("Start to get URL"));
        m_urlHint->show();
    }

    if (m_tray)
        m_tray->setToolTip(running ? u8("PS5 Streamer — Running")
                                   : QStringLiteral("PS5 Streamer"));
}
