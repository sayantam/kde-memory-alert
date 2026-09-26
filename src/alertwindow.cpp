#include "alertwindow.h"
#include "trayplacement.h"

#include <KLocalizedString>
#include <LayerShellQt/Window>
#include <QCloseEvent>
#include <QCursor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QScreen>
#include <QTimer>

AlertWindow::AlertWindow()
    : QWidget(nullptr, Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint
                           | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
    , m_message(new QLabel(this))
{
    setWindowTitle(i18n("KDE Memory Alert"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    auto *icon = new QLabel(this);
    icon->setPixmap(QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(48, 48));
    m_message->setTextFormat(Qt::PlainText);
    m_message->setWordWrap(true);
    m_message->setFixedWidth(400);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    layout->addWidget(icon);
    layout->addWidget(m_message);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
        // Obtain a QWindow before showing it; layer-shell must be selected before mapping.
        winId();
        auto *surface = LayerShellQt::Window::get(windowHandle());
        m_surface = surface;
        surface->setScope(QStringLiteral("kde-memory-alert"));
        surface->setLayer(LayerShellQt::Window::LayerTop);
        surface->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        surface->setActivateOnShow(false);
        surface->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)
                            | LayerShellQt::Window::AnchorRight);
        surface->setMargins(QMargins(16, 16, 16, 16));
        surface->setExclusiveZone(0); // Respect panels without reserving desktop space.
        surface->setWantsToBeOnActiveScreen(true);
    }
}

void AlertWindow::present(bool alerting, const QString &message)
{
    m_alerting = alerting;
    m_message->setText(message);
    if (alerting) {
        if (!isVisible() && !m_positionPending) {
            showNearTray();
        }
    } else {
        hide();
    }
}

void AlertWindow::showNearTray()
{
    if (!m_surface) {
        show();
        return;
    }
    // Read the current layout once per alert, without changing Plasma settings.
    const auto script = QStringLiteral(R"JS(
var result = [];
panels().forEach(function(panel) {
    panel.widgets().forEach(function(widget) {
        if (widget.type === "org.kde.plasma.systemtray") {
            result.push({screenGeometry: screenGeometry(panel.screen), location: panel.location,
                height: panel.height, length: panel.length, alignment: panel.alignment,
                offset: panel.offset, floating: panel.floating, geometry: widget.geometry});
        }
    });
});
print(JSON.stringify(result));
)JS");
    auto request = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
        QStringLiteral("/PlasmaShell"), QStringLiteral("org.kde.PlasmaShell"), QStringLiteral("evaluateScript"));
    request << script;
    m_positionPending = true;
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(request, 1000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        const QDBusPendingReply<QString> reply = *watcher;
        watcher->deleteLater();
        m_positionPending = false;
        if (!m_alerting) {
            return; // A recovery or explicit Quit may have happened during the query.
        }
        layout()->activate();
        const auto placement = trayPlacement(reply.isError() ? QByteArray() : reply.value().toUtf8(),
                                             sizeHint(), QCursor::pos());
        QScreen *target = nullptr;
        if (placement) {
            for (auto *candidate : QGuiApplication::screens()) {
                if (candidate->geometry() == placement->screen) {
                    target = candidate;
                    break;
                }
            }
        }
        if (target) {
            m_surface->setScreen(target);
            m_surface->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)
                                  | LayerShellQt::Window::AnchorLeft);
            // Coordinates already include the panel's thickness; avoid adding its
            // exclusive zone a second time. The alert itself reserves no space.
            m_surface->setExclusiveZone(-1);
            m_surface->setMargins(QMargins(placement->position.x(), placement->position.y(), 0, 0));
        } else {
            // A missing/restarting or restricted shell must not suppress an alert.
            m_surface->setWantsToBeOnActiveScreen(true);
            m_surface->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)
                                  | LayerShellQt::Window::AnchorRight);
            m_surface->setExclusiveZone(0);
            m_surface->setMargins(QMargins(16, 16, 16, 16));
        }
        show();
    });
}

QString AlertWindow::message() const
{
    return m_message->text();
}

void AlertWindow::closeEvent(QCloseEvent *event)
{
    if (m_alerting) {
        event->ignore();
    } else {
        QWidget::closeEvent(event);
    }
}

void AlertWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && m_alerting && isMinimized()) {
        QTimer::singleShot(0, this, [this] {
            if (m_alerting) {
                showNormal();
            }
        });
    }
}
