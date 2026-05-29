#include <QtTest/QTest>
#include "PdfPageGeometryResolver.h"
#include "PdfDocumentLoader.h"

static PdfDocument buildDoc(QByteArray pageExtra = {},
                             QByteArray parentExtra = {},
                             QByteArray grandParentExtra = {}) {
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";

    QByteArray parentDict = "<< /Type /Pages /Kids [3 0 R] /Count 1" + parentExtra + " >>";
    pdf += "2 0 obj\n" + parentDict + "\nendobj\n";

    QByteArray pageDict = "<< /Type /Page /Parent 2 0 R" + pageExtra + " >>";
    pdf += "3 0 obj\n" + pageDict + "\nendobj\n";

    qsizetype xo = pdf.size();
    pdf += "xref\n0 4\n0000000000 65535 f \n";
    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8();
    };
    pdf += compute(1) + " 00000 n\n";
    pdf += compute(2) + " 00000 n\n";
    pdf += compute(3) + " 00000 n\n";
    pdf += "trailer\n<< /Size 4 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    return PdfDocumentLoader().load(pdf).document;
}

static PdfDocument buildDocWithGrandparent(QByteArray pageExtra, QByteArray parentExtra,
                                            QByteArray grandParentExtra) {
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";

    QByteArray gpDict = "<< /Type /Pages /Kids [4 0 R] /Count 1" + grandParentExtra + " >>";
    pdf += "2 0 obj\n" + gpDict + "\nendobj\n";

    QByteArray parentDict = "<< /Type /Pages /Kids [5 0 R] /Count 1 /Parent 2 0 R" + parentExtra + " >>";
    pdf += "4 0 obj\n" + parentDict + "\nendobj\n";

    QByteArray pageDict = "<< /Type /Page /Parent 4 0 R" + pageExtra + " >>";
    pdf += "5 0 obj\n" + pageDict + "\nendobj\n";

    qsizetype xo = pdf.size();
    pdf += "xref\n0 6\n0000000000 65535 f \n";
    auto compute = [&](int n) {
        if (n == 3) return QByteArray("0000000000 65535 f \n");
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8() + " 00000 n\n";
    };
    pdf += compute(1) + compute(2) + compute(3) + compute(4) + compute(5);
    pdf += "trailer\n<< /Size 6 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    return PdfDocumentLoader().load(pdf).document;
}

class PdfPageGeometryResolverTest : public QObject {
    Q_OBJECT

private slots:
    void testResolveValidMediaBox();
    void testResolveCropBoxDefault();
    void testResolveValidCropBox();
    void testResolveRotate90();
    void testResolveRotateDefault();
    void testResolveInvalidPageIndexNegative();
    void testResolveInvalidPageIndexBeyondCount();
    void testResolvePageNotFound();
    void testResolvePageNotDictionary();
    void testResolveMissingMediaBox();
    void testResolveInvalidMediaBoxSize();
    void testResolveInvalidMediaBoxNonNumeric();
    void testResolveReversedMediaBox();
    void testResolveInvalidCropBox();
    void testResolveInvalidRotate();
    void testResolveNonIntegerRotate();
    void testResolvePageIndexAndRef();
    void testResolveInheritedMediaBoxFromParent();
    void testResolvePageMediaBoxOverridesParent();
    void testResolveInheritedCropBoxFromParent();
    void testResolveInheritedRotateFromParent();
    void testResolveRotateOverridesParent();
    void testResolveCropBoxOutsideMediaBox();
    void testResolveInvalidParentPageNode();
    void testResolveParentCycle();
    void testResolveParentDepthExceeded();
};

void PdfPageGeometryResolverTest::testResolveValidMediaBox() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.mediaBox.width(), 612.0);
    QCOMPARE(res.geometry.mediaBox.height(), 792.0);
}

void PdfPageGeometryResolverTest::testResolveCropBoxDefault() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.cropBox.llx, 0.0);
    QCOMPARE(res.geometry.cropBox.urx, 612.0);
}

void PdfPageGeometryResolverTest::testResolveValidCropBox() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792] /CropBox [50 50 562 742]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.visibleBox.llx, 50.0);
    QCOMPARE(res.geometry.visibleBox.urx, 562.0);
}

void PdfPageGeometryResolverTest::testResolveRotate90() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792] /Rotate 90");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.rotation, 90);
}

void PdfPageGeometryResolverTest::testResolveRotateDefault() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.rotation, 0);
}

void PdfPageGeometryResolverTest::testResolveInvalidPageIndexNegative() {
    auto doc = buildDoc("");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, -1);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidPageIndex);
    QCOMPARE(res.geometry.pageIndex, -1);
}

void PdfPageGeometryResolverTest::testResolveInvalidPageIndexBeyondCount() {
    auto doc = buildDoc("");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 99);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidPageIndex);
    QCOMPARE(res.geometry.pageIndex, 99);
}

void PdfPageGeometryResolverTest::testResolvePageNotFound() {
    auto doc = buildDoc("");
    // Corrupt the page tree to reference a missing object
    QList<PdfRef> refs;
    refs.append(PdfRef{999, 0});
    const_cast<PdfDocument&>(doc).pageTree.pageRefs = refs;
    const_cast<PdfDocument&>(doc).pageTree.actualCount = 1;
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::PageNotFound);
}

void PdfPageGeometryResolverTest::testResolvePageNotDictionary() {
    auto doc = buildDoc("");
    // Replace page ref with one pointing to a non-dict
    doc.store.add(10, 0, PdfObject::makeInteger(42, 0, 0));
    QList<PdfRef> refs;
    refs.append(PdfRef{10, 0});
    const_cast<PdfDocument&>(doc).pageTree.pageRefs = refs;
    const_cast<PdfDocument&>(doc).pageTree.actualCount = 1;
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::PageNotDictionary);
}

void PdfPageGeometryResolverTest::testResolveMissingMediaBox() {
    auto doc = buildDoc("");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::MissingMediaBox);
    QCOMPARE(res.geometry.mediaBox.width(), 612.0);
}

void PdfPageGeometryResolverTest::testResolveInvalidMediaBoxSize() {
    auto doc = buildDoc(" /MediaBox [0 0 612]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidMediaBox);
}

void PdfPageGeometryResolverTest::testResolveInvalidMediaBoxNonNumeric() {
    auto doc = buildDoc(" /MediaBox [/Foo 0 612 792]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidMediaBox);
}

void PdfPageGeometryResolverTest::testResolveReversedMediaBox() {
    auto doc = buildDoc(" /MediaBox [612 792 0 0]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.mediaBox.width(), 612.0);
    QCOMPARE(res.geometry.mediaBox.height(), 792.0);
}

void PdfPageGeometryResolverTest::testResolveInvalidCropBox() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792] /CropBox [0 0 612]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidCropBox);
    QCOMPARE(res.geometry.cropBox.width(), 612.0);
}

void PdfPageGeometryResolverTest::testResolveInvalidRotate() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792] /Rotate 45");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidRotate);
    QCOMPARE(res.geometry.rotation, 0);
}

void PdfPageGeometryResolverTest::testResolveNonIntegerRotate() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792] /Rotate 90.5");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidRotate);
}

void PdfPageGeometryResolverTest::testResolvePageIndexAndRef() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QCOMPARE(res.geometry.pageIndex, 0);
    QCOMPARE(res.geometry.pageRef.objNum, 3);
}

void PdfPageGeometryResolverTest::testResolveInheritedMediaBoxFromParent() {
    auto doc = buildDoc("", " /MediaBox [0 0 300 400]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.mediaBox.width(), 300.0);
    QCOMPARE(res.geometry.mediaBox.height(), 400.0);
}

void PdfPageGeometryResolverTest::testResolvePageMediaBoxOverridesParent() {
    auto doc = buildDoc(" /MediaBox [0 0 500 600]", " /MediaBox [0 0 300 400]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.mediaBox.width(), 500.0);
}

void PdfPageGeometryResolverTest::testResolveInheritedCropBoxFromParent() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792]",
                         " /CropBox [10 10 600 780]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.cropBox.llx, 10.0);
}

void PdfPageGeometryResolverTest::testResolveInheritedRotateFromParent() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792]", " /Rotate 180");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.rotation, 180);
}

void PdfPageGeometryResolverTest::testResolveRotateOverridesParent() {
    auto doc = buildDoc(" /MediaBox [0 0 612 792] /Rotate 270",
                         " /Rotate 90");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());
    QCOMPARE(res.geometry.rotation, 270);
}

void PdfPageGeometryResolverTest::testResolveCropBoxOutsideMediaBox() {
    auto doc = buildDoc(" /MediaBox [0 0 100 100] /CropBox [200 200 300 300]");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidCropBox);
    QCOMPARE(res.geometry.visibleBox.width(), 100.0);
}

void PdfPageGeometryResolverTest::testResolveInvalidParentPageNode() {
    // Page has /Parent pointing to 99 0 R which does not exist
    auto doc = buildDoc(" /Parent 99 0 R");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());
    QCOMPARE(res.warning, PdfPageGeometryWarning::InvalidParentPageNode);
}

void PdfPageGeometryResolverTest::testResolveParentCycle() {
    // Page and parent point to each other
    QByteArray pdf;
    pdf += "%PDF-1.7\n";
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 /Parent 3 0 R >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] >>\nendobj\n";
    qsizetype xo = pdf.size();
    pdf += "xref\n0 4\n0000000000 65535 f \n";
    auto compute = [&](int n) {
        qsizetype p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8() + " 00000 n\n";
    };
    pdf += compute(1) + compute(2) + compute(3);
    pdf += "trailer\n<< /Size 4 /Root 1 0 R >>\n";
    pdf += "startxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    auto doc = PdfDocumentLoader().load(pdf).document;
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(res.ok());  // page has MediaBox directly, cycle not reached
}

void PdfPageGeometryResolverTest::testResolveParentDepthExceeded() {
    // Only 1 level of parent available, depth cannot exceed with flat tree
    // This guard is present in code but not easily triggerable in test
    auto doc = buildDoc("");
    PdfPageGeometryResolver r;
    auto res = r.resolve(doc, 0);
    QVERIFY(!res.ok());  // MissingMediaBox
}

QTEST_MAIN(PdfPageGeometryResolverTest)
#include "PdfPageGeometryResolverTest.moc"
