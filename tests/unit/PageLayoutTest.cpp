// Unit tests for page view settings, geometry, palette and persistence.

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTest>

#include "app/appearance/ReaderAppearanceDefaults.h"
#include "app/layout/PageGeometry.h"
#include "app/layout/PageLayoutRepository.h"
#include "app/layout/PageViewPalette.h"

using namespace mdbr::layout;

class PageLayoutTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void a4AtStandardDpiMatchesPrintSize();
    void marginsConvertToPixels();
    void gapFitsInsideSmallestMargin();
    void idsRoundTrip();
    void unknownIdsFallBack();

    void paletteKeepsPageColour();
    void darkPageGetsLighterDesk();
    void lightPageGetsDarkerDesk();
    void deskIsDistinctForEveryGrey();
    void deskIsDistinctForBuiltInThemes_data();
    void deskIsDistinctForBuiltInThemes();

    void repositoryRoundTrip();
    void repositoryDefaultsToContinuous();
};

void PageLayoutTest::initTestCase()
{
    // Never touch the user's real settings.
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("md-backpack-reader-tests");
    QCoreApplication::setApplicationName("PageLayoutTest");
    PageLayoutRepository::clear();
}

void PageLayoutTest::a4AtStandardDpiMatchesPrintSize()
{
    PageLayoutSettings s;
    s.paper = PaperSize::A4;
    const PageGeometry g = computePageGeometry(s, 96.0);
    QCOMPARE(qRound(g.pageSize.width()), 794);   // 210 mm
    QCOMPARE(qRound(g.pageSize.height()), 1123); // 297 mm
}

void PageLayoutTest::marginsConvertToPixels()
{
    PageLayoutSettings s;
    s.margins = PageMargins::Normal;
    QCOMPARE(qRound(computePageGeometry(s, 96.0).margin), 96); // 2.54 cm = 1 inch
    s.margins = PageMargins::Narrow;
    QCOMPARE(qRound(computePageGeometry(s, 96.0).margin), 48);
}

void PageLayoutTest::gapFitsInsideSmallestMargin()
{
    // Half a gap is taken from each adjacent page's margin; text must never reach it.
    PageLayoutSettings s;
    s.margins = PageMargins::Narrow;
    const PageGeometry g = computePageGeometry(s, 72.0); // lowest common DPI
    QVERIFY(PageGeometry::kPageGapPx / 2 < g.margin);
}

void PageLayoutTest::idsRoundTrip()
{
    for (const PaperSize p : {PaperSize::A4, PaperSize::A5, PaperSize::Letter, PaperSize::Legal})
        QCOMPARE(paperFromId(paperId(p)), p);
    for (const PageMargins m : {PageMargins::Narrow, PageMargins::Moderate, PageMargins::Normal, PageMargins::Wide})
        QCOMPARE(marginsFromId(marginsId(m)), m);
}

void PageLayoutTest::unknownIdsFallBack()
{
    QCOMPARE(paperFromId("B7-unknown"), PaperSize::A4);
    QCOMPARE(marginsFromId(""), PageMargins::Normal);
    QCOMPARE(paperFromId("letter"), PaperSize::Letter); // case-insensitive
}

void PageLayoutTest::paletteKeepsPageColour()
{
    const QColor night("#0F141A");
    QCOMPARE(PageViewPalette::fromPageBackground(night).page, night);
}

void PageLayoutTest::darkPageGetsLighterDesk()
{
    const auto p = PageViewPalette::fromPageBackground(QColor("#0F141A"));
    QVERIFY(p.desk.lightnessF() > p.page.lightnessF());
}

void PageLayoutTest::lightPageGetsDarkerDesk()
{
    const auto p = PageViewPalette::fromPageBackground(QColor(Qt::white));
    QVERIFY(p.desk.lightnessF() < p.page.lightnessF());
}

void PageLayoutTest::deskIsDistinctForEveryGrey()
{
    for (int v = 0; v <= 255; ++v) {
        const QColor page(v, v, v);
        const auto p = PageViewPalette::fromPageBackground(page);
        QVERIFY2(contrastRatio(p.page, p.desk) >= PageViewPalette::kMinDeskContrast,
                 qPrintable(QString("grey %1: contrast %2").arg(v).arg(contrastRatio(p.page, p.desk))));
        QVERIFY2(p.border != p.page, qPrintable(QString("grey %1: invisible border").arg(v)));
    }
}

void PageLayoutTest::deskIsDistinctForBuiltInThemes_data()
{
    QTest::addColumn<QString>("pageBackground");
    QTest::newRow("calm")  << ReaderAppearanceDefaults::calmPalette().pageBackground;
    QTest::newRow("day")   << ReaderAppearanceDefaults::dayPalette().pageBackground;
    QTest::newRow("night") << ReaderAppearanceDefaults::nightPalette().pageBackground;
}

void PageLayoutTest::deskIsDistinctForBuiltInThemes()
{
    QFETCH(QString, pageBackground);
    const auto p = PageViewPalette::fromPageBackground(QColor(pageBackground));
    QVERIFY(contrastRatio(p.page, p.desk) >= PageViewPalette::kMinDeskContrast);
}

void PageLayoutTest::repositoryRoundTrip()
{
    PageLayoutSettings s;
    s.pageViewEnabled = true;
    s.paper = PaperSize::Letter;
    s.margins = PageMargins::Wide;
    PageLayoutRepository::save(s);
    QCOMPARE(PageLayoutRepository::load(), s);
    PageLayoutRepository::clear();
}

void PageLayoutTest::repositoryDefaultsToContinuous()
{
    PageLayoutRepository::clear();
    const PageLayoutSettings loaded = PageLayoutRepository::load();
    QVERIFY(!loaded.pageViewEnabled);
    QCOMPARE(loaded.paper, PaperSize::A4);
}

QTEST_GUILESS_MAIN(PageLayoutTest)
#include "PageLayoutTest.moc"
