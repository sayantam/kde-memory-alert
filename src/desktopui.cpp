#include "desktopui.h"

#include <KLocalizedString>
#include <QAction>
#include <QIcon>
#include <QLocale>
#include <QMenu>

DesktopUi::DesktopUi(QObject *parent)
    : QObject(parent), m_tray(QStringLiteral("kde-memory-alert"), this)
{
    m_tray.setTitle(i18n("KDE Memory Alert"));
    m_tray.setCategory(KStatusNotifierItem::Hardware);
    m_tray.setIconByName(QStringLiteral("hwinfo"));
    m_tray.setStatus(KStatusNotifierItem::Passive);
    m_tray.setStandardActionsEnabled(false);
    m_tray.setIsMenu(true);
    auto *menu = new QMenu;
    auto *quit = menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Quit"));
    connect(quit, &QAction::triggered, this, &DesktopUi::requestQuit);
    m_tray.setContextMenu(menu); // KStatusNotifierItem owns the menu.
    // No associated window: tray activation must never toggle the alert.
}

void DesktopUi::requestQuit()
{
    // QApplication asks windows to close when quitting. Release the persistent
    // warning first so it cannot veto the user's explicit Quit action.
    m_window.present(false, {});
    Q_EMIT quitRequested();
}

void DesktopUi::update(const MonitorState &state)
{
    QString text;
    if (!state.usagePercent) {
        text = i18n("Memory usage unavailable — retrying.");
    } else {
        const auto usage = QLocale().toString(*state.usagePercent, 'f', 1);
        const auto threshold = QLocale().toString(state.thresholdPercent, 'g', 12);
        text = state.alerting
            ? i18n("Warning! Memory usage at %1% — exceeds %2% threshold", usage, threshold)
            : i18n("Memory usage at %1% — threshold %2%", usage, threshold);
        if (state.stale) {
            text += QLatin1Char('\n') + i18n("Reading stale — unable to read memory usage; retrying.");
        }
    }
    const auto icon = state.alerting ? QStringLiteral("dialog-warning") : QStringLiteral("hwinfo");
    m_tray.setIconByName(icon);
    m_tray.setToolTip(icon, i18n("KDE Memory Alert"), text);
    m_tray.setStatus(state.alerting ? KStatusNotifierItem::Active : KStatusNotifierItem::Passive);
    m_window.present(state.alerting, text);
}
