#include "PdfDocumentLoader.h"
#include "PdfByteReader.h"
#include "PdfHeaderParser.h"
#include "PdfXRefParser.h"
#include "PdfTrailerParser.h"
#include "PdfIndirectObjectLoader.h"
#include "PdfCatalogResolver.h"
#include "PdfPageTreeResolver.h"

bool PdfDocumentLoader::isStructuralPageTreeWarning(PdfPageTreeResolverWarning w) {
    switch (w) {
    case PdfPageTreeResolverWarning::UnresolvedPageNode:
    case PdfPageTreeResolverWarning::InvalidPageNode:
    case PdfPageTreeResolverWarning::InvalidPageType:
    case PdfPageTreeResolverWarning::MissingKids:
    case PdfPageTreeResolverWarning::InvalidKids:
    case PdfPageTreeResolverWarning::InvalidKidReference:
    case PdfPageTreeResolverWarning::CycleDetected:
    case PdfPageTreeResolverWarning::DepthExceeded:
        return true;
    default:
        return false;
    }
}

PdfDocumentLoader::PdfStartxrefResult PdfDocumentLoader::findStartxref(QByteArrayView data) {
    PdfStartxrefResult result;
    if (data.size() < 12) {
        result.warning = PdfDocumentLoaderWarning::MissingEofMarker;
        result.diagnosticMessage = QStringLiteral("Data too short to contain %%EOF");
        return result;
    }

    qsizetype searchStart = qMax(qsizetype(0), data.size() - 1024);
    QByteArrayView tail = data.sliced(searchStart);

    qsizetype eofRel = tail.lastIndexOf("%%EOF");
    if (eofRel < 0) {
        result.warning = PdfDocumentLoaderWarning::MissingEofMarker;
        result.diagnosticMessage = QStringLiteral("%%EOF marker not found near end of data");
        return result;
    }
    qsizetype eofAbs = searchStart + eofRel;

    // Allow trailing whitespace after %%EOF
    qsizetype afterEof = eofAbs + 5;
    while (afterEof < data.size()) {
        char c = data[afterEof];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            afterEof++;
        } else break;
    }
    // Tolerated; no error for trailing whitespace

    // Search for startxref before %%EOF
    QByteArrayView beforeEof = data.sliced(searchStart, eofAbs - searchStart);
    qsizetype sxRel = beforeEof.lastIndexOf("startxref");
    if (sxRel < 0) {
        result.warning = PdfDocumentLoaderWarning::MissingStartxref;
        result.diagnosticMessage = QStringLiteral("startxref not found before %%EOF");
        return result;
    }
    qsizetype sxAbs = searchStart + sxRel + 9; // past "startxref"

    // Skip whitespace after startxref
    while (sxAbs < eofAbs) {
        char c = data[sxAbs];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            sxAbs++;
        } else break;
    }

    if (sxAbs >= eofAbs) {
        result.warning = PdfDocumentLoaderWarning::InvalidStartxref;
        result.diagnosticMessage = QStringLiteral("No offset value after startxref");
        return result;
    }

    // Read the integer token
    qsizetype numStart = sxAbs;
    qsizetype numEnd = sxAbs;
    while (numEnd < eofAbs) {
        char c = data[numEnd];
        if (c >= '0' && c <= '9') {
            numEnd++;
        } else break;
    }

    if (numEnd == numStart) {
        result.warning = PdfDocumentLoaderWarning::InvalidStartxref;
        result.diagnosticMessage = QStringLiteral("No valid offset after startxref");
        return result;
    }

    // Check for sign characters (reject both - and +)
    if (numStart > 0) {
        char prev = data[numStart - 1];
        if (prev == '-' || prev == '+') {
            result.warning = PdfDocumentLoaderWarning::InvalidStartxref;
            result.diagnosticMessage = QStringLiteral("Signed startxref offset not allowed");
            return result;
        }
    }

    // After the digit run, only whitespace allowed before %%EOF
    while (numEnd < eofAbs) {
        char c = data[numEnd];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            numEnd++;
        } else {
            result.warning = PdfDocumentLoaderWarning::InvalidStartxref;
            result.diagnosticMessage = QStringLiteral("Unexpected characters between startxref offset and %%EOF");
            return result;
        }
    }

    QByteArrayView numView = data.sliced(numStart, numEnd - numStart);
    bool ok = false;
    qsizetype offset = numView.toLongLong(&ok);
    if (!ok || offset < 0) {
        result.warning = PdfDocumentLoaderWarning::InvalidStartxref;
        result.diagnosticMessage = QStringLiteral("Invalid startxref offset");
        return result;
    }
    if (offset >= data.size()) {
        result.warning = PdfDocumentLoaderWarning::InvalidStartxref;
        result.diagnosticMessage = QStringLiteral("startxref offset out of range");
        return result;
    }

    result.offset = offset;
    return result;
}

PdfDocumentResult PdfDocumentLoader::load(QByteArrayView data) {
    PdfDocumentResult result;
    PdfByteReader reader(data);

    // 1. Header
    auto headerResult = PdfHeaderParser::parse(reader);
    if (!headerResult.success) {
        result.warning = PdfDocumentLoaderWarning::InvalidHeader;
        result.diagnosticMessage = QStringLiteral("Invalid PDF header: %1").arg(headerResult.errorMessage);
        return result;
    }
    result.document.version = headerResult.version;

    // 2. startxref
    auto sxResult = findStartxref(data);
    if (!sxResult.ok()) {
        result.warning = sxResult.warning;
        result.diagnosticMessage = sxResult.diagnosticMessage;
        return result;
    }

    // 3. XRef
    PdfXRefParser xrefParser;
    auto xrefResult = xrefParser.parse(reader, sxResult.offset);
    result.document.xrefWarning = xrefResult.warning;

    if (xrefResult.warning != PdfXRefParserWarning::None) {
        // Fatal xref warnings
        switch (xrefResult.warning) {
        case PdfXRefParserWarning::MissingXrefKeyword:
        case PdfXRefParserWarning::InvalidStartxref:
        case PdfXRefParserWarning::InvalidSubsectionHeader:
        case PdfXRefParserWarning::TruncatedEntry:
        case PdfXRefParserWarning::UnexpectedToken:
            result.warning = PdfDocumentLoaderWarning::XrefParseFailure;
            result.diagnosticMessage = xrefResult.diagnosticMessage;
            return result;
        default:
            // MalformedEntry is non-fatal — continue
            break;
        }
    }

    result.document.xrefTable = xrefResult.table;
    qsizetype trailerOffset = xrefResult.trailerOffset;
    if (trailerOffset < 0) {
        result.warning = PdfDocumentLoaderWarning::TrailerParseFailure;
        result.diagnosticMessage = QStringLiteral("No trailer offset found in xref table");
        return result;
    }

    // 4. Trailer
    PdfTrailerParser trailerParser;
    auto trailerResult = trailerParser.parse(reader, trailerOffset);
    result.document.trailerWarning = trailerResult.warning;

    if (trailerResult.warning == PdfTrailerParserWarning::EncryptedUnsupported) {
        result.document.trailer = trailerResult.trailer;
        result.warning = PdfDocumentLoaderWarning::EncryptedUnsupported;
        result.diagnosticMessage = trailerResult.diagnosticMessage;
        return result;
    }
    if (trailerResult.warning != PdfTrailerParserWarning::None) {
        result.warning = PdfDocumentLoaderWarning::TrailerParseFailure;
        result.diagnosticMessage = trailerResult.diagnosticMessage;
        return result;
    }

    result.document.trailer = trailerResult.trailer;

    // 5. Encrypted check
    if (result.document.trailer.encrypted) {
        result.warning = PdfDocumentLoaderWarning::EncryptedUnsupported;
        result.diagnosticMessage = QStringLiteral("PDF document is encrypted");
        return result;
    }

    // 6. Object loading
    PdfIndirectObjectLoader objectLoader;
    auto loadResult = objectLoader.load(reader, xrefResult.table, result.document.store);
    result.document.objectLoaderWarning = loadResult.warning;

    if (loadResult.inUseEntries > 0 && loadResult.loadedCount == 0) {
        result.warning = PdfDocumentLoaderWarning::ObjectLoadFailure;
        result.diagnosticMessage = QStringLiteral("No objects could be loaded from %1 in-use xref entries")
            .arg(loadResult.inUseEntries);
        return result;
    }

    // 7. Catalog
    PdfCatalogResolver catalogResolver;
    auto catResult = catalogResolver.resolve(result.document.trailer, result.document.store);
    if (!catResult.ok()) {
        result.warning = PdfDocumentLoaderWarning::CatalogFailure;
        result.diagnosticMessage = catResult.diagnosticMessage;
        return result;
    }
    result.document.catalog = catResult.catalog;

    // 8. Page tree
    PdfPageTreeResolver pageTreeResolver;
    auto ptResult = pageTreeResolver.resolve(result.document.catalog, result.document.store);
    result.document.pageTreeWarning = ptResult.warning;

    if (ptResult.warning != PdfPageTreeResolverWarning::None) {
        if (isStructuralPageTreeWarning(ptResult.warning)) {
            result.warning = PdfDocumentLoaderWarning::PageTreeFailure;
            result.diagnosticMessage = ptResult.diagnosticMessage;
            return result;
        }
        // Count diagnostics — non-fatal, warning stored in document.pageTreeWarning
    }

    result.document.pageTree = ptResult.tree;
    return result;
}
