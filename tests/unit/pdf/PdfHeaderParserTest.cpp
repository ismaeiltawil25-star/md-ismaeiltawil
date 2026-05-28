#include <QtTest/QTest>
#include "PdfHeaderParser.h"

class PdfHeaderParserTest : public QObject {
    Q_OBJECT

private slots:
    void testParse_validHeaderPdf17();
    void testParse_validHeaderPdf14_crlf();
    void testParse_validHeaderPdf10();
    void testParse_validHeaderPdf17_cr();
    void testParse_leadingWhitespace_skipped();
    void testParse_emptyFile_returnsTooShort();
    void testParse_noHeader_returnsMissingMagic();
    void testParse_truncatedHeader_returnsTooShort();
    void testParse_wrongMagic_returnsMissingMagic();
    void testParse_percentNotComment_provesHeaderParsed();
    void testParse_major2_returnsUnsupportedMajorVersion();
    void testParse_minor8_returnsUnsupportedMinorVersion();
    void testParse_missingDot_returnsMissingVersionDot();
    void testParse_nonDigitMajor_returnsInvalidVersionDigits();
    void testParse_nonDigitMinor_returnsInvalidVersionDigits();
    void testParse_failureRestoresPosition();
    void testParse_successConsumesWhitespaceAndHeader();
    void testParse_pdf17_exact8bytes();
    void testParse_largeLeadingWhitespace();
};

void PdfHeaderParserTest::testParse_validHeaderPdf17() {
    QByteArray data = "%PDF-1.7\nrest";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(result.error, PdfHeaderError::None);
    QCOMPARE(result.version.major, uint8_t(1));
    QCOMPARE(result.version.minor, uint8_t(7));
    QVERIFY(result.errorMessage.isEmpty());
}

void PdfHeaderParserTest::testParse_validHeaderPdf14_crlf() {
    QByteArray data = "%PDF-1.4\r\nrest";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(result.error, PdfHeaderError::None);
    QCOMPARE(result.version.major, uint8_t(1));
    QCOMPARE(result.version.minor, uint8_t(4));
}

void PdfHeaderParserTest::testParse_validHeaderPdf10() {
    QByteArray data = "%PDF-1.0\nrest";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(result.error, PdfHeaderError::None);
    QCOMPARE(result.version.major, uint8_t(1));
    QCOMPARE(result.version.minor, uint8_t(0));
}

void PdfHeaderParserTest::testParse_validHeaderPdf17_cr() {
    QByteArray data = "%PDF-1.7\rrest";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(result.error, PdfHeaderError::None);
    QCOMPARE(result.version.major, uint8_t(1));
    QCOMPARE(result.version.minor, uint8_t(7));
}

void PdfHeaderParserTest::testParse_leadingWhitespace_skipped() {
    QByteArray data = " \t\r\n%PDF-1.7\nrest";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(result.version.major, uint8_t(1));
    QCOMPARE(result.version.minor, uint8_t(7));
}

void PdfHeaderParserTest::testParse_emptyFile_returnsTooShort() {
    QByteArray data;
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::TooShort);
}

void PdfHeaderParserTest::testParse_noHeader_returnsMissingMagic() {
    QByteArray data = "xyz\nrest";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::MissingMagic);
}

void PdfHeaderParserTest::testParse_truncatedHeader_returnsTooShort() {
    QByteArray data = "%PDF-1.";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::TooShort);
}

void PdfHeaderParserTest::testParse_wrongMagic_returnsMissingMagic() {
    QByteArray data = "notpdf-1.7\n";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::MissingMagic);
}

void PdfHeaderParserTest::testParse_percentNotComment_provesHeaderParsed() {
    QByteArray data = "%PDF-1.7";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(result.error, PdfHeaderError::None);
    QCOMPARE(result.version.major, uint8_t(1));
    QCOMPARE(result.version.minor, uint8_t(7));
}

void PdfHeaderParserTest::testParse_major2_returnsUnsupportedMajorVersion() {
    QByteArray data = "%PDF-2.0\n";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::UnsupportedMajorVersion);
}

void PdfHeaderParserTest::testParse_minor8_returnsUnsupportedMinorVersion() {
    QByteArray data = "%PDF-1.8\n";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::UnsupportedMinorVersion);
}

void PdfHeaderParserTest::testParse_missingDot_returnsMissingVersionDot() {
    QByteArray data = "%PDF-17\n";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::MissingVersionDot);
}

void PdfHeaderParserTest::testParse_nonDigitMajor_returnsInvalidVersionDigits() {
    QByteArray data = "%PDF-x.7\n";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::InvalidVersionDigits);
}

void PdfHeaderParserTest::testParse_nonDigitMinor_returnsInvalidVersionDigits() {
    QByteArray data = "%PDF-1.x\n";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(result.error, PdfHeaderError::InvalidVersionDigits);
}

void PdfHeaderParserTest::testParse_failureRestoresPosition() {
    QByteArray data = "notpdf-1.7\n";
    PdfByteReader reader(data);
    QCOMPARE(reader.tell(), qsizetype(0));
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(!result.success);
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfHeaderParserTest::testParse_successConsumesWhitespaceAndHeader() {
    QByteArray data = " %PDF-1.7\nrest";
    PdfByteReader reader(data);
    QCOMPARE(reader.tell(), qsizetype(0));
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(reader.tell(), qsizetype(9));
}

void PdfHeaderParserTest::testParse_pdf17_exact8bytes() {
    QByteArray data = "%PDF-1.7";
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(reader.tell(), qsizetype(8));
    QCOMPARE(reader.eof(), true);
}

void PdfHeaderParserTest::testParse_largeLeadingWhitespace() {
    QByteArray data(500, ' ');
    data.append("%PDF-1.7\n");
    PdfByteReader reader(data);
    auto result = PdfHeaderParser::parse(reader);
    QVERIFY(result.success);
    QCOMPARE(result.version.major, uint8_t(1));
    QCOMPARE(result.version.minor, uint8_t(7));
}

QTEST_MAIN(PdfHeaderParserTest)
#include "PdfHeaderParserTest.moc"
