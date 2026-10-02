#include "InteractionStyle.h"

#include "app/layout/PageViewPalette.h"

namespace mdbr::settings {

namespace {

using layout::contrastRatio;

constexpr qreal kMinTextContrast = 4.5;
constexpr qreal kMinStateContrast = 1.4;

[[nodiscard]] QColor mix(const QColor &from, const QColor &to, qreal amount)
{
    return QColor::fromRgbF(float(from.redF() + (to.redF() - from.redF()) * amount),
                            float(from.greenF() + (to.greenF() - from.greenF()) * amount),
                            float(from.blueF() + (to.blueF() - from.blueF()) * amount));
}

[[nodiscard]] bool isGoodState(const QColor &candidate, const QColor &window, const QColor &text)
{
    return contrastRatio(candidate, window) >= kMinStateContrast
        && contrastRatio(candidate, text) >= kMinTextContrast;
}

// Moves `window` towards `towards` until the result differs enough from the
// window while text stays readable on it. If `towards` is too close to the
// window (e.g. a pale accent on a white window), the text colour is used as the
// direction instead, which always works.
[[nodiscard]] QColor visibleState(const QColor &window, const QColor &text, const QColor &towards,
                                  qreal start)
{
    for (const QColor &direction : {towards, text}) {
        for (qreal amount = start; amount <= 0.6; amount += 0.03) {
            const QColor candidate = mix(window, direction, amount);
            if (isGoodState(candidate, window, text))
                return candidate;
        }
    }
    return mix(window, text, start);
}

[[nodiscard]] QColor readableOn(const QColor &background, const QColor &preferred)
{
    if (contrastRatio(background, preferred) >= kMinTextContrast)
        return preferred;
    const QColor black(Qt::black), white(Qt::white);
    return contrastRatio(background, black) >= contrastRatio(background, white) ? black : white;
}

} // namespace

InteractionColors interactionColors(const QPalette &palette)
{
    InteractionColors c;
    c.window = palette.color(QPalette::Window);
    c.text = palette.color(QPalette::WindowText);
    const QColor accent = palette.color(QPalette::Highlight);

    c.hover = visibleState(c.window, c.text, c.text, 0.10);
    // A checked button keeps the normal text colour on an accent tint, so it is
    // obviously "on" without turning into a bright block with unreadable text.
    c.checked = visibleState(c.window, c.text, accent, 0.25);
    c.checkedBorder = accent;

    c.selection = accent;
    c.selectionText = readableOn(accent, palette.color(QPalette::HighlightedText));
    return c;
}

QString interactionStyleSheet(const QPalette &palette)
{
    const InteractionColors c = interactionColors(palette);
    return QStringLiteral(
               "QMenu::item:selected { background-color: %3; color: %4; }"
               "QToolBar QToolButton { padding: 3px 6px; border: 1px solid transparent; border-radius: 4px; }"
               "QToolBar QToolButton:hover { background-color: %1; }"
               "QToolBar QToolButton:checked { background-color: %2; border-color: %5; color: %6; }"
               "QToolBar QToolButton:checked:hover { background-color: %2; }"
               "QListWidget::item { padding: 6px 8px; }"
               "QListWidget::item:hover { background-color: %1; color: %6; }"
               "QListWidget::item:selected { background-color: %3; color: %4; }")
        .arg(c.hover.name(), c.checked.name(), c.selection.name(), c.selectionText.name(),
             c.checkedBorder.name(), c.text.name());
}

} // namespace mdbr::settings
