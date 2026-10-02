// Tests for mdbr::files — which files the reader accepts.

#include <QTest>
#include "services/SupportedFileTypes.h"

using mdbr::files::FileKind;

Q_DECLARE_METATYPE(FileKind)

class SupportedFileTypesTest : public QObject
{
    Q_OBJECT

private slots:
    void classifiesPaths_data();
    void classifiesPaths();
    void dialogFilterListsEveryExtension();
};

void SupportedFileTypesTest::classifiesPaths_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<FileKind>("expected");

    const QStringList markdown = {"md", "markdown", "mdown", "mdwn", "mkd", "mkdn", "mkdown",
                                  "mdtxt", "mdtext", "rmd", "qmd", "mdx", "livemd", "ronn"};
    for (const QString &ext : markdown)
        QTest::newRow(qPrintable("markdown ." + ext)) << "C:/docs/readme." + ext << FileKind::Markdown;

    QTest::newRow("txt")                << "notes.txt"            << FileKind::PlainText;
    QTest::newRow("text")               << "notes.text"           << FileKind::PlainText;
    QTest::newRow("pdf")                << "paper.pdf"            << FileKind::Pdf;
    QTest::newRow("upper case")         << "README.MD"            << FileKind::Markdown;
    QTest::newRow("mixed case")         << "Guide.Markdown"       << FileKind::Markdown;
    QTest::newRow("Rmd case")           << "analysis.Rmd"         << FileKind::Markdown;
    QTest::newRow("double suffix")      << "notes.backup.md"      << FileKind::Markdown;
    QTest::newRow("dotted folder")      << "C:/my.md.folder/a.md" << FileKind::Markdown;
    QTest::newRow("archive")            << "docs.md.zip"          << FileKind::Unsupported;
    QTest::newRow("cmd is not md")      << "build.cmd"            << FileKind::Unsupported;
    QTest::newRow("no extension")       << "README"               << FileKind::Unsupported;
    QTest::newRow("trailing dot")       << "file."                << FileKind::Unsupported;
    QTest::newRow("folder named md")    << "C:/notes.md/file"     << FileKind::Unsupported;
    QTest::newRow("docx")               << "report.docx"          << FileKind::Unsupported;
}

void SupportedFileTypesTest::classifiesPaths()
{
    QFETCH(QString, path);
    QFETCH(FileKind, expected);

    QCOMPARE(mdbr::files::fileKind(path), expected);
    QCOMPARE(mdbr::files::isSupported(path), expected != FileKind::Unsupported);
    QCOMPARE(mdbr::files::isPdf(path), expected == FileKind::Pdf);
}

void SupportedFileTypesTest::dialogFilterListsEveryExtension()
{
    const QString filter = mdbr::files::openDialogFilter();
    const QStringList all = mdbr::files::markdownExtensions()
                          + mdbr::files::plainTextExtensions()
                          + mdbr::files::pdfExtensions();
    for (const QString &ext : all)
        QVERIFY2(filter.contains("*." + ext + " ") || filter.contains("*." + ext + ")"),
                 qPrintable("dialog filter is missing *." + ext));
    QVERIFY(filter.endsWith("All Files (*)"));
}

QTEST_APPLESS_MAIN(SupportedFileTypesTest)
#include "SupportedFileTypesTest.moc"
