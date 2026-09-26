#include "trayplacement.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

std::optional<TrayPlacement> trayPlacement(const QByteArray &layout, QSize warningSize, QPoint preferredScreenPoint)
{
    const auto rectangle = [](const QJsonObject &value) {
        return QRect(value.value("x").toInt(), value.value("y").toInt(),
                     value.value("width").toInt(), value.value("height").toInt());
    };
    std::optional<TrayPlacement> first;
    for (const auto &entry : QJsonDocument::fromJson(layout).array()) {
        const auto panel = entry.toObject();
        const auto screen = rectangle(panel.value("screenGeometry").toObject());
        const auto tray = rectangle(panel.value("geometry").toObject());
        const auto edge = panel.value("location").toString();
        const bool horizontal = edge == "top" || edge == "bottom";
        const bool vertical = edge == "left" || edge == "right";
        const int length = panel.value("length").toInt();
        const int thickness = panel.value("height").toInt();
        if (!screen.isValid() || !tray.isValid() || (!horizontal && !vertical)
            || length <= 0 || thickness <= 0) {
            continue;
        }
        const int extent = horizontal ? screen.width() : screen.height();
        const int offset = panel.value("offset").toInt();
        const auto alignment = panel.value("alignment").toString();
        int start = offset;
        if (alignment == "center") {
            start = (extent - length) / 2 + offset;
        } else if (alignment == "right") {
            start = extent - length - offset;
        }
        constexpr int gap = 12;
        const int inset = thickness + gap + (panel.value("floating").toBool() ? gap : 0);
        int x;
        int y;
        if (horizontal) {
            x = start + tray.x() + tray.width() / 2 - warningSize.width() / 2;
            y = edge == "top" ? inset : screen.height() - inset - warningSize.height();
        } else {
            x = edge == "left" ? inset : screen.width() - inset - warningSize.width();
            y = start + tray.y() + tray.height() / 2 - warningSize.height() / 2;
        }
        x = std::clamp(x, gap, std::max(gap, screen.width() - warningSize.width() - gap));
        y = std::clamp(y, gap, std::max(gap, screen.height() - warningSize.height() - gap));
        const TrayPlacement placement{screen, QPoint(x, y)};
        if (screen.contains(preferredScreenPoint)) {
            return placement;
        }
        if (!first) {
            first = placement;
        }
    }
    return first;
}
