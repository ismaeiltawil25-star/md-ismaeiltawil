#include "PrintIcons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

#include <functional>

namespace mdbr::print {

namespace {

// Icons are designed on a 24×24 grid and rendered at every common size.
using Drawing = std::function<void(QPainter &, const QColor &)>;

QIcon renderIcon(const QColor &color, const Drawing &draw)
{
    QIcon icon;
    for (const int size : {16, 20, 24, 32, 48, 64}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(size / 24.0, size / 24.0);
        draw(painter, color);
        icon.addPixmap(pixmap);
    }
    return icon;
}

void drawPrinter(QPainter &p, const QColor &color)
{
    QPen pen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    p.drawRect(QRectF(7, 3, 10, 5));                 // paper going in
    p.drawRoundedRect(QRectF(3, 8, 18, 9), 2, 2);    // printer body
    p.setBrush(Qt::white);
    p.drawRect(QRectF(7, 13, 10, 8));                // printed page coming out
    p.setPen(QPen(color, 1.2));
    p.drawLine(QPointF(9, 16), QPointF(15, 16));     // text lines on the page
    p.drawLine(QPointF(9, 18.5), QPointF(13.5, 18.5));
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawEllipse(QPointF(17.5, 11), 1, 1);          // status light
}

void drawPreview(QPainter &p, const QColor &color)
{
    QPen pen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    QPainterPath sheet;                              // page with a folded corner
    sheet.moveTo(5, 2.5);
    sheet.lineTo(14, 2.5);
    sheet.lineTo(18, 6.5);
    sheet.lineTo(18, 11);
    sheet.moveTo(10.5, 21.5);
    sheet.lineTo(5, 21.5);
    sheet.lineTo(5, 2.5);
    p.drawPath(sheet);
    p.drawLine(QPointF(8, 8), QPointF(13, 8));
    p.drawLine(QPointF(8, 11), QPointF(11, 11));

    p.drawEllipse(QPointF(15, 16), 3.6, 3.6);        // magnifying glass
    p.drawLine(QPointF(17.6, 18.6), QPointF(21, 22));
}

} // namespace

QIcon printIcon(const QColor &color)
{
    return renderIcon(color, drawPrinter);
}

QIcon printPreviewIcon(const QColor &color)
{
    return renderIcon(color, drawPreview);
}

} // namespace mdbr::print
