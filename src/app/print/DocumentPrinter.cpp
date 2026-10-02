#include "DocumentPrinter.h"

#include "app/appearance/ReaderAppearanceDefaults.h"
#include "app/appearance/ReaderStylesheetBuilder.h"
#include "app/layout/PageGeometry.h"
#include "PdfDocumentLoader.h"
#include "PdfPageImageRenderer.h"
#include "PdfStreamDecoder.h"

#include <QAbstractTextDocumentLayout>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPageLayout>
#include <QPainter>
#include <QPrinter>
#include <QScreen>
#include <QTextDocument>
#include <QTextFrame>

#include <optional>
#include <utility>

namespace mdbr::print {

namespace {

using layout::PageLayoutSettings;

constexpr double kPdfRenderDpi = 200.0; // sharp on paper, bounded memory per page
constexpr int kFooterPixelSize = 11;    // in layout pixels (scaled with the page)

[[nodiscard]] QPageSize::PageSizeId pageSizeId(layout::PaperSize paper)
{
    switch (paper) {
    case layout::PaperSize::A4:     return QPageSize::A4;
    case layout::PaperSize::A5:     return QPageSize::A5;
    case layout::PaperSize::Letter: return QPageSize::Letter;
    case layout::PaperSize::Legal:  return QPageSize::Legal;
    }
    return QPageSize::A4;
}

// Paper size the printer will actually use (the user may change paper or
// orientation in the print dialog), in millimetres.
[[nodiscard]] QSizeF printerPaperMm(const QPrinter &printer)
{
    const QPageLayout pageLayout = printer.pageLayout();
    QSizeF mm = pageLayout.pageSize().size(QPageSize::Millimeter);
    if (pageLayout.orientation() == QPageLayout::Landscape)
        mm.transpose();
    return mm;
}

// First and last 0-based page to print, honouring the dialog's page range.
[[nodiscard]] std::pair<int, int> pageRange(const QPrinter &printer, int total)
{
    int first = 0;
    int last = total - 1;
    if (printer.printRange() == QPrinter::PageRange && printer.fromPage() > 0) {
        first = printer.fromPage() - 1;
        last = printer.toPage() > 0 ? qMin(printer.toPage() - 1, total - 1) : last;
    }
    return {first, last};
}

[[nodiscard]] std::unique_ptr<QTextDocument> buildDocument(const PrintableContent &content,
                                                           QSizeF paperMm, qreal marginMm)
{
    const qreal dpi = DocumentPrinter::layoutDpi();
    auto doc = std::make_unique<QTextDocument>();

    // Paper is white: always print with the light palette, never the night theme.
    doc->setDefaultStyleSheet(
        ReaderStylesheetBuilder::buildDocumentStylesheet(ReaderAppearanceDefaults::calmPalette()));
    doc->setBaseUrl(content.baseUrl);
    if (content.kind == PrintableContent::Kind::Html)
        doc->setHtml(content.html);
    else
        doc->setPlainText(content.plainText);

    const qreal margin = layout::mmToPx(marginMm, dpi);
    QTextFrameFormat frame = doc->rootFrame()->frameFormat();
    frame.setLeftMargin(margin);
    frame.setRightMargin(margin);
    frame.setTopMargin(margin);
    frame.setBottomMargin(margin);
    frame.setPadding(0);
    frame.clearBackground();
    doc->rootFrame()->setFrameFormat(frame);

    doc->setPageSize(QSizeF(layout::mmToPx(paperMm.width(), dpi), layout::mmToPx(paperMm.height(), dpi)));
    return doc;
}

// Paints page `index` of a print document at `scale` device pixels per layout
// pixel: content clipped to the page, plus the "page / total" footer.
// Shared by real printing and renderPageImage(), so both always match.
void paintTextPage(QPainter &painter, QTextDocument &doc, int index, qreal scaleX, qreal scaleY,
                   qreal margin)
{
    const QSizeF page = doc.pageSize();

    // Text without an explicit colour is drawn with the paint context's palette.
    // QTextDocument::drawContents() would use the APPLICATION palette, which is
    // white text under Windows dark mode: white ink on white paper. Always ink
    // with the light print palette instead.
    const QRectF pageRect(0, index * page.height(), page.width(), page.height());
    QAbstractTextDocumentLayout::PaintContext context;
    context.clip = pageRect;
    const QColor ink(ReaderAppearanceDefaults::calmPalette().mainText);
    context.palette.setColor(QPalette::Text, ink);
    context.palette.setColor(QPalette::WindowText, ink);

    painter.save();
    painter.scale(scaleX, scaleY);
    painter.translate(0, -index * page.height());
    painter.setClipRect(pageRect);
    doc.documentLayout()->draw(&painter, context);
    painter.restore();

    QFont footerFont;
    footerFont.setPixelSize(kFooterPixelSize);
    painter.save();
    painter.scale(scaleX, scaleY);
    painter.setFont(footerFont);
    painter.setPen(QColor(110, 110, 110));
    painter.drawText(QRectF(0, page.height() - margin, page.width(), margin), Qt::AlignCenter,
                     QStringLiteral("%1 / %2").arg(index + 1).arg(doc.pageCount()));
    painter.restore();
}

PrintResult printText(const PrintableContent &content, QPrinter &printer, const PageLayoutSettings &settings)
{
    const qreal marginMm = layout::marginMm(settings.margins);
    const auto doc = buildDocument(content, printerPaperMm(printer), marginMm);
    const int total = doc->pageCount();
    const auto [first, last] = pageRange(printer, total);
    if (first > last)
        return {false, 0, QStringLiteral("The selected page range is empty (the document has %1 pages).").arg(total)};

    QPainter painter;
    if (!painter.begin(&printer))
        return {false, 0, QStringLiteral("Could not start printing. Check that the printer is available.")};

    const qreal dpi = DocumentPrinter::layoutDpi();
    const qreal scaleX = printer.logicalDpiX() / dpi;
    const qreal scaleY = printer.logicalDpiY() / dpi;
    const qreal margin = layout::mmToPx(marginMm, dpi);

    for (int i = first; i <= last; ++i) {
        if (i > first && !printer.newPage()) {
            painter.end();
            return {false, i - first, QStringLiteral("The printer stopped after %1 pages.").arg(i - first)};
        }
        paintTextPage(painter, *doc, i, scaleX, scaleY, margin);
    }

    if (!painter.end() || printer.printerState() == QPrinter::Error)
        return {false, 0, QStringLiteral("The printer reported an error.")};
    return {true, last - first + 1, {}};
}

[[nodiscard]] std::optional<PdfDocumentResult> loadPdf(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("Cannot open %1").arg(path);
        return std::nullopt;
    }
    PdfDocumentResult loaded = PdfDocumentLoader().load(file.readAll());
    if (!loaded.ok()) {
        *error = QStringLiteral("This PDF cannot be read: %1").arg(loaded.diagnosticMessage);
        return std::nullopt;
    }
    return loaded;
}

PrintResult printPdf(const PrintableContent &content, QPrinter &printer)
{
    QString error;
    const auto loaded = loadPdf(content.pdfPath, &error);
    if (!loaded)
        return {false, 0, error};

    const int total = loaded->document.pageTree.actualCount;
    if (total <= 0)
        return {false, 0, QStringLiteral("The PDF has no pages.")};
    const auto [first, last] = pageRange(printer, total);
    if (first > last)
        return {false, 0, QStringLiteral("The selected page range is empty (the PDF has %1 pages).").arg(total)};

    QPainter painter;
    if (!painter.begin(&printer))
        return {false, 0, QStringLiteral("Could not start printing. Check that the printer is available.")};

    PdfStreamDecoder decoder;
    PdfPageImageRenderer renderer(decoder);
    PdfPathRenderSettings renderSettings;
    renderSettings.dpi = kPdfRenderDpi;
    renderSettings.antialiasing = true;

    const QRectF paper = printer.paperRect(QPrinter::DevicePixel);
    for (int i = first; i <= last; ++i) {
        if (i > first && !printer.newPage()) {
            painter.end();
            return {false, i - first, QStringLiteral("The printer stopped after %1 pages.").arg(i - first)};
        }
        const PdfPageImageRenderResult page = renderer.renderPage(loaded->document, i, renderSettings);
        if (page.image.isNull()) {
            painter.drawText(paper, Qt::AlignCenter,
                             QStringLiteral("Page %1 could not be rendered.\n%2").arg(i + 1).arg(page.diagnosticSummary));
            continue;
        }
        const QSizeF fitted = QSizeF(page.image.size()).scaled(paper.size(), Qt::KeepAspectRatio);
        const QRectF target(paper.left() + (paper.width() - fitted.width()) / 2,
                            paper.top() + (paper.height() - fitted.height()) / 2,
                            fitted.width(), fitted.height());
        painter.drawImage(target, page.image);
    }

    if (!painter.end() || printer.printerState() == QPrinter::Error)
        return {false, 0, QStringLiteral("The printer reported an error.")};
    return {true, last - first + 1, {}};
}

} // namespace

qreal DocumentPrinter::layoutDpi()
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    return screen ? screen->logicalDotsPerInchY() : 96.0;
}

void DocumentPrinter::configurePrinter(QPrinter &printer, const PageLayoutSettings &settings)
{
    printer.setPageSize(QPageSize(pageSizeId(settings.paper)));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setFullPage(true); // the document carries its own margins
}

std::unique_ptr<QTextDocument> DocumentPrinter::buildTextDocument(const PrintableContent &content,
                                                                  const PageLayoutSettings &settings)
{
    return buildDocument(content, layout::paperSizeMm(settings.paper), layout::marginMm(settings.margins));
}

QImage DocumentPrinter::renderPageImage(const PrintableContent &content, const PageLayoutSettings &settings,
                                        int pageIndex, qreal dpi)
{
    if (content.kind != PrintableContent::Kind::Html && content.kind != PrintableContent::Kind::PlainText)
        return {};
    const auto doc = buildTextDocument(content, settings);
    if (pageIndex < 0 || pageIndex >= doc->pageCount())
        return {};

    const qreal scale = dpi / layoutDpi();
    const QSizeF page = doc->pageSize() * scale;
    QImage image(page.toSize(), QImage::Format_RGB32);
    image.fill(Qt::white); // paper
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    paintTextPage(painter, *doc, pageIndex, scale, scale, layout::mmToPx(layout::marginMm(settings.margins), layoutDpi()));
    return image;
}

int DocumentPrinter::pageCount(const PrintableContent &content, const PageLayoutSettings &settings)
{
    switch (content.kind) {
    case PrintableContent::Kind::None:
        return 0;
    case PrintableContent::Kind::Html:
    case PrintableContent::Kind::PlainText:
        return buildTextDocument(content, settings)->pageCount();
    case PrintableContent::Kind::Pdf: {
        QString error;
        const auto loaded = loadPdf(content.pdfPath, &error);
        return loaded ? loaded->document.pageTree.actualCount : 0;
    }
    }
    return 0;
}

PrintResult DocumentPrinter::print(const PrintableContent &content, QPrinter &printer,
                                   const PageLayoutSettings &settings)
{
    switch (content.kind) {
    case PrintableContent::Kind::None:
        return {false, 0, QStringLiteral("There is no document to print.")};
    case PrintableContent::Kind::Html:
    case PrintableContent::Kind::PlainText:
        return printText(content, printer, settings);
    case PrintableContent::Kind::Pdf:
        return printPdf(content, printer);
    }
    return {false, 0, QStringLiteral("Unknown document type.")};
}

} // namespace mdbr::print
