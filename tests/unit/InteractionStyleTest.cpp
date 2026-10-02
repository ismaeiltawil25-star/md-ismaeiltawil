// Hover / checked / selected states must stay clearly visible and readable in
// light and dark system themes (regression: a checked toolbar button was pale
// blue with white text in Windows dark mode).

#include <QTest>

#include "app/layout/PageViewPalette.h"
#include "app/settings/InteractionStyle.h"

using mdbr::layout::contrastRatio;
using namespace mdbr::settings;

namespace {

QPalette makePalette(const QColor &window, const QColor &text, const QColor &accent, const QColor &accentText)
{
    QPalette p;
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, accentText);
    return p;
}

} // namespace

class InteractionStyleTest : public QObject
{
    Q_OBJECT

private slots:
    void statesAreVisibleAndReadable_data();
    void statesAreVisibleAndReadable();
    void styleSheetCoversTheStates();
};

void InteractionStyleTest::statesAreVisibleAndReadable_data()
{
    QTest::addColumn<QPalette>("palette");
    // Windows 11 dark: light-blue accent with white highlighted text (the bad case).
    QTest::newRow("windows dark, light accent")
        << makePalette(QColor("#202020"), Qt::white, QColor("#99ebff"), Qt::white);
    QTest::newRow("windows dark, blue accent")
        << makePalette(QColor("#202020"), Qt::white, QColor("#0078d4"), Qt::white);
    QTest::newRow("windows light")
        << makePalette(QColor("#f3f3f3"), Qt::black, QColor("#0078d4"), Qt::white);
    QTest::newRow("light, pale accent")
        << makePalette(Qt::white, QColor("#1a1a1a"), QColor("#cce8ff"), Qt::white);
}

void InteractionStyleTest::statesAreVisibleAndReadable()
{
    QFETCH(QPalette, palette);
    const InteractionColors c = interactionColors(palette);

    QVERIFY2(contrastRatio(c.hover, c.text) >= 4.5, "text unreadable on hover");
    QVERIFY2(contrastRatio(c.hover, c.window) >= 1.4, "hover not visible");
    QVERIFY2(contrastRatio(c.checked, c.text) >= 4.5, "text unreadable on a checked button");
    QVERIFY2(contrastRatio(c.checked, c.window) >= 1.4, "checked state not visible");
    QVERIFY2(contrastRatio(c.selection, c.selectionText) >= 4.5, "selected menu item unreadable");
}

void InteractionStyleTest::styleSheetCoversTheStates()
{
    const QString sheet = interactionStyleSheet(makePalette(QColor("#202020"), Qt::white, QColor("#0078d4"), Qt::white));
    for (const char *selector : {"QMenu::item:selected", "QToolButton:hover", "QToolButton:checked",
                                 "QListWidget::item:selected"})
        QVERIFY2(sheet.contains(QLatin1String(selector)), selector);
}

QTEST_APPLESS_MAIN(InteractionStyleTest)
#include "InteractionStyleTest.moc"
