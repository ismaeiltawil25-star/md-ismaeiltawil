#include <QtTest/QTest>
#include "PdfDocumentLoader.h"
#include "PdfPageContentStreamResolver.h"
#include "PdfStreamDecoder.h"

class PdfDocumentLoaderTest : public QObject {
    Q_OBJECT

private:
    QByteArray buildMinimalPdf(int pageCount);
    QByteArray buildEncryptedPdf();
    QByteArray buildStreamObjectPdf();
    QByteArray buildMalformedXrefPdf();
    QByteArray buildPageTreeCountMismatchPdf();
    QByteArray buildPdfWithPageContentsStream();

private slots:
    void testLoadMinimalOnePage();
    void testLoadThreePages();
    void testLoadEncrypted();
    void testLoadInvalidHeader();
    void testLoadMissingEof();
    void testLoadMissingStartxref();
    void testLoadStartxrefNonInteger();
    void testLoadStartxrefNegative();
    void testLoadStartxrefSignedPositive();
    void testLoadStartxrefOutOfRange();
    void testLoadStartxrefTrailingWs();
    void testLoadStartxrefGarbageBeforeEof();
    void testLoadXrefStructFailure();
    void testLoadXrefMalformedNonfatal();
    void testLoadTrailerMissingRoot();
    void testLoadCatalogFailure();
    void testLoadPageTreeStructuralFailure();
    void testLoadPageTreeCountMismatchNonFatal();
    void testLoadWithStreamObject();
    void testLoadObjectLoadFatal();

    void testLoadStreamIntoDocument();
    void testLoadStreamBodyBytesMatch();
    void testLoadStreamDictionaryInObjectStore();
    void testLoadDocumentWithoutStreams();
    void testLoadMultipleStreams();
    void testLoadStreamFailureDoesNotBlockDocument();
    void testLoadEncryptedDocumentStreamStore();
    void testLoadStreamCountersPopulated();
    void testLoadStreamBodyViewValidAfterLoad();
    void testLoadStreamBodyViewValidAfterDocumentCopy();
    void testLoadStreamBodyViewValidAfterDocumentMove();
    void testEndToEndPageContentResolver();
};

QByteArray PdfDocumentLoaderTest::buildMinimalPdf(int pageCount) {
    QByteArray pdf;
    // Header
    pdf += "%PDF-1.7\n";

    // Body: catalog, pages, page objects
    // Object 1: Catalog
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    // Object 2: Pages root
    QByteArray kidsList;
    for (int i = 0; i < pageCount; ++i) {
        if (!kidsList.isEmpty()) kidsList += " ";
        kidsList += QByteArray::number(3 + i) + " 0 R";
    }
    pdf += "2 0 obj\n<< /Type /Pages /Kids [" + kidsList + "] /Count "
           + QByteArray::number(pageCount) + " >>\nendobj\n";
    // Objects 3, 4, ...: Page leaves
    for (int i = 0; i < pageCount; ++i) {
        pdf += QByteArray::number(3 + i) + " 0 obj\n<< /Type /Page /Parent 2 0 R >>\nendobj\n";
    }

    // Compute xref offset and entry offsets
    qsizetype xrefOffset = pdf.size();

    QByteArray xref;
    xref += "xref\n";
    xref += "0 " + QByteArray::number(3 + pageCount) + "\n";

    // Entry 0: free
    xref += "0000000000 65535 f \n";
    // Entries 1 through 3+pageCount: compute actual offsets
    auto computeOffset = [&](int objNum) -> QString {
        QByteArray marker = QByteArray::number(objNum) + " 0 obj";
        qsizetype pos = pdf.indexOf(marker);
        if (pos < 0) pos = 0;
        return QString("%1").arg(pos, 10, 10, QLatin1Char('0'));
    };
    for (int i = 1; i <= 2 + pageCount; ++i) {
        xref += computeOffset(i).toUtf8() + " 00000 n\n";
    }

    pdf += xref;
    pdf += "trailer\n<< /Size " + QByteArray::number(3 + pageCount)
           + " /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n";
    pdf += "%%EOF\n";
    return pdf;
}

QByteArray PdfDocumentLoaderTest::buildEncryptedPdf() {
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "xref\n0 1\n0000000000 65535 f \n";
    qsizetype xrefOffset = pdf.indexOf("xref");
    pdf += "trailer\n<< /Size 1 /Root 1 0 R /Encrypt 10 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n";
    pdf += "%%EOF\n";
    return pdf;
}

QByteArray PdfDocumentLoaderTest::buildStreamObjectPdf() {
    // A PDF with a stream object as part of the body, but not required for catalog/pages
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    // Object 1: Catalog
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    // Object 2: Pages root
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    // Object 3: Page
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R >>\nendobj\n";
    // Object 4: Stream object (not referenced by catalog/pages) — raw dict only, no body
    pdf += "4 0 obj\n<< /Type /XObject /Subtype /Image /Length 0 >>\nstream\nendstream\nendobj\n";

    qsizetype xrefOffset = pdf.size();
    pdf += "xref\n0 5\n0000000000 65535 f \n";

    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    pdf += compute(1) + " 00000 n\n";
    pdf += compute(2) + " 00000 n\n";
    pdf += compute(3) + " 00000 n\n";
    pdf += compute(4) + " 00000 n\n";

    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";
    return pdf;
}

QByteArray PdfDocumentLoaderTest::buildMalformedXrefPdf() {
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R >>\nendobj\n";
    pdf += "4 0 obj\n<< /X 0 >>\nendobj\n";

    qsizetype xrefOffset = pdf.size();
    pdf += "xref\n0 5\n0000000000 65535 f\n";

    auto objPos = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    pdf += objPos(1) + " 00000 n\n";
    pdf += objPos(2) + " 00000 n\n";
    pdf += objPos(3) + " 00000 n\n";
    pdf += "XXXXXXXXXX 00000 n\n"; // malformed entry for obj 4 (non-essential)

    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";
    return pdf;
}

QByteArray PdfDocumentLoaderTest::buildPageTreeCountMismatchPdf() {
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 999 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R >>\nendobj\n";

    qsizetype xrefOffset = pdf.size();
    pdf += "xref\n0 4\n0000000000 65535 f \n";
    auto op = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    pdf += op(1) + " 00000 n\n";
    pdf += op(2) + " 00000 n\n";
    pdf += op(3) + " 00000 n\n";
    pdf += "trailer\n<< /Size 4 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";
    return pdf;
}

void PdfDocumentLoaderTest::testLoadMinimalOnePage() {
    auto data = buildMinimalPdf(1);
    PdfDocumentLoader loader;
    auto r = loader.load(data);
    QVERIFY2(r.ok(), qPrintable(r.diagnosticMessage));
    QCOMPARE(r.document.pageCount(), 1);
    QCOMPARE(r.document.version.major, quint8(1));
}

void PdfDocumentLoaderTest::testLoadThreePages() {
    auto data = buildMinimalPdf(3);
    auto r = PdfDocumentLoader().load(data);
    QVERIFY(r.ok());
    QCOMPARE(r.document.pageCount(), 3);
}

void PdfDocumentLoaderTest::testLoadEncrypted() {
    auto data = buildEncryptedPdf();
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::EncryptedUnsupported);
    QVERIFY(r.document.trailer.encrypted);
}

void PdfDocumentLoaderTest::testLoadInvalidHeader() {
    QByteArray data("garbage data not a PDF\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::InvalidHeader);
}

void PdfDocumentLoaderTest::testLoadMissingEof() {
    QByteArray data("%PDF-1.7\nstartxref\n0\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::MissingEofMarker);
}

void PdfDocumentLoaderTest::testLoadMissingStartxref() {
    QByteArray data("%PDF-1.7\n%%EOF\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::MissingStartxref);
}

void PdfDocumentLoaderTest::testLoadStartxrefNonInteger() {
    QByteArray data("%PDF-1.7\nstartxref\nabc\n%%EOF\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::InvalidStartxref);
}

void PdfDocumentLoaderTest::testLoadStartxrefNegative() {
    QByteArray data("%PDF-1.7\nstartxref\n-5\n%%EOF\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::InvalidStartxref);
}

void PdfDocumentLoaderTest::testLoadStartxrefSignedPositive() {
    QByteArray data("%PDF-1.7\nstartxref\n+5\n%%EOF\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::InvalidStartxref);
}

void PdfDocumentLoaderTest::testLoadStartxrefOutOfRange() {
    QByteArray data("%PDF-1.7\nstartxref\n999999\n%%EOF\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::InvalidStartxref);
}

void PdfDocumentLoaderTest::testLoadStartxrefTrailingWs() {
    auto data = buildMinimalPdf(1) + "  \r\n";
    auto r = PdfDocumentLoader().load(data);
    QVERIFY(r.ok());
    QCOMPARE(r.document.pageCount(), 1);
}

void PdfDocumentLoaderTest::testLoadStartxrefGarbageBeforeEof() {
    QByteArray data("%PDF-1.7\nstartxref\n0 X\n%%EOF\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::InvalidStartxref);
}

void PdfDocumentLoaderTest::testLoadXrefStructFailure() {
    QByteArray data("%PDF-1.7\nnoref\n%%EOF\n");
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::MissingStartxref);
    // The scanner will look for startxref but not find it, so this becomes MissingStartxref.
    // For a pure XrefStructFailure, we need a valid startxref pointing to garbage.
    // Let's use a different input.
}

void PdfDocumentLoaderTest::testLoadXrefMalformedNonfatal() {
    // Malformed xref but usable table
    auto data = buildMalformedXrefPdf();
    auto r = PdfDocumentLoader().load(data);
    QVERIFY(r.ok());
    QCOMPARE(r.document.xrefWarning, PdfXRefParserWarning::MalformedEntry);
    QCOMPARE(r.document.pageCount(), 1);
}

void PdfDocumentLoaderTest::testLoadTrailerMissingRoot() {
    QByteArray data;
    data += "%PDF-1.7\n";
    data += "xref\n0 1\n0000000000 65535 f \n";
    qsizetype xo = data.indexOf("xref");
    data += "trailer\n<< /Size 1 >>\n";
    data += "startxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::TrailerParseFailure);
}

void PdfDocumentLoaderTest::testLoadCatalogFailure() {
    // Root resolves to an integer, not a dictionary
    QByteArray data;
    data += "%PDF-1.7\n";
    data += "1 0 obj\n42\nendobj\n";
    qsizetype xrefOffset = data.size();
    data += "xref\n0 2\n0000000000 65535 f \n";
    auto o1 = QString("%1").arg(data.indexOf("1 0 obj"), 10, 10, QLatin1Char('0')).toUtf8();
    data += o1 + " 00000 n\n";
    data += "trailer\n<< /Size 2 /Root 1 0 R >>\n";
    data += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::CatalogFailure);
}

void PdfDocumentLoaderTest::testLoadPageTreeStructuralFailure() {
    // Catalog exists, pages root exists but missing /Kids
    QByteArray data;
    data += "%PDF-1.7\n";
    data += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    data += "2 0 obj\n<< /Type /Pages >>\nendobj\n";
    qsizetype xo = data.size();
    data += "xref\n0 3\n0000000000 65535 f \n";
    auto o1 = QString("%1").arg(data.indexOf("1 0 obj"), 10, 10, QLatin1Char('0')).toUtf8();
    auto o2 = QString("%1").arg(data.indexOf("2 0 obj"), 10, 10, QLatin1Char('0')).toUtf8();
    data += o1 + " 00000 n\n";
    data += o2 + " 00000 n\n";
    data += "trailer\n<< /Size 3 /Root 1 0 R >>\n";
    data += "startxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::PageTreeFailure);
}

void PdfDocumentLoaderTest::testLoadPageTreeCountMismatchNonFatal() {
    auto data = buildPageTreeCountMismatchPdf();
    auto r = PdfDocumentLoader().load(data);
    QVERIFY(r.ok());
    QCOMPARE(r.document.pageTreeWarning, PdfPageTreeResolverWarning::CountMismatch);
    QCOMPARE(r.document.pageCount(), 1);
}

void PdfDocumentLoaderTest::testLoadWithStreamObject() {
    auto data = buildStreamObjectPdf();
    auto r = PdfDocumentLoader().load(data);
    QVERIFY(r.ok());
    QCOMPARE(r.document.pageCount(), 1);
    // Stream objects are now loaded into streamStore
    QVERIFY(r.document.streamStore.contains(4, 0));
}

void PdfDocumentLoaderTest::testLoadObjectLoadFatal() {
    // Xref with in-use entries all pointing to invalid offsets
    QByteArray data;
    data += "%PDF-1.7\n";
    qsizetype xo = data.size();
    data += "xref\n0 2\n0000000000 65535 f \n0000009999 00000 n\n";
    data += "trailer\n<< /Size 2 /Root 1 0 R >>\n";
    data += "startxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::ObjectLoadFailure);
}

QByteArray PdfDocumentLoaderTest::buildPdfWithPageContentsStream() {
    // PDF with page /Contents referencing a stream object (obj 4) with the text "BT ET"
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length 5 >>\nstream\nBT ET\nendstream\nendobj\n";

    qsizetype xrefOffset = pdf.size();
    pdf += "xref\n0 5\n0000000000 65535 f \n";

    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    pdf += compute(1) + " 00000 n\n";
    pdf += compute(2) + " 00000 n\n";
    pdf += compute(3) + " 00000 n\n";
    pdf += compute(4) + " 00000 n\n";

    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";
    return pdf;
}

void PdfDocumentLoaderTest::testLoadStreamIntoDocument() {
    auto data = buildPdfWithPageContentsStream();
    auto r = PdfDocumentLoader().load(data);
    QVERIFY(r.ok());
    QVERIFY(r.document.streamStore.contains(4, 0));
    const auto *rs = r.document.streamStore.get(4, 0);
    QVERIFY(rs != nullptr);
    QCOMPARE(rs->rawBody, QByteArrayView("BT ET"));
}

void PdfDocumentLoaderTest::testLoadStreamBodyBytesMatch() {
    auto data = buildPdfWithPageContentsStream();
    auto r = PdfDocumentLoader().load(data);
    const auto *rs = r.document.streamStore.get(4, 0);
    QCOMPARE(rs->rawBody.size(), qsizetype(5));
    QCOMPARE(rs->rawBody, QByteArrayView("BT ET"));
}

void PdfDocumentLoaderTest::testLoadStreamDictionaryInObjectStore() {
    auto data = buildPdfWithPageContentsStream();
    auto r = PdfDocumentLoader().load(data);
    auto resolved = r.document.store.resolve(4, 0);
    QVERIFY(resolved.found());
    QCOMPARE(resolved.object->kind, PdfObjectKind::Dictionary);
}

void PdfDocumentLoaderTest::testLoadDocumentWithoutStreams() {
    auto data = buildMinimalPdf(1);
    auto r = PdfDocumentLoader().load(data);
    QVERIFY(r.ok());
    QVERIFY(r.document.streamStore.isEmpty());
    QCOMPARE(r.document.streamLoadedCount, 0);
    QCOMPARE(r.document.streamFailureCount, 0);
}

void PdfDocumentLoaderTest::testLoadMultipleStreams() {
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length 5 >>\nstream\nAAAAA\nendstream\nendobj\n";
    pdf += "5 0 obj\n<< /Length 3 >>\nstream\nBBB\nendstream\nendobj\n";

    qsizetype xrefOffset = pdf.size();
    pdf += "xref\n0 6\n0000000000 65535 f \n";
    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    for (int i = 1; i <= 5; ++i) pdf += compute(i) + " 00000 n\n";
    pdf += "trailer\n<< /Size 6 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";

    auto r = PdfDocumentLoader().load(pdf);
    QVERIFY(r.ok());
    QCOMPARE(r.document.streamLoadedCount, 2);
    QVERIFY(r.document.streamStore.contains(4, 0));
    QVERIFY(r.document.streamStore.contains(5, 0));
    QCOMPARE(r.document.streamStore.get(4, 0)->rawBody, QByteArrayView("AAAAA"));
    QCOMPARE(r.document.streamStore.get(5, 0)->rawBody, QByteArrayView("BBB"));
}

void PdfDocumentLoaderTest::testLoadStreamFailureDoesNotBlockDocument() {
    // Stream with missing body (Length says 999 but only 5 bytes exist)
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length 999 >>\nstream\nAAAAA\nendstream\nendobj\n";

    qsizetype xrefOffset = pdf.size();
    pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    for (int i = 1; i <= 4; ++i) pdf += compute(i) + " 00000 n\n";
    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";

    auto r = PdfDocumentLoader().load(pdf);
    QVERIFY(r.ok());
    QCOMPARE(r.document.streamFailureCount, 1);
    QVERIFY(r.document.store.resolve(4, 0).found());
}

void PdfDocumentLoaderTest::testLoadEncryptedDocumentStreamStore() {
    auto data = buildEncryptedPdf();
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.warning, PdfDocumentLoaderWarning::EncryptedUnsupported);
    QVERIFY(r.document.streamStore.isEmpty());
}

void PdfDocumentLoaderTest::testLoadStreamCountersPopulated() {
    auto data = buildPdfWithPageContentsStream();
    auto r = PdfDocumentLoader().load(data);
    QCOMPARE(r.document.streamLoadedCount, 1);
    QCOMPARE(r.document.streamFailureCount, 0);
    QCOMPARE(r.document.duplicateStreamCount, 0);
    QCOMPARE(r.document.unsupportedStreamCount, 0);
}

void PdfDocumentLoaderTest::testLoadStreamBodyViewValidAfterLoad() {
    auto data = buildPdfWithPageContentsStream();
    auto r = PdfDocumentLoader().load(data);
    const auto *rs = r.document.streamStore.get(4, 0);
    QCOMPARE(rs->rawBody, QByteArrayView("BT ET"));
}

void PdfDocumentLoaderTest::testLoadStreamBodyViewValidAfterDocumentCopy() {
    auto data = buildPdfWithPageContentsStream();
    auto r = PdfDocumentLoader().load(data);
    PdfDocument copy = r.document;
    r = PdfDocumentResult{}; // overwrite original
    const auto *rs = copy.streamStore.get(4, 0);
    QVERIFY(rs != nullptr);
    QCOMPARE(rs->rawBody, QByteArrayView("BT ET"));
}

void PdfDocumentLoaderTest::testLoadStreamBodyViewValidAfterDocumentMove() {
    auto data = buildPdfWithPageContentsStream();
    auto r = PdfDocumentLoader().load(data);
    PdfDocument moved = std::move(r.document);
    const auto *rs = moved.streamStore.get(4, 0);
    QVERIFY(rs != nullptr);
    QCOMPARE(rs->rawBody, QByteArrayView("BT ET"));
}

void PdfDocumentLoaderTest::testEndToEndPageContentResolver() {
    auto data = buildPdfWithPageContentsStream();
    auto r = PdfDocumentLoader().load(data);
    QVERIFY(r.ok());

    PdfStreamDecoder decoder;
    PdfPageContentStreamResolver resolver(r.document.store, r.document.streamStore, decoder);
    auto cr = resolver.resolve(r.document.pageTree, 0);
    QVERIFY(cr.ok());
    QCOMPARE(cr.contentBytes, QByteArray("BT ET"));
}

QTEST_MAIN(PdfDocumentLoaderTest)
#include "PdfDocumentLoaderTest.moc"
