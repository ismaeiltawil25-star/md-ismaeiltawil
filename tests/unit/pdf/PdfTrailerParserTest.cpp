#include <QtTest/QTest>
#include "PdfByteReader.h"
#include "PdfTrailerParser.h"

class PdfTrailerParserTest : public QObject {
    Q_OBJECT

private slots:
    void testParseMinimalTrailer();
    void testParseTrailerWithPrev();
    void testParseTrailerWithEncrypt();
    void testParseTrailerWithInfo();
    void testParseTrailerMultipleKeys();

    void testParseInvalidTrailerOffsetNegative();
    void testParseInvalidTrailerOffsetBeyondSize();

    void testParseMissingTrailerKeyword();
    void testParseMissingDict();

    void testParseMissingSize();
    void testParseMissingRoot();

    void testParseSizeZero();
    void testParseSizeNegative();
    void testParseSizeNotInteger();
    void testParseSizeOverflow();

    void testParseRootNotReference();
    void testParseRootZeroObj();

    void testParseIDLiteral();
    void testParseIDHex();
    void testParseIDMixed();
    void testParseIDMalformed();
    void testParseIDWrongSize();

    void testParsePrevNegative();
    void testParsePrevNotInteger();

    void testParseMalformedTrailerDict();
    void testParseEncryptWithMissingRoot();
    void testParseEncryptedTrailer();

    void testParseTrailerNonZeroOffset();
};

void PdfTrailerParserTest::testParseMinimalTrailer() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QCOMPARE(r.trailer.size, 5);
    QCOMPARE(r.trailer.root.objNum, 1);
    QCOMPARE(r.trailer.root.genNum, 0);
}

void PdfTrailerParserTest::testParseTrailerWithPrev() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /Prev 12345 >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(r.trailer.prev.has_value());
    QCOMPARE(*r.trailer.prev, qsizetype(12345));
}

void PdfTrailerParserTest::testParseTrailerWithEncrypt() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /Encrypt 10 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.trailer.encrypted);
    QCOMPARE(r.warning, PdfTrailerParserWarning::EncryptedUnsupported);
}

void PdfTrailerParserTest::testParseTrailerWithInfo() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /Info 2 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(r.trailer.rawDict != nullptr);
    QVERIFY(r.trailer.rawDict->has("Info"));
}

void PdfTrailerParserTest::testParseTrailerMultipleKeys() {
    QByteArray data("trailer << /Prev 100 /Size 7 /ID [ (a) (b) ] /Root 3 0 R /Encrypt 4 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    // Encrypt present, Size and Root valid → EncryptedUnsupported
    QCOMPARE(r.warning, PdfTrailerParserWarning::EncryptedUnsupported);
    QCOMPARE(r.trailer.size, 7);
    QCOMPARE(r.trailer.root.objNum, 3);
    QVERIFY(r.trailer.prev.has_value());
    QCOMPARE(*r.trailer.prev, qsizetype(100));
    QVERIFY(r.trailer.id.has_value());
    QCOMPARE(r.trailer.id->first, QByteArray("a"));
    QCOMPARE(r.trailer.id->second, QByteArray("b"));
}

void PdfTrailerParserTest::testParseInvalidTrailerOffsetNegative() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R >>");
    PdfByteReader reader(data);
    reader.seek(3);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, -1);
    QCOMPARE(r.warning, PdfTrailerParserWarning::InvalidTrailerOffset);
    QCOMPARE(reader.tell(), qsizetype(3));
}

void PdfTrailerParserTest::testParseInvalidTrailerOffsetBeyondSize() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, data.size() + 100);
    QCOMPARE(r.warning, PdfTrailerParserWarning::InvalidTrailerOffset);
}

void PdfTrailerParserTest::testParseMissingTrailerKeyword() {
    QByteArray data("notrailer << /Size 5 /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingTrailerKeyword);
}

void PdfTrailerParserTest::testParseMissingDict() {
    QByteArray data("trailer");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingTrailerDict);
}

void PdfTrailerParserTest::testParseMissingSize() {
    QByteArray data("trailer << /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseMissingRoot() {
    QByteArray data("trailer << /Size 5 >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseSizeZero() {
    QByteArray data("trailer << /Size 0 /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseSizeNegative() {
    QByteArray data("trailer << /Size -1 /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseSizeNotInteger() {
    QByteArray data("trailer << /Size /Wrong /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseSizeOverflow() {
    QByteArray data("trailer << /Size 999999999999 /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseRootNotReference() {
    QByteArray data("trailer << /Size 5 /Root 42 >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseRootZeroObj() {
    QByteArray data("trailer << /Size 5 /Root 0 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseIDLiteral() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /ID [ (abc) (def) ] >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(r.trailer.id.has_value());
    QCOMPARE(r.trailer.id->first, QByteArray("abc"));
    QCOMPARE(r.trailer.id->second, QByteArray("def"));
}

void PdfTrailerParserTest::testParseIDHex() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /ID [ <ABC> <DEF> ] >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(r.trailer.id.has_value());
    QByteArray expectedFirst;
    expectedFirst += '\xAB';
    expectedFirst += '\xC0';
    QByteArray expectedSecond;
    expectedSecond += '\xDE';
    expectedSecond += '\xF0';
    QCOMPARE(r.trailer.id->first, expectedFirst);
    QCOMPARE(r.trailer.id->second, expectedSecond);
}

void PdfTrailerParserTest::testParseIDMixed() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /ID [ (abc) <DEF> ] >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(r.trailer.id.has_value());
    QCOMPARE(r.trailer.id->first, QByteArray("abc"));
    QByteArray expectedSecond;
    expectedSecond += '\xDE';
    expectedSecond += '\xF0';
    QCOMPARE(r.trailer.id->second, expectedSecond);
}

void PdfTrailerParserTest::testParseIDMalformed() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /ID /NotArray >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(!r.trailer.id.has_value());
}

void PdfTrailerParserTest::testParseIDWrongSize() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /ID [ <ABC> ] >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(!r.trailer.id.has_value());
}

void PdfTrailerParserTest::testParsePrevNegative() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /Prev -1 >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(!r.trailer.prev.has_value());
}

void PdfTrailerParserTest::testParsePrevNotInteger() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /Prev /Foo >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.ok());
    QVERIFY(!r.trailer.prev.has_value());
}

void PdfTrailerParserTest::testParseMalformedTrailerDict() {
    QByteArray data("trailer << /Size 5");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MalformedTrailerDict);
}

void PdfTrailerParserTest::testParseEncryptWithMissingRoot() {
    QByteArray data("trailer << /Size 5 /Encrypt 10 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QCOMPARE(r.warning, PdfTrailerParserWarning::MissingRequiredKey);
}

void PdfTrailerParserTest::testParseEncryptedTrailer() {
    QByteArray data("trailer << /Size 5 /Root 1 0 R /Encrypt 10 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    auto r = parser.parse(reader, 0);
    QVERIFY(r.trailer.encrypted);
    QCOMPARE(r.warning, PdfTrailerParserWarning::EncryptedUnsupported);
    QCOMPARE(r.trailer.size, 5);
}

void PdfTrailerParserTest::testParseTrailerNonZeroOffset() {
    QByteArray data = QByteArray("xxx-prefix-xxx") + QByteArray("trailer << /Size 5 /Root 1 0 R >>");
    PdfByteReader reader(data);
    PdfTrailerParser parser;
    qsizetype off = 14; // "xxx-prefix-xxx" = 14 bytes
    auto r = parser.parse(reader, off);
    QVERIFY(r.ok());
    QCOMPARE(r.trailer.size, 5);
}

QTEST_MAIN(PdfTrailerParserTest)
#include "PdfTrailerParserTest.moc"
