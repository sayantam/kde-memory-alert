#pragma once

#include "alertwindow.h"
#include "monitor.h"

#include <KStatusNotifierItem>
#include <QObject>

class DesktopUi : public QObject {
    Q_OBJECT
public:
    explicit DesktopUi(QObject *parent = nullptr);
    void update(const MonitorState &state);
    void requestQuit();
    const AlertWindow &window() const { return m_window; }
    const KStatusNotifierItem &tray() const { return m_tray; }

Q_SIGNALS:
    void quitRequested();

private:
    AlertWindow m_window;
    KStatusNotifierItem m_tray;
};
