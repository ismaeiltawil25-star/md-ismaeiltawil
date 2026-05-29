#include <QtTest/QTest>
#include "PdfMatrix.h"

class PdfMatrixTest : public QObject {
    Q_OBJECT

private slots:
    void testIdentity();
    void testIdentityHasZeroTranslation();
    void testMultiplyIdentity();
    void testTranslation();
    void testFromCm();
    void testCmConcat();
    void testNearlyEquals();
};

void PdfMatrixTest::testIdentity() {
    auto m = PdfMatrix::identity();
    QCOMPARE(m.a, 1.0);
    QCOMPARE(m.b, 0.0);
    QCOMPARE(m.c, 0.0);
    QCOMPARE(m.d, 1.0);
    QCOMPARE(m.e, 0.0);
    QCOMPARE(m.f, 0.0);
}

void PdfMatrixTest::testIdentityHasZeroTranslation() {
    auto m = PdfMatrix::identity();
    QCOMPARE(m.e, 0.0);
    QCOMPARE(m.f, 0.0);
}

void PdfMatrixTest::testMultiplyIdentity() {
    auto id = PdfMatrix::identity();
    auto m = PdfMatrix::fromCm(2, 0, 0, 3, 10, 20);
    QVERIFY((id * m).nearlyEquals(m));
    QVERIFY((m * id).nearlyEquals(m));
}

void PdfMatrixTest::testTranslation() {
    auto t1 = PdfMatrix::translation(10, 20);
    auto t2 = PdfMatrix::translation(5, -3);
    auto combined = t1 * t2;
    auto expected = PdfMatrix::translation(15, 17);
    QVERIFY(combined.nearlyEquals(expected));
}

void PdfMatrixTest::testFromCm() {
    auto m = PdfMatrix::fromCm(1, 2, 3, 4, 5, 6);
    QCOMPARE(m.a, 1.0); QCOMPARE(m.b, 2.0);
    QCOMPARE(m.c, 3.0); QCOMPARE(m.d, 4.0);
    QCOMPARE(m.e, 5.0); QCOMPARE(m.f, 6.0);
}

void PdfMatrixTest::testCmConcat() {
    auto id = PdfMatrix::identity();
    auto scale = PdfMatrix::fromCm(2, 0, 0, 2, 0, 0);
    auto trans = PdfMatrix::translation(10, 20);
    auto composite = id * scale * trans;
    QVERIFY(composite.nearlyEquals(PdfMatrix::fromCm(2, 0, 0, 2, 10, 20)));
}

void PdfMatrixTest::testNearlyEquals() {
    auto a = PdfMatrix::fromCm(1, 2, 3, 4, 5, 6);
    auto b = PdfMatrix::fromCm(1 + 1e-10, 2, 3, 4, 5, 6);
    auto c = PdfMatrix::fromCm(1 + 1e-3, 2, 3, 4, 5, 6);
    QVERIFY(a.nearlyEquals(b));
    QVERIFY(!a.nearlyEquals(c));
    QVERIFY(a.nearlyEquals(b, 1e-7));
}

QTEST_MAIN(PdfMatrixTest)
#include "PdfMatrixTest.moc"
