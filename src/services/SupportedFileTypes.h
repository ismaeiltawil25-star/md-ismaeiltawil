#pragma once

// SupportedFileTypes — the single source of truth for which files the reader
// opens. The open dialog filter, drag-and-drop, command-line arguments and the
// Recent Files menu all ask this module; never hard-code extensions elsewhere.
//
// To support a new extension, add it to the matching list in the .cpp file and
// add a row to SupportedFileTypesTest.
//
// Matching is by the last suffix only and is case-insensitive ("README.MD",
// "notes.backup.md" are Markdown; "archive.md.zip" is not).

#include <QString>
#include <QStringList>

namespace mdbr::files {

enum class FileKind
{
    Unsupported,
    Markdown,   // rendered by the Markdown pipeline
    PlainText,  // also rendered by the Markdown pipeline (plain text is valid Markdown)
    Pdf,        // rendered by the PDF preview
};

[[nodiscard]] FileKind fileKind(const QString &path);

[[nodiscard]] inline bool isSupported(const QString &path) { return fileKind(path) != FileKind::Unsupported; }
[[nodiscard]] inline bool isPdf(const QString &path) { return fileKind(path) == FileKind::Pdf; }

// Lowercase extensions without the dot, e.g. {"md", "markdown", ...}.
[[nodiscard]] const QStringList &markdownExtensions();
[[nodiscard]] const QStringList &plainTextExtensions();
[[nodiscard]] const QStringList &pdfExtensions();

// Filter string for QFileDialog, e.g. "Supported Files (*.md *.markdown ...);;...".
[[nodiscard]] QString openDialogFilter();

} // namespace mdbr::files
