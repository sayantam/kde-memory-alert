#pragma once

#include <QByteArray>
#include <QRect>
#include <optional>

struct TrayPlacement {
    QRect screen;
    QPoint position; // Screen-local top-left of the warning.
};

// Plasma reports the widget rectangle relative to its panel, not to the desktop.
std::optional<TrayPlacement> trayPlacement(const QByteArray &layout, QSize warningSize, QPoint preferredScreenPoint);
