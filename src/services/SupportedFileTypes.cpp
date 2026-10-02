#include "SupportedFileTypes.h"

#include <QFileInfo>

namespace mdbr::files {

const QStringList &markdownExtensions()
{
    static const QStringList extensions = {
        // Standard Markdown extensions (GitHub Linguist, editors, static site generators)
        QStringLiteral("md"),
        QStringLiteral("markdown"),
        QStringLiteral("mdown"),
        QStringLiteral("mdwn"),
        QStringLiteral("mkd"),
        QStringLiteral("mkdn"),
        QStringLiteral("mkdown"),
        QStringLiteral("mdtxt"),
        QStringLiteral("mdtext"),
        // Markdown dialects: extra syntax is shown as code or plain text
        QStringLiteral("rmd"),    // R Markdown
        QStringLiteral("qmd"),    // Quarto
        QStringLiteral("mdx"),    // Markdown + JSX (JSX shown literally)
        QStringLiteral("livemd"), // Elixir Livebook
        QStringLiteral("ronn"),   // man pages written in Markdown
    };
    return extensions;
}

const QStringList &plainTextExtensions()
{
    static const QStringList extensions = {
        QStringLiteral("txt"),
        QStringLiteral("text"), // classic Markdown convention (Daring Fireball)
    };
    return extensions;
}

const QStringList &pdfExtensions()
{
    static const QStringList extensions = {QStringLiteral("pdf")};
    return extensions;
}

FileKind fileKind(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix.isEmpty())
        return FileKind::Unsupported;
    if (markdownExtensions().contains(suffix))
        return FileKind::Markdown;
    if (plainTextExtensions().contains(suffix))
        return FileKind::PlainText;
    if (pdfExtensions().contains(suffix))
        return FileKind::Pdf;
    return FileKind::Unsupported;
}

namespace {

[[nodiscard]] QString patterns(const QStringList &extensions)
{
    QStringList globs;
    globs.reserve(extensions.size());
    for (const QString &ext : extensions)
        globs << QStringLiteral("*.") + ext;
    return globs.join(u' ');
}

} // namespace

QString openDialogFilter()
{
    const QString markdownAndText = patterns(markdownExtensions() + plainTextExtensions());
    return QStringLiteral("Supported Files (%1 %2);;Markdown / Text Files (%1);;PDF Files (%2);;All Files (*)")
        .arg(markdownAndText, patterns(pdfExtensions()));
}

} // namespace mdbr::files
