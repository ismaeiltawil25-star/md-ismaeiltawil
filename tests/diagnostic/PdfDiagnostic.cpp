#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QApplication>
#include "PdfDocumentLoader.h"
#include "PdfPageImageRenderer.h"
#include "PdfStreamDecoder.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QTextStream out(stdout);

    if (argc < 2) {
        out << "Usage: PdfDiagnostic <path-to-pdf>" << Qt::endl;
        return 1;
    }

    QString path(argv[1]);
    out << "=== File Path Diagnostic ===" << Qt::endl;
    out << "Path: " << path << Qt::endl;

    QFileInfo fi(path);
    out << "exists: " << fi.exists() << Qt::endl;
    out << "isFile: " << fi.isFile() << Qt::endl;
    out << "size: " << fi.size() << " bytes" << Qt::endl;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        out << "Cannot open file: " << f.errorString() << Qt::endl;
        return 1;
    }

    QByteArray data = f.readAll();
    out << "Read " << data.size() << " bytes" << Qt::endl;

    if (data.size() >= 8) {
        QByteArray first8 = data.left(8);
        out << "First 8 hex: " << first8.toHex(' ').toUpper() << Qt::endl;
        out << "First 8 ascii: " << QString::fromLatin1(first8) << Qt::endl;
        out << "Starts with %PDF-: " << data.startsWith("%PDF-") << Qt::endl;
    }

    out << Qt::endl;
    out << "=== PdfDocumentLoader ===" << Qt::endl;
    auto docResult = PdfDocumentLoader().load(data);
    out << "warning: " << static_cast<int>(docResult.warning) << Qt::endl;
    out << "diagnostic: " << docResult.diagnosticMessage << Qt::endl;
    out << "pageCount: " << docResult.document.pageCount() << Qt::endl;
    out << "catalog.rootRef: " << docResult.document.catalog.rootRef.objNum
         << " " << docResult.document.catalog.rootRef.genNum << Qt::endl;

    if (docResult.warning != PdfDocumentLoaderWarning::None) {
        out << "FAILED at loader layer" << Qt::endl;
        return 2;
    }

    out << Qt::endl;
    out << "=== PdfPageImageRenderer ===" << Qt::endl;
    PdfStreamDecoder decoder;
    PdfPageImageRenderer renderer(decoder);
    auto renderRes = renderer.renderPage(docResult.document, 0);
    out << "warning: " << static_cast<int>(renderRes.warning) << Qt::endl;
    out << "image.isNull: " << renderRes.image.isNull() << Qt::endl;
    out << "image size: " << renderRes.imageWidth << "x" << renderRes.imageHeight << Qt::endl;
    out << "diagnosticSummary: " << renderRes.diagnosticSummary << Qt::endl;
    out << "planResult.ok: " << renderRes.planResult.ok() << Qt::endl;
    out << "pathResult.warning: " << static_cast<int>(renderRes.pathResult.warning) << Qt::endl;
    out << "textRenderResult.warning: " << static_cast<int>(renderRes.textRenderResult.warning) << Qt::endl;
    out << "textRenderResult.runsRendered: " << renderRes.textRenderResult.runsRendered << Qt::endl;
    out << "textRenderResult.runsSkipped: " << renderRes.textRenderResult.runsSkipped << Qt::endl;

    out << Qt::endl;
    out << "=== Summary ===" << Qt::endl;
    if (!renderRes.image.isNull()) {
        out << "SUCCESS: Image rendered " << renderRes.imageWidth << "x" << renderRes.imageHeight << Qt::endl;
        return 0;
    } else {
        out << "FAILURE: No image produced" << Qt::endl;
        return 3;
    }
}
