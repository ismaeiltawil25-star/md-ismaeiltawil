// Page view, end to end on the real widget: pagination, margins, gaps, and the
// actual pixels drawn in day and night themes.

#include <QAbstractTextDocumentLayout>
#include <QFile>
#include <QScrollBar>
#include <QTest>
#include <QTextBlock>
#include <QTextLayout>

#include "app/appearance/ReaderAppearanceDefaults.h"
#include "app/appearance/ReaderStylesheetBuilder.h"
#include "app/document/DocumentTabPage.h"
#include "app/layout/PageViewPalette.h"
#include "renderers/MarkdownHtmlRenderer.h"
#include "services/MarkdownPreprocessor.h"

using namespace mdbr::layout;

namespace {

QString longHtml(int paragraphs = 120)
{
    QString html;
    for (int i = 0; i < paragraphs; ++i)
        html += QString("<p>Paragraph %1. The quick brown fox jumps over the lazy dog, "
                        "again and again, to fill a printed page with text.</p>").arg(i);
    return html;
}

PageLayoutSettings pageView(PaperSize paper = PaperSize::A4)
{
    PageLayoutSettings s;
    s.pageViewEnabled = true;
    s.paper = paper;
    return s;
}

// Configures the widget the same way MainWindow does.
void applyTheme(DocumentTabPage &page, const PageLayoutSettings &layout, const QString &pageBackground)
{
    const auto palette = PageViewPalette::fromPageBackground(QColor(pageBackground));
    const QString widgetBackground = layout.pageViewEnabled ? palette.desk.name() : pageBackground;
    page.setStyleSheet(QString("QTextBrowser { background-color: %1; }").arg(widgetBackground));
    page.setPageView(layout, palette);
}

} // namespace

class PageViewRenderingTest : public QObject
{
    Q_OBJECT

private slots:
    void paginatesLongDocument();
    void textNeverEntersMarginsOrGaps();
    void paginationSurvivesNewContent();
    void continuousModeIsUnchanged();
    void turningPageViewOffRestoresContinuous();
    void nightThemePixels();
    void dayThemePixels();
    void sheetsAreDrawnAfterScrolling_data();
    void sheetsAreDrawnAfterScrolling();
    void realDocumentKeepsTextOutOfGaps_data();
    void realDocumentKeepsTextOutOfGaps();
};

void PageViewRenderingTest::paginatesLongDocument()
{
    DocumentTabPage page;
    page.resize(1000, 800);
    applyTheme(page, pageView(), "#FFFFFF");
    page.setHtml(longHtml());

    QVERIFY2(page.pageCount() > 2, qPrintable(QString("pages: %1").arg(page.pageCount())));
    QCOMPARE(page.document()->pageSize(), page.pageGeometry().pageSize);
}

void PageViewRenderingTest::textNeverEntersMarginsOrGaps()
{
    DocumentTabPage page;
    page.resize(1000, 800);
    applyTheme(page, pageView(), "#FFFFFF");
    page.setHtml(longHtml());

    const PageGeometry &g = page.pageGeometry();
    const qreal pageHeight = g.pageSize.height();
    auto *layout = page.document()->documentLayout();

    int checkedLines = 0;
    for (QTextBlock block = page.document()->begin(); block.isValid(); block = block.next()) {
        const QRectF blockRect = layout->blockBoundingRect(block);
        const QTextLayout *textLayout = block.layout();
        for (int i = 0; i < textLayout->lineCount(); ++i) {
            const QTextLine line = textLayout->lineAt(i);
            const qreal top = blockRect.top() + line.y();
            const qreal bottom = top + line.height();
            const int pageIndex = int(top / pageHeight);
            const qreal pageTop = pageIndex * pageHeight;
            QVERIFY2(top >= pageTop + g.margin - 1 && bottom <= pageTop + pageHeight - g.margin + 1,
                     qPrintable(QString("line at %1-%2 crosses margins of page %3").arg(top).arg(bottom).arg(pageIndex)));
            ++checkedLines;
        }
    }
    QVERIFY(checkedLines > 50);
}

void PageViewRenderingTest::paginationSurvivesNewContent()
{
    // MainWindow re-sets the HTML when the theme changes; pages must persist.
    DocumentTabPage page;
    page.resize(1000, 800);
    applyTheme(page, pageView(), "#FFFFFF");
    page.setHtml("<p>short</p>");
    page.setHtml(longHtml());
    QVERIFY(page.pageCount() > 2);
    QCOMPARE(page.document()->rootFrame()->frameFormat().topMargin(), page.pageGeometry().margin);

    page.setPlainText(QString("plain text line\n").repeated(400));
    QVERIFY(page.pageCount() > 2);
}

void PageViewRenderingTest::continuousModeIsUnchanged()
{
    DocumentTabPage page;
    page.resize(1000, 800);
    applyTheme(page, PageLayoutSettings{}, "#FFFFFF");
    page.setHtml(longHtml());

    QCOMPARE(page.pageCount(), 1);
    // No page height (0 before first show, -1 after) = one continuous column.
    QVERIFY(page.document()->pageSize().height() <= 0);
    QCOMPARE(page.lineWrapMode(), QTextEdit::WidgetWidth);
}

void PageViewRenderingTest::turningPageViewOffRestoresContinuous()
{
    DocumentTabPage page;
    page.resize(1200, 800);
    page.setHtml(longHtml());
    const qreal originalMargin = page.document()->rootFrame()->frameFormat().leftMargin();

    applyTheme(page, pageView(), "#FFFFFF");
    QVERIFY(page.pageCount() > 2);

    applyTheme(page, PageLayoutSettings{}, "#FFFFFF");
    QCOMPARE(page.lineWrapMode(), QTextEdit::WidgetWidth);
    QCOMPARE(page.document()->rootFrame()->frameFormat().leftMargin(), originalMargin);
    QCOMPARE(page.viewport()->x(), page.frameWidth()); // no centring margins left
}

void PageViewRenderingTest::nightThemePixels()
{
    const QString night = ReaderAppearanceDefaults::nightPalette().pageBackground;
    const auto palette = PageViewPalette::fromPageBackground(QColor(night));

    DocumentTabPage page;
    page.resize(1200, 900);
    applyTheme(page, pageView(), night);
    page.setHtml(longHtml());
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));

    const QImage image = page.grab().toImage();
    const QPoint origin = page.viewport()->geometry().topLeft();

    // Desk to the left of the page (outside the viewport, scroll-area background).
    QCOMPARE(image.pixelColor(origin.x() - 10, origin.y() + 200).name(), palette.desk.name());
    // Gap above the first sheet (inside the viewport).
    QCOMPARE(image.pixelColor(origin.x() + 300, origin.y() + 3).name(), palette.desk.name());
    // Inside the page's left margin: the unchanged night page colour.
    QCOMPARE(image.pixelColor(origin.x() + 30, origin.y() + 200).name(), QColor(night).name());

    QVERIFY(contrastRatio(palette.desk, palette.page) >= PageViewPalette::kMinDeskContrast);
}

void PageViewRenderingTest::dayThemePixels()
{
    const QString day = ReaderAppearanceDefaults::dayPalette().pageBackground;
    const auto palette = PageViewPalette::fromPageBackground(QColor(day));

    DocumentTabPage page;
    page.resize(1200, 900);
    applyTheme(page, pageView(), day);
    page.setHtml(longHtml());
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));

    const QImage image = page.grab().toImage();
    const QPoint origin = page.viewport()->geometry().topLeft();
    QCOMPARE(image.pixelColor(origin.x() - 10, origin.y() + 200).name(), palette.desk.name());
    QCOMPARE(image.pixelColor(origin.x() + 300, origin.y() + 3).name(), palette.desk.name());
    QCOMPARE(image.pixelColor(origin.x() + 30, origin.y() + 200).name(), QColor(day).name());
}

void PageViewRenderingTest::sheetsAreDrawnAfterScrolling_data()
{
    QTest::addColumn<int>("paper");
    QTest::newRow("A4") << int(PaperSize::A4);
    QTest::newRow("A5") << int(PaperSize::A5);
}

void PageViewRenderingTest::sheetsAreDrawnAfterScrolling()
{
    // Regression: sheets vanished in the middle of long documents because the
    // visible-page range was clamped to Qt's lazily-computed page count.
    QFETCH(int, paper);
    const QString night = ReaderAppearanceDefaults::nightPalette().pageBackground;

    DocumentTabPage page;
    page.resize(1000, 900);
    applyTheme(page, pageView(PaperSize(paper)), night);
    page.setHtml(longHtml(400));
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));

    page.verticalScrollBar()->setValue(page.verticalScrollBar()->maximum() / 2);
    const QImage image = page.grab().toImage();
    const QPoint origin = page.viewport()->geometry().topLeft();

    // Scan a column inside the left page margin: most of it must be page colour.
    int pagePixels = 0;
    const int height = page.viewport()->height();
    for (int y = 0; y < height; ++y)
        pagePixels += image.pixelColor(origin.x() + 20, origin.y() + y).name() == QColor(night).name();
    QVERIFY2(pagePixels > height * 3 / 4,
             qPrintable(QString("only %1 of %2 rows show a page sheet").arg(pagePixels).arg(height)));
}

void PageViewRenderingTest::realDocumentKeepsTextOutOfGaps_data()
{
    QTest::addColumn<int>("paper");
    QTest::addColumn<QString>("file");
    const QString fixture = QStringLiteral(TEST_DATA_DIR "/comprehensive-test.md");
    const QString visual = QStringLiteral(TEST_DATA_DIR "/../manual/comprehensive-visual-test.md");
    for (const auto &[name, path] : {std::pair{"fixture", fixture}, std::pair{"visual", visual}}) {
        QTest::newRow(qPrintable(QString("%1 A4").arg(name))) << int(PaperSize::A4) << path;
        QTest::newRow(qPrintable(QString("%1 A5").arg(name))) << int(PaperSize::A5) << path;
        QTest::newRow(qPrintable(QString("%1 Letter").arg(name))) << int(PaperSize::Letter) << path;
    }
}

void PageViewRenderingTest::realDocumentKeepsTextOutOfGaps()
{
    // The real pipeline and the real reader stylesheet (line-height, list and
    // table margins): every visible line must lie inside a page's text area.
    QFETCH(int, paper);
    QFETCH(QString, file);
    QFile source(file);
    QVERIFY(source.open(QIODevice::ReadOnly));
    const QString markdown = MarkdownPreprocessor::convertDefinitionLists(
        MarkdownPreprocessor::stripYamlFrontMatter(QString::fromUtf8(source.readAll())));
    const auto rendered = MarkdownHtmlRenderer::render(markdown);
    QVERIFY(rendered.isValid());

    const auto appearance = ReaderAppearanceDefaults::nightPalette();
    DocumentTabPage page;
    page.resize(1000, 900);
    page.document()->setDefaultStyleSheet(ReaderStylesheetBuilder::buildDocumentStylesheet(appearance));
    applyTheme(page, pageView(PaperSize(paper)), appearance.pageBackground);
    page.setHtml(rendered.html);
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));

    const PageGeometry &g = page.pageGeometry();
    const qreal pageHeight = g.pageSize.height();
    auto *layout = page.document()->documentLayout();
    QStringList offenders;
    for (QTextBlock block = page.document()->begin(); block.isValid(); block = block.next()) {
        const QRectF blockRect = layout->blockBoundingRect(block);
        const QTextLayout *textLayout = block.layout();
        for (int i = 0; i < textLayout->lineCount(); ++i) {
            const QTextLine line = textLayout->lineAt(i);
            const qreal top = blockRect.top() + line.y();
            const qreal bottom = top + line.height();
            const int pageIndex = int(top / pageHeight);
            const qreal pageTop = pageIndex * pageHeight;
            const bool inside = top >= pageTop + PageGeometry::kPageGapPx / 2
                             && bottom <= pageTop + pageHeight - PageGeometry::kPageGapPx / 2;
            if (!inside)
                offenders << QString("page %1 y=%2..%3 \"%4\"").arg(pageIndex).arg(top).arg(bottom)
                                 .arg(block.text().mid(line.textStart(), 30));
        }
    }
    QVERIFY2(offenders.isEmpty(), qPrintable(offenders.join('\n')));
}

QTEST_MAIN(PageViewRenderingTest)
#include "PageViewRenderingTest.moc"
