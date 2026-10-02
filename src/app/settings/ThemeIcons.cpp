#include "ThemeIcons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace mdbr::settings {

QIcon nightModeIcon(const QColor &color)
{
    QIcon icon;
    for (const int size : {16, 20, 24, 32, 48, 64}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(size / 24.0, size / 24.0);

        // Crescent: a full disc minus an offset disc.
        QPainterPath moon;
        moon.addEllipse(QPointF(11, 12.5), 8, 8);
        QPainterPath bite;
        bite.addEllipse(QPointF(15.5, 9), 6.5, 6.5);
        painter.setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(moon.subtracted(bite));

        // A small star.
        painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(19.5, 14.5), QPointF(19.5, 18.5));
        painter.drawLine(QPointF(17.5, 16.5), QPointF(21.5, 16.5));
        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace mdbr::settings
