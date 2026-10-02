// Printing, verified end to end: documents are printed to PDF files, and the
// files are read back with the project's own PDF engine to count pages.

#include <QFile>
#include <QGuiApplication>
#include <QPalette>
#include <QPainter>
#include <QPrinter>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include "PdfDocumentLoader.h"
#include "app/appearance/ReaderAppearanceDefaults.h"
#include "app/print/DocumentPrinter.h"
#include "renderers/MarkdownHtmlRenderer.h"

using namespace mdbr::print;
using mdbr::layout::PageLayoutSettings;

namespace {

PrintableContent markdownContent(const QString &markdown, const QString &title = "test")
{
    PrintableContent content;
    content.kind = PrintableContent::Kind::Html;
    content.title = title;
    content.html = MarkdownHtmlRenderer::render(markdown).html;
    return content;
}

QString longMarkdown(int paragraphs = 150)
{
    QString md = "# Long document\n\n";
    for (int i = 0; i < paragraphs; ++i)
        md += QString("Paragraph %1 with enough words to wrap across the printed line width.\n\n").arg(i);
    return md;
}

// Pages in a PDF file, counted by the project's PDF engine.
int pdfPageCount(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return -1;
    const QByteArray data = file.readAll();
    const PdfDocumentResult loaded = PdfDocumentLoader().load(data);
    if (!loaded.ok()) {
        qWarning("PDF engine could not read %s (warning %d): %s", qPrintable(path),
                 int(loaded.warning), qPrintable(loaded.diagnosticMessage));
        return -1;
    }
    return loaded.document.pageTree.actualCount;
}

void printToPdfFile(QPrinter &printer, const QString &path, const PageLayoutSettings &settings)
{
    DocumentPrinter::configurePrinter(printer, settings);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
}

} // namespace

class DocumentPrinterTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void printerUsesPaperFromSettings();
    void printColoursAreLightEvenInNightTheme();
    void longDocumentHasSeveralPages();
    void printedPageIsInkOnWhitePaper();
    void darkSystemThemeStillPrintsDarkText();
    void htmlPrintsAllPagesToPdf();
    void pageRangeIsHonoured();
    void plainTextPrints();
    void pdfDocumentReprints();
    void nothingToPrintIsAnError();
    void emptyPageRangeIsAnError();

private:
    QTemporaryDir m_dir;
};

void DocumentPrinterTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
}

void DocumentPrinterTest::printerUsesPaperFromSettings()
{
    PageLayoutSettings settings;
    settings.paper = mdbr::layout::PaperSize::Letter;
    QPrinter printer(QPrinter::HighResolution);
    DocumentPrinter::configurePrinter(printer, settings);
    QCOMPARE(printer.pageLayout().pageSize().id(), QPageSize::Letter);
    QVERIFY(printer.fullPage());
}

void DocumentPrinterTest::printColoursAreLightEvenInNightTheme()
{
    // The reader may use the night theme; paper must still get dark text.
    const auto doc = DocumentPrinter::buildTextDocument(markdownContent("Plain words here."), {});
    const QTextCursor cursor = doc->find("Plain");
    QVERIFY(!cursor.isNull());
    const QColor text = cursor.charFormat().foreground().color();
    const QColor night(ReaderAppearanceDefaults::nightPalette().mainText);
    QVERIFY2(text != night, "night-theme text colour leaked into the printout");
    QVERIFY2(text.lightnessF() < 0.5, qPrintable("text is not dark: " + text.name()));
}

void DocumentPrinterTest::longDocumentHasSeveralPages()
{
    QCOMPARE(DocumentPrinter::pageCount(markdownContent("Short."), {}), 1);
    QVERIFY(DocumentPrinter::pageCount(markdownContent(longMarkdown()), {}) > 2);
}

void DocumentPrinterTest::printedPageIsInkOnWhitePaper()
{
    const QImage page = DocumentPrinter::renderPageImage(markdownContent(longMarkdown()), {}, 0, 96.0);
    QVERIFY(!page.isNull());
    QCOMPARE(qRound(page.width() / 96.0 * 25.4), 210); // A4 width in mm

    int white = 0, dark = 0;
    for (int y = 0; y < page.height(); y += 2)
        for (int x = 0; x < page.width(); x += 2) {
            const int lightness = page.pixelColor(x, y).lightness();
            white += lightness > 245;
            dark += lightness < 110;
        }
    const int samples = (page.width() / 2) * (page.height() / 2);
    QVERIFY2(white > samples * 0.7, "the page background is not white paper");
    QVERIFY2(dark > 200, "no dark text was printed");

    // The whole text area must carry text, not just the heading: count rows
    // with ink in the middle half of the page.
    int inkRows = 0;
    for (int y = page.height() / 4; y < page.height() * 3 / 4; ++y) {
        for (int x = 0; x < page.width(); x += 2) {
            if (page.pixelColor(x, y).lightness() < 110) {
                ++inkRows;
                break;
            }
        }
    }
    QVERIFY2(inkRows > page.height() / 8, qPrintable(QString("only %1 rows with text in the middle of page 1").arg(inkRows)));

    if (qEnvironmentVariableIsSet("MDBR_SAVE_PRINT_PAGE"))
        page.save(qEnvironmentVariable("MDBR_SAVE_PRINT_PAGE"));
}

void DocumentPrinterTest::darkSystemThemeStillPrintsDarkText()
{
    // Regression: under Windows dark mode the application palette has white
    // text, and paragraphs (which inherit their colour) printed white on white.
    const QPalette original = QGuiApplication::palette();
    QPalette dark = original;
    dark.setColor(QPalette::Text, Qt::white);
    dark.setColor(QPalette::WindowText, Qt::white);
    QGuiApplication::setPalette(dark);

    PrintableContent plain;
    plain.kind = PrintableContent::Kind::PlainText;
    plain.plainText = QString("Plain text line with no colour of its own.\n").repeated(40);
    const QImage page = DocumentPrinter::renderPageImage(plain, {}, 0, 96.0);
    QGuiApplication::setPalette(original);

    int inkRows = 0;
    for (int y = 0; y < page.height(); ++y)
        for (int x = 0; x < page.width(); x += 2)
            if (page.pixelColor(x, y).lightness() < 110) { ++inkRows; break; }
    QVERIFY2(inkRows > 100, qPrintable(QString("only %1 rows of dark text").arg(inkRows)));
}

void DocumentPrinterTest::htmlPrintsAllPagesToPdf()
{
    const PageLayoutSettings settings;
    const PrintableContent content = markdownContent(longMarkdown());
    const int expected = DocumentPrinter::pageCount(content, settings);

    const QString path = m_dir.filePath("all-pages.pdf");
    QPrinter printer(QPrinter::HighResolution);
    printToPdfFile(printer, path, settings);
    const PrintResult result = DocumentPrinter::print(content, printer, settings);

    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.pagesPrinted, expected);
    QCOMPARE(pdfPageCount(path), expected);
}

void DocumentPrinterTest::pageRangeIsHonoured()
{
    const PageLayoutSettings settings;
    const QString path = m_dir.filePath("range.pdf");
    QPrinter printer(QPrinter::HighResolution);
    printToPdfFile(printer, path, settings);
    printer.setPrintRange(QPrinter::PageRange);
    printer.setFromTo(2, 3);

    const PrintResult result = DocumentPrinter::print(markdownContent(longMarkdown()), printer, settings);
    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.pagesPrinted, 2);
    QCOMPARE(pdfPageCount(path), 2);
}

void DocumentPrinterTest::plainTextPrints()
{
    PrintableContent content;
    content.kind = PrintableContent::Kind::PlainText;
    content.plainText = QString("plain line\n").repeated(300);

    const QString path = m_dir.filePath("plain.pdf");
    QPrinter printer(QPrinter::HighResolution);
    printToPdfFile(printer, path, {});
    const PrintResult result = DocumentPrinter::print(content, printer, {});
    QVERIFY2(result.ok, qPrintable(result.error));
    QVERIFY(result.pagesPrinted > 1);
    QCOMPARE(pdfPageCount(path), result.pagesPrinted);
}

void DocumentPrinterTest::pdfDocumentReprints()
{
    // Make a 3+ page PDF, then print it again as a PDF tab would.
    const QString source = m_dir.filePath("source.pdf");
    {
        QPrinter printer(QPrinter::HighResolution);
        printToPdfFile(printer, source, {});
        QVERIFY(DocumentPrinter::print(markdownContent(longMarkdown()), printer, {}).ok);
    }
    const int sourcePages = pdfPageCount(source);
    QVERIFY(sourcePages > 2);

    PrintableContent content;
    content.kind = PrintableContent::Kind::Pdf;
    content.pdfPath = source;
    QCOMPARE(DocumentPrinter::pageCount(content, {}), sourcePages);

    const QString copy = m_dir.filePath("reprint.pdf");
    QPrinter printer(QPrinter::HighResolution);
    printToPdfFile(printer, copy, {});
    const PrintResult result = DocumentPrinter::print(content, printer, {});
    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.pagesPrinted, sourcePages);
    QCOMPARE(pdfPageCount(copy), sourcePages);
}

void DocumentPrinterTest::nothingToPrintIsAnError()
{
    QPrinter printer(QPrinter::HighResolution);
    printToPdfFile(printer, m_dir.filePath("none.pdf"), {});
    const PrintResult result = DocumentPrinter::print(PrintableContent{}, printer, {});
    QVERIFY(!result.ok);
    QVERIFY(!result.error.isEmpty());
}

void DocumentPrinterTest::emptyPageRangeIsAnError()
{
    QPrinter printer(QPrinter::HighResolution);
    printToPdfFile(printer, m_dir.filePath("empty-range.pdf"), {});
    printer.setPrintRange(QPrinter::PageRange);
    printer.setFromTo(50, 60); // a one-page document
    const PrintResult result = DocumentPrinter::print(markdownContent("Short."), printer, {});
    QVERIFY(!result.ok);
    QVERIFY(result.error.contains("range"));
}

QTEST_MAIN(DocumentPrinterTest)
#include "DocumentPrinterTest.moc"
