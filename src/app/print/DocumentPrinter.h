#pragma once

// DocumentPrinter — prints a PrintableContent to any QPrinter (a real printer,
// a PDF file, or the print-preview widget).
//
// Markdown and text:
//   * paper size and margins come from the Page Layout settings, and the page
//     is split with the same PageGeometry as page view, so the printout matches
//     what page view shows on screen;
//   * printing always uses the light "calm" palette (dark text on white), never
//     the night theme, to save ink and stay readable on paper;
//   * every page gets a "page / total" footer in the bottom margin.
//
// PDF: every page is rendered by the project's PDF engine and fitted to the
// paper, keeping its aspect ratio.
//
// The page range selected in the print dialog (printer.fromPage/toPage) is
// honoured for both.

#include "PrintableContent.h"
#include "app/layout/PageLayoutSettings.h"

#include <QImage>
#include <QString>

#include <memory>

class QPrinter;
class QTextDocument;

namespace mdbr::print {

struct PrintResult
{
    bool ok = false;
    int pagesPrinted = 0;
    QString error; // user-facing message when !ok
};

class DocumentPrinter
{
public:
    // Paper size and orientation from the settings; full-page coordinates
    // (the document itself carries the margins).
    static void configurePrinter(QPrinter &printer, const layout::PageLayoutSettings &settings);

    // The document that will be printed for Html/PlainText content: print
    // colours, page-view margins and page size. Exposed for tests.
    [[nodiscard]] static std::unique_ptr<QTextDocument>
    buildTextDocument(const PrintableContent &content, const layout::PageLayoutSettings &settings);

    // Number of pages `content` will produce (0 for nothing printable).
    [[nodiscard]] static int pageCount(const PrintableContent &content,
                                       const layout::PageLayoutSettings &settings);

    // One page of Html/PlainText content as an image at `dpi`, drawn by exactly
    // the same code as printing (paper colour, margins, footer). For previews
    // and tests; returns a null image for PDFs or out-of-range pages.
    [[nodiscard]] static QImage renderPageImage(const PrintableContent &content,
                                                const layout::PageLayoutSettings &settings,
                                                int pageIndex, qreal dpi = 96.0);

    static PrintResult print(const PrintableContent &content, QPrinter &printer,
                             const layout::PageLayoutSettings &settings);

    // Logical DPI the print document is laid out at (matches the screen, so
    // pagination equals page view).
    [[nodiscard]] static qreal layoutDpi();
};

} // namespace mdbr::print
