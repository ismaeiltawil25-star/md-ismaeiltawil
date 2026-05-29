#include <QtTest/QTest>
#include "PdfByteReader.h"
#include "PdfStreamBodyReader.h"

class PdfStreamBodyReaderTest : public QObject {
    Q_OBJECT

private slots:
    // Success paths
    void testReadBodySimple();
    void testReadBodyCrLf();
    void testReadBodyCrOnly();
    void testReadBodyNoPreEndstreamEol();
    void testReadBodyZeroLength();
    void testReadBodyTrailingNewlineAfterEndstream();
    void testReadBodyLargeLength();
    void testReadBodyEndstreamFollowedByDelimiter();
    void testReadBodyEndstreamFollowedByEof();

    // InvalidStreamOffset
    void testReadBodyInvalidOffsetNegative();
    void testReadBodyInvalidOffsetBeyondSize();
    void testReadBodyInvalidOffsetEqualToSize();
    void testReadBodyEmptyDataOffsetZero();

    // InvalidLength
    void testReadBodyInvalidLengthNegative();
    void testReadBodyInvalidLengthNegativeReaderUnchanged();

    // MissingStreamKeyword
    void testReadBodyMissingStreamKeyword();
    void testReadBodyMissingStreamKeywordWrongCase();

    // MissingStreamEol
    void testReadBodyMissingStreamEolEof();
    void testReadBodyMissingStreamEolNonEol();

    // DataTooShort
    void testReadBodyDataTooShortFewerBytesThanLength();
    void testReadBodyDataTooShortEmptyAfterStream();

    // Length exceeding logical body but within buffer extent
    void testReadBodyLengthExceedsBodyButEndstreamPresent();

    // MissingEndstream
    void testReadBodyMissingEndstream();
    void testReadBodyMissingEndstreamAfterEol();
    void testReadBodyMissingEndstreamTruncated();

    // InvalidEndstreamBoundary
    void testReadBodyInvalidEndstreamBoundarySpace();
    void testReadBodyInvalidEndstreamBoundaryTab();
    void testReadBodyInvalidEndstreamBoundaryDoubleEol();
    void testReadBodyInvalidEndstreamBoundaryGarbage();
    void testReadBodyInvalidEndstreamBoundaryWrong9Bytes();
    void testReadBodyInvalidEndstreamTrailingRegularChar();

    // Reader-position verification
    void testReadBodyReaderUnchangedOnInvalidOffset();
    void testReadBodyReaderRestoredOnMissingStreamEol();
    void testReadBodyReaderRestoredOnDataTooShort();

    // Non-zero stream offset
    void testReadBodyStreamAtNonZeroOffset();

    // Body containing literal keywords
    void testReadBodyBodyContainsStreamKeyword();
    void testReadBodyBodyContainsEndstreamKeyword();
    void testReadBodyBodyContainsCrLf();

    // Zero-length body, no endstream
    void testReadBodyZeroLengthMissingEndstream();
};

// Success paths

void PdfStreamBodyReaderTest::testReadBodySimple() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyCrLf() {
    QByteArray data("stream\r\nAAAAA\r\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyCrOnly() {
    QByteArray data("stream\rAAAAA\rendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyNoPreEndstreamEol() {
    QByteArray data("stream\nAAAAAendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyZeroLength() {
    QByteArray data("stream\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 0);
    QVERIFY(result.ok());
    QVERIFY(result.body.isEmpty());
}

void PdfStreamBodyReaderTest::testReadBodyTrailingNewlineAfterEndstream() {
    QByteArray data("stream\nAAAAA\nendstream\n");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyLargeLength() {
    QByteArray bodyData(2000, 'X');
    QByteArray data = QByteArray("stream\n") + bodyData + QByteArray("\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 2000);
    QVERIFY(result.ok());
    QCOMPARE(result.body.size(), qsizetype(2000));
}

void PdfStreamBodyReaderTest::testReadBodyEndstreamFollowedByDelimiter() {
    QByteArray data("stream\nAAAAA\nendstream/");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyEndstreamFollowedByEof() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

// InvalidStreamOffset

void PdfStreamBodyReaderTest::testReadBodyInvalidOffsetNegative() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, -1, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidStreamOffset);
}

void PdfStreamBodyReaderTest::testReadBodyInvalidOffsetBeyondSize() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, data.size() + 10, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidStreamOffset);
}

void PdfStreamBodyReaderTest::testReadBodyInvalidOffsetEqualToSize() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, data.size(), 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidStreamOffset);
}

void PdfStreamBodyReaderTest::testReadBodyEmptyDataOffsetZero() {
    QByteArray data("");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidStreamOffset);
}

// InvalidLength

void PdfStreamBodyReaderTest::testReadBodyInvalidLengthNegative() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, -1);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidLength);
}

void PdfStreamBodyReaderTest::testReadBodyInvalidLengthNegativeReaderUnchanged() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    reader.seek(5);
    qsizetype posBefore = reader.tell();
    auto result = PdfStreamBodyReader::readBody(reader, 0, -1);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidLength);
    QCOMPARE(reader.tell(), posBefore);
}

// MissingStreamKeyword

void PdfStreamBodyReaderTest::testReadBodyMissingStreamKeyword() {
    QByteArray data("hello\nbody\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingStreamKeyword);
}

void PdfStreamBodyReaderTest::testReadBodyMissingStreamKeywordWrongCase() {
    QByteArray data("streaM\nbody\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingStreamKeyword);
}

// MissingStreamEol

void PdfStreamBodyReaderTest::testReadBodyMissingStreamEolEof() {
    QByteArray data("stream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingStreamEol);
}

void PdfStreamBodyReaderTest::testReadBodyMissingStreamEolNonEol() {
    QByteArray data("streamXmore bytes here");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingStreamEol);
}

// DataTooShort

void PdfStreamBodyReaderTest::testReadBodyDataTooShortFewerBytesThanLength() {
    QByteArray data("stream\nAAA");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::DataTooShort);
}

void PdfStreamBodyReaderTest::testReadBodyDataTooShortEmptyAfterStream() {
    QByteArray data("stream\n");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 1);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::DataTooShort);
}

// Length exceeding logical body but within buffer extent

void PdfStreamBodyReaderTest::testReadBodyLengthExceedsBodyButEndstreamPresent() {
    QByteArray data("stream\nAAAAA\nendstreamBBBBBBBBB");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 10);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidEndstreamBoundary);
    QCOMPARE(result.body, QByteArrayView("AAAAA\nends"));
}

// MissingEndstream

void PdfStreamBodyReaderTest::testReadBodyMissingEndstream() {
    QByteArray data("stream\nAAAAA");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingEndstream);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyMissingEndstreamAfterEol() {
    QByteArray data("stream\nAAAAA\n");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingEndstream);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyMissingEndstreamTruncated() {
    QByteArray data("stream\nAAAAA\nendstre");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingEndstream);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

// InvalidEndstreamBoundary

void PdfStreamBodyReaderTest::testReadBodyInvalidEndstreamBoundarySpace() {
    QByteArray data("stream\nAAAAA endstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidEndstreamBoundary);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyInvalidEndstreamBoundaryTab() {
    QByteArray data("stream\nAAAAA\tendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidEndstreamBoundary);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyInvalidEndstreamBoundaryDoubleEol() {
    QByteArray data("stream\nAAAAA\n\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidEndstreamBoundary);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyInvalidEndstreamBoundaryGarbage() {
    QByteArray data("stream\nAAAAA\nXYZendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidEndstreamBoundary);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyInvalidEndstreamBoundaryWrong9Bytes() {
    QByteArray data("stream\nAAAAA\nendstreaX");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidEndstreamBoundary);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

void PdfStreamBodyReaderTest::testReadBodyInvalidEndstreamTrailingRegularChar() {
    QByteArray data("stream\nAAAAA\nendstreamX");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::InvalidEndstreamBoundary);
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

// Reader-position verification

void PdfStreamBodyReaderTest::testReadBodyReaderUnchangedOnInvalidOffset() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    reader.seek(3);
    qsizetype posBefore = reader.tell();
    auto result = PdfStreamBodyReader::readBody(reader, -1, 5);
    QVERIFY(!result.ok());
    QCOMPARE(reader.tell(), posBefore);
}

void PdfStreamBodyReaderTest::testReadBodyReaderRestoredOnMissingStreamEol() {
    QByteArray data("streamXbody");
    PdfByteReader reader(data);
    reader.seek(0);
    qsizetype posBefore = reader.tell();
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingStreamEol);
    QCOMPARE(reader.tell(), posBefore);
}

void PdfStreamBodyReaderTest::testReadBodyReaderRestoredOnDataTooShort() {
    QByteArray data("stream\nAA");
    PdfByteReader reader(data);
    reader.seek(0);
    qsizetype posBefore = reader.tell();
    auto result = PdfStreamBodyReader::readBody(reader, 0, 5);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::DataTooShort);
    QCOMPARE(reader.tell(), posBefore);
}

// Non-zero stream offset

void PdfStreamBodyReaderTest::testReadBodyStreamAtNonZeroOffset() {
    QByteArray data("preamble junk\nstream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 14, 5);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAAAA"));
}

// Body containing literal keywords

void PdfStreamBodyReaderTest::testReadBodyBodyContainsStreamKeyword() {
    QByteArray data("stream\nAAstreamAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 11);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAstreamAA\n"));
}

void PdfStreamBodyReaderTest::testReadBodyBodyContainsEndstreamKeyword() {
    QByteArray data("stream\nAAendstreamAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 13);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AAendstreamAA"));
}

void PdfStreamBodyReaderTest::testReadBodyBodyContainsCrLf() {
    QByteArray data("stream\nAA\r\nAA\nendstream");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 7);
    QVERIFY(result.ok());
    QCOMPARE(result.body, QByteArrayView("AA\r\nAA\n"));
}

// Zero-length body, no endstream

void PdfStreamBodyReaderTest::testReadBodyZeroLengthMissingEndstream() {
    QByteArray data("stream\n");
    PdfByteReader reader(data);
    auto result = PdfStreamBodyReader::readBody(reader, 0, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamBodyReaderWarning::MissingEndstream);
    QVERIFY(result.body.isEmpty());
}

QTEST_MAIN(PdfStreamBodyReaderTest)
#include "PdfStreamBodyReaderTest.moc"
