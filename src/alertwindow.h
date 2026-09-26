#pragma once

#include <QWidget>

class QLabel;
namespace LayerShellQt { class Window; }

class AlertWindow : public QWidget {
public:
    AlertWindow();
    void present(bool alerting, const QString &message);
    QString message() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void showNearTray();
    QLabel *m_message;
    LayerShellQt::Window *m_surface = nullptr;
    bool m_alerting = false;
    bool m_positionPending = false;
};
