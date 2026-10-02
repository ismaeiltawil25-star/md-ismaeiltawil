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

namespace mdbr::settings {

QIcon themePreviewIcon(const ReaderAppearanceSettings &palette, const ReaderAppearanceSettings *secondHalf)
{
    const QSize size(72, 48);
    QIcon icon;
    for (const qreal scale : {1.0, 2.0}) {
        QPixmap pixmap(size * scale);
        pixmap.setDevicePixelRatio(scale);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);

        const QRectF page(1, 1, size.width() - 2, size.height() - 2);
        const auto drawHalf = [&](const ReaderAppearanceSettings &pal, const QRectF &clip) {
            p.save();
            p.setClipRect(clip);
            p.setPen(QPen(QColor(pal.horizontalRule), 1));
            p.setBrush(QColor(pal.pageBackground));
            p.drawRoundedRect(page, 6, 6);
            QFont font = p.font();
            font.setPixelSize(17);
            font.setBold(true);
            p.setFont(font);
            p.setPen(QColor(pal.mainText));
            p.drawText(QRectF(page.left() + 8, page.top() + 4, page.width() - 16, 24),
                       Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Aa"));
            p.setPen(QPen(QColor(pal.mutedText), 2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(page.left() + 9, 33), QPointF(page.right() - 9, 33));
            p.setPen(QPen(QColor(pal.linkText), 2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(page.left() + 9, 39), QPointF(page.center().x() + 4, 39));
            p.restore();
        };

        if (secondHalf) {
            drawHalf(palette, QRectF(0, 0, size.width() / 2.0, size.height()));
            drawHalf(*secondHalf, QRectF(size.width() / 2.0, 0, size.width() / 2.0, size.height()));
        } else {
            drawHalf(palette, QRectF(QPointF(0, 0), QSizeF(size)));
        }
        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace mdbr::settings
