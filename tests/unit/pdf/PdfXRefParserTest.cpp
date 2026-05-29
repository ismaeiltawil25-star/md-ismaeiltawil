#include <QtTest/QTest>
#include "PdfByteReader.h"
#include "PdfXRefParser.h"

class PdfXRefParserTest : public QObject {
    Q_OBJECT

private slots:
    void testParseSingleSubsection();
    void testParseMultipleSubsections();
    void testParseFreeEntry();
    void testParseMixedInUseFree();
    void testParseLargeOffset();
    void testParseEmptySubsection();
    void testParseTrailerOffset();
    void testParseReaderAtTrailer();
    void testParseStartxrefStored();
    void testParseInvalidStartxrefNegative();
    void testParseInvalidStartxrefBeyondSize();
    void testParseMissingXrefKeyword();
    void testParseMalformedEntryBadOffset();
    void testParseMalformedEntryBadGen();
    void testParseMalformedEntryBadType();
    void testParseMalformedEntryTrailingSpaces();
    void testParseMalformedEntryShortLine();
    void testParseMalformedEntryLeadingSpace();
    void testParseMalformedEntryBlankLine();
    void testParseInvalidSubsectionHeaderNegativeStart();
    void testParseInvalidSubsectionHeaderNegativeCount();
    void testParseInvalidSubsectionHeaderSingleField();
    void testParseInvalidSubsectionHeaderThreeFields();
    void testParseInvalidSubsectionHeaderNonInteger();
    void testParseInvalidSubsectionHeaderOverflow();
    void testParseUnexpectedTokenTrailerLike();
    void testParseTruncatedEntry();
    void testParseLineEndingLF();
    void testParseLineEndingCRLF();
    void testParseLineEndingCR();
    void testParseNonZeroStartxref();
    void testParseNonZeroReaderStart();
    void testParseReaderPreservedOnInvalidStartxref();
};

void PdfXRefParserTest::testParseSingleSubsection() {
    QByteArray data("xref\n0 1\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.table.subsections.size(), 1);
    QCOMPARE(result.table.subsections[0].startObjNum, 0);
    QCOMPARE(result.table.subsections[0].count, 1);
    QCOMPARE(result.table.subsections[0].entries.size(), 1);
    QCOMPARE(result.table.subsections[0].entries[0].offset, qsizetype(9));
    QCOMPARE(result.table.subsections[0].entries[0].genNum, 0);
    QCOMPARE(result.table.subsections[0].entries[0].type, PdfXRefEntryType::InUse);
}

void PdfXRefParserTest::testParseMultipleSubsections() {
    QByteArray data("xref\n0 1\n0000000000 65535 f\n3 1\n0000000058 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.table.subsections.size(), 2);
    QCOMPARE(result.table.subsections[0].startObjNum, 0);
    QCOMPARE(result.table.subsections[1].startObjNum, 3);
    QCOMPARE(result.table.subsections[1].entries[0].offset, qsizetype(58));
}

void PdfXRefParserTest::testParseFreeEntry() {
    QByteArray data("xref\n0 1\n0000000000 65535 f\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    auto &e = result.table.subsections[0].entries[0];
    QCOMPARE(e.type, PdfXRefEntryType::Free);
    QCOMPARE(e.genNum, 65535);
}

void PdfXRefParserTest::testParseMixedInUseFree() {
    QByteArray data("xref\n0 2\n0000000000 65535 f\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.table.subsections[0].entries.size(), 2);
    QCOMPARE(result.table.subsections[0].entries[0].type, PdfXRefEntryType::Free);
    QCOMPARE(result.table.subsections[0].entries[1].type, PdfXRefEntryType::InUse);
}

void PdfXRefParserTest::testParseLargeOffset() {
    QByteArray data("xref\n0 1\n9999999999 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.table.subsections[0].entries[0].offset, qsizetype(9999999999LL));
}

void PdfXRefParserTest::testParseEmptySubsection() {
    QByteArray data("xref\n0 0\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.table.subsections.size(), 1);
    QCOMPARE(result.table.subsections[0].count, 0);
    QCOMPARE(result.table.subsections[0].entries.size(), 0);
}

void PdfXRefParserTest::testParseTrailerOffset() {
    QByteArray data("xref\n0 1\n0000000009 00000 n\ntrailer\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QVERIFY(result.trailerOffset >= 0);
}

void PdfXRefParserTest::testParseReaderAtTrailer() {
    QByteArray data("xref\n0 1\n0000000009 00000 n\ntrailer\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QVERIFY(result.trailerOffset == reader.tell());
    QCOMPARE(static_cast<char>(*reader.peekByte()), 't');
}

void PdfXRefParserTest::testParseStartxrefStored() {
    QByteArray data = QByteArray(42, 'x') + QByteArray("xref\n0 1\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 42);
    QVERIFY(result.ok());
    QCOMPARE(result.table.startxref, qsizetype(42));
}

void PdfXRefParserTest::testParseInvalidStartxrefNegative() {
    QByteArray data("xref\n0 1\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    reader.seek(5);
    PdfXRefParser parser;
    auto result = parser.parse(reader, -1);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidStartxref);
    QCOMPARE(reader.tell(), qsizetype(5));
}

void PdfXRefParserTest::testParseInvalidStartxrefBeyondSize() {
    QByteArray data("xref\n0 1\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    qsizetype beyond = data.size() + 100;
    PdfXRefParser parser;
    auto result = parser.parse(reader, beyond);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidStartxref);
}

void PdfXRefParserTest::testParseMissingXrefKeyword() {
    QByteArray data("noref\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::MissingXrefKeyword);
}

void PdfXRefParserTest::testParseMalformedEntryBadOffset() {
    QByteArray data("xref\n0 1\nXXXXXXXXXX 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::MalformedEntry);
}

void PdfXRefParserTest::testParseMalformedEntryBadGen() {
    QByteArray data("xref\n0 1\n0000000009 YYYYY n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::MalformedEntry);
}

void PdfXRefParserTest::testParseMalformedEntryBadType() {
    QByteArray data("xref\n0 1\n0000000009 00000 x\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::MalformedEntry);
}

void PdfXRefParserTest::testParseMalformedEntryTrailingSpaces() {
    QByteArray data("xref\n0 1\n0000000009 00000 n   \n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::MalformedEntry);
}

void PdfXRefParserTest::testParseMalformedEntryShortLine() {
    QByteArray data("xref\n0 1\n0000000009 00000\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::MalformedEntry);
}

void PdfXRefParserTest::testParseMalformedEntryLeadingSpace() {
    QByteArray data("xref\n0 1\n 0000000009 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::MalformedEntry);
}

void PdfXRefParserTest::testParseMalformedEntryBlankLine() {
    QByteArray data("xref\n0 1\n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::MalformedEntry);
}

void PdfXRefParserTest::testParseInvalidSubsectionHeaderNegativeStart() {
    QByteArray data("xref\n-1 3\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidSubsectionHeader);
}

void PdfXRefParserTest::testParseInvalidSubsectionHeaderNegativeCount() {
    QByteArray data("xref\n0 -1\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidSubsectionHeader);
}

void PdfXRefParserTest::testParseInvalidSubsectionHeaderSingleField() {
    QByteArray data("xref\n0\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidSubsectionHeader);
}

void PdfXRefParserTest::testParseInvalidSubsectionHeaderThreeFields() {
    QByteArray data("xref\n0 1 extra\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidSubsectionHeader);
}

void PdfXRefParserTest::testParseInvalidSubsectionHeaderNonInteger() {
    QByteArray data("xref\n/Name 1\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidSubsectionHeader);
}

void PdfXRefParserTest::testParseInvalidSubsectionHeaderOverflow() {
    QByteArray data("xref\n2147483647 1\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidSubsectionHeader);
}

void PdfXRefParserTest::testParseUnexpectedTokenTrailerLike() {
    QByteArray data("xref\n0 1\n0000000009 00000 n\ntrash\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::UnexpectedToken);
}

void PdfXRefParserTest::testParseTruncatedEntry() {
    QByteArray data("xref\n0 2\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QCOMPARE(result.warning, PdfXRefParserWarning::TruncatedEntry);
}

void PdfXRefParserTest::testParseLineEndingLF() {
    QByteArray data;
    data += "xref";
    data += '\x0a';
    data += "0 1";
    data += '\x0a';
    data += "0000000009 00000 n";
    data += '\x0a';
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.table.subsections[0].entries[0].offset, qsizetype(9));
}

void PdfXRefParserTest::testParseLineEndingCRLF() {
    QByteArray data;
    data += "xref";
    data += '\x0d';
    data += '\x0a';
    data += "0 1";
    data += '\x0d';
    data += '\x0a';
    data += "0000000009 00000 n";
    data += '\x0d';
    data += '\x0a';
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.table.subsections[0].entries[0].offset, qsizetype(9));
}

void PdfXRefParserTest::testParseLineEndingCR() {
    QByteArray data;
    data += "xref";
    data += '\x0d';
    data += "0 1";
    data += '\x0d';
    data += "0000000009 00000 n";
    data += '\x0d';
    PdfByteReader reader(data);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.table.subsections[0].entries[0].offset, qsizetype(9));
}

void PdfXRefParserTest::testParseNonZeroStartxref() {
    QByteArray data = QByteArray("xxx-padding-before-") + QByteArray("xref\n0 1\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    PdfXRefParser parser;
    qsizetype prefixLen = 19; // "xxx-padding-before-" is 19 bytes
    auto result = parser.parse(reader, prefixLen);
    QVERIFY(result.ok());
    QCOMPARE(result.table.startxref, prefixLen);
    QCOMPARE(result.table.subsections[0].entries[0].offset, qsizetype(9));
}

void PdfXRefParserTest::testParseNonZeroReaderStart() {
    QByteArray data = QByteArray(50, 'x') + QByteArray("xref\n0 1\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    reader.seek(0);
    PdfXRefParser parser;
    auto result = parser.parse(reader, 50);
    QVERIFY(result.ok());
    QCOMPARE(result.table.startxref, qsizetype(50));
}

void PdfXRefParserTest::testParseReaderPreservedOnInvalidStartxref() {
    QByteArray data("xref\n0 1\n0000000009 00000 n\n");
    PdfByteReader reader(data);
    reader.seek(3);
    PdfXRefParser parser;
    auto result = parser.parse(reader, -1);
    QCOMPARE(result.warning, PdfXRefParserWarning::InvalidStartxref);
    QCOMPARE(reader.tell(), qsizetype(3));
}

QTEST_MAIN(PdfXRefParserTest)
#include "PdfXRefParserTest.moc"
