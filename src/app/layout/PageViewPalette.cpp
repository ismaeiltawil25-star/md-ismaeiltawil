#include "PageViewPalette.h"

#include <QtMath>

namespace mdbr::layout {

namespace {

[[nodiscard]] qreal linearChannel(qreal c)
{
    return c <= 0.04045 ? c / 12.92 : qPow((c + 0.055) / 1.055, 2.4);
}

[[nodiscard]] qreal relativeLuminance(const QColor &color)
{
    return 0.2126 * linearChannel(color.redF())
         + 0.7152 * linearChannel(color.greenF())
         + 0.0722 * linearChannel(color.blueF());
}

[[nodiscard]] QColor mix(const QColor &from, const QColor &to, qreal amount)
{
    const auto lerp = [amount](qreal a, qreal b) { return a + (b - a) * amount; };
    return QColor::fromRgbF(float(lerp(from.redF(), to.redF())),
                            float(lerp(from.greenF(), to.greenF())),
                            float(lerp(from.blueF(), to.blueF())));
}

} // namespace

qreal contrastRatio(const QColor &a, const QColor &b)
{
    const qreal la = relativeLuminance(a);
    const qreal lb = relativeLuminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}

PageViewPalette PageViewPalette::fromPageBackground(const QColor &pageBackground)
{
    const QColor page = pageBackground.isValid() ? pageBackground.toRgb() : QColor(Qt::white);
    const bool darkPage = relativeLuminance(page) < 0.18;
    const QColor towards = darkPage ? QColor(Qt::white) : QColor(Qt::black);

    // Start subtle and strengthen until the desk is clearly distinct.
    qreal amount = darkPage ? 0.08 : 0.10;
    QColor desk = mix(page, towards, amount);
    while (contrastRatio(page, desk) < kMinDeskContrast && amount < 0.6) {
        amount += 0.02;
        desk = mix(page, towards, amount);
    }

    PageViewPalette palette;
    palette.page = page;
    palette.desk = desk;
    palette.border = mix(page, towards, qMin(amount * 2.0, 0.7));
    palette.shadow = darkPage ? QColor(0, 0, 0, 140) : QColor(0, 0, 0, 45);
    return palette;
}

} // namespace mdbr::layout
