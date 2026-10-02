#pragma once

// PrintableContent — WHAT to print, independent of any widget.
//
// MainWindow builds one from the current tab; DocumentPrinter turns it into
// pages. Keeping this a plain value type lets printing be tested without UI.

#include <QString>
#include <QUrl>

namespace mdbr::print {

struct PrintableContent
{
    enum class Kind
    {
        None,      // nothing printable (no tab open)
        Html,      // rendered Markdown
        PlainText, // text files and the render-failure fallback
        Pdf,       // PDF tabs: pages are rendered by the PDF engine
    };

    Kind kind = Kind::None;
    QString title;      // shown as the print job name
    QString html;       // Kind::Html
    QString plainText;  // Kind::PlainText
    QUrl baseUrl;       // resolves relative images in Html
    QString pdfPath;    // Kind::Pdf

    [[nodiscard]] bool isPrintable() const { return kind != Kind::None; }
};

} // namespace mdbr::print
