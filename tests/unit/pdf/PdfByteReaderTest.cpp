#include <QtTest/QTest>
#include "PdfByteReader.h"

class PdfByteReaderTest : public QObject {
    Q_OBJECT

private slots:
    void testReadByte_returnsByteAndAdvances();
    void testReadByte_atEof_returnsNullopt();
    void testReadByte_emptyInput();
    void testPeekByte_returnsByteWithoutAdvancing();
    void testPeekByte_atEof_returnsNullopt();
    void testSkipBytes_movesPosition();
    void testSkipBytes_beyondEof_clamps();
    void testSkipBytes_negative_isNoop();
    void testSkipBytes_zero_isNoop();
    void testSeek_randomAccess();
    void testSeek_beyondEof_clamps();
    void testSeek_negative_clamps();
    void testSeek_zero_goesToStart();
    void testReadBytes_returnsSlice();
    void testReadBytes_insufficientData_returnsNullopt();
    void testReadBytes_zero_returnsEmpty();
    void testReadBytes_negative_returnsNullopt();
    void testReadBytes_doesNotAdvanceOnFailure();
    void testReadLine_untilNewline();
    void testReadLine_crlf();
    void testReadLine_crOnly();
    void testReadLine_noLineEnding();
    void testReadLine_emptyInput();
    void testReadLine_atEof();
    void testViewAt_returnsSlice();
    void testViewAt_beyondData_returnsEmpty();
    void testViewAt_negativePos_returnsEmpty();
    void testViewAt_negativeCount_returnsEmpty();
    void testViewAt_zeroCount_returnsEmpty();
    void testViewAt_doesNotMutatePosition();
    void testEof_initiallyFalse();
    void testEof_afterReadingAll();
    void testEof_emptyInput_isTrue();
    void testRemaining_returnsLeftBytes();
    void testRemaining_emptyInput();
    void testSkipAsciiWhitespace_skipsOnlyWhitespace();
    void testSkipAsciiWhitespace_skipsNullAndFormFeed();
    void testSkipAsciiWhitespace_atEof();
    void testSkipWhitespaceAndComments_skipsPercentComments();
    void testTell_returnsPositionType();
    void testRemaining_typeIsQsizetype();
    void testDataView_returnsFullData();
    void testNonOwningView_lifetime();
};

void PdfByteReaderTest::testReadByte_returnsByteAndAdvances() {
    QByteArray data = "ABC";
    PdfByteReader reader(data);
    QCOMPARE(reader.tell(), qsizetype(0));
    auto b = reader.readByte();
    QVERIFY(b.has_value());
    QCOMPARE(b.value(), uint8_t('A'));
    QCOMPARE(reader.tell(), qsizetype(1));
}

void PdfByteReaderTest::testReadByte_atEof_returnsNullopt() {
    QByteArray data = "A";
    PdfByteReader reader(data);
    reader.readByte();
    QCOMPARE(reader.eof(), true);
    auto b = reader.readByte();
    QVERIFY(!b.has_value());
}

void PdfByteReaderTest::testReadByte_emptyInput() {
    QByteArray data;
    PdfByteReader reader(data);
    QCOMPARE(reader.eof(), true);
    auto b = reader.readByte();
    QVERIFY(!b.has_value());
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testPeekByte_returnsByteWithoutAdvancing() {
    QByteArray data = "X";
    PdfByteReader reader(data);
    auto b = reader.peekByte();
    QVERIFY(b.has_value());
    QCOMPARE(b.value(), uint8_t('X'));
    QCOMPARE(reader.tell(), qsizetype(0));
    auto b2 = reader.peekByte();
    QVERIFY(b2.has_value());
    QCOMPARE(b2.value(), uint8_t('X'));
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testPeekByte_atEof_returnsNullopt() {
    QByteArray data;
    PdfByteReader reader(data);
    QVERIFY(!reader.peekByte().has_value());
}

void PdfByteReaderTest::testSkipBytes_movesPosition() {
    QByteArray data = "ABCDE";
    PdfByteReader reader(data);
    reader.skipBytes(3);
    QCOMPARE(reader.tell(), qsizetype(3));
    auto b = reader.readByte();
    QVERIFY(b.has_value());
    QCOMPARE(b.value(), uint8_t('D'));
}

void PdfByteReaderTest::testSkipBytes_beyondEof_clamps() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    reader.skipBytes(100);
    QCOMPARE(reader.tell(), qsizetype(2));
    QCOMPARE(reader.eof(), true);
}

void PdfByteReaderTest::testSkipBytes_negative_isNoop() {
    QByteArray data = "ABC";
    PdfByteReader reader(data);
    reader.skipBytes(1);
    QCOMPARE(reader.tell(), qsizetype(1));
    reader.skipBytes(-5);
    QCOMPARE(reader.tell(), qsizetype(1));
}

void PdfByteReaderTest::testSkipBytes_zero_isNoop() {
    QByteArray data = "ABC";
    PdfByteReader reader(data);
    reader.skipBytes(0);
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testSeek_randomAccess() {
    QByteArray data = "ABCDE";
    PdfByteReader reader(data);
    reader.seek(2);
    QCOMPARE(reader.tell(), qsizetype(2));
    auto b = reader.readByte();
    QVERIFY(b.has_value());
    QCOMPARE(b.value(), uint8_t('C'));
}

void PdfByteReaderTest::testSeek_beyondEof_clamps() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    reader.seek(999);
    QCOMPARE(reader.tell(), qsizetype(2));
    QCOMPARE(reader.eof(), true);
}

void PdfByteReaderTest::testSeek_negative_clamps() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    reader.seek(1);
    reader.seek(-10);
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testSeek_zero_goesToStart() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    reader.seek(1);
    reader.seek(0);
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testReadBytes_returnsSlice() {
    QByteArray data = "ABCDE";
    PdfByteReader reader(data);
    auto result = reader.readBytes(3);
    QVERIFY(result.has_value());
    QCOMPARE(result.value().size(), qsizetype(3));
    QCOMPARE(result.value(), QByteArrayView("ABC"));
    QCOMPARE(reader.tell(), qsizetype(3));
}

void PdfByteReaderTest::testReadBytes_insufficientData_returnsNullopt() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    auto result = reader.readBytes(5);
    QVERIFY(!result.has_value());
}

void PdfByteReaderTest::testReadBytes_zero_returnsEmpty() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    auto result = reader.readBytes(0);
    QVERIFY(result.has_value());
    QCOMPARE(result.value().size(), qsizetype(0));
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testReadBytes_negative_returnsNullopt() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    auto result = reader.readBytes(-3);
    QVERIFY(!result.has_value());
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testReadBytes_doesNotAdvanceOnFailure() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    QCOMPARE(reader.tell(), qsizetype(0));
    auto result = reader.readBytes(10);
    QVERIFY(!result.has_value());
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testReadLine_untilNewline() {
    QByteArray data = "hello\nworld";
    PdfByteReader reader(data);
    auto line = reader.readLine();
    QCOMPARE(line, QByteArrayView("hello"));
    QCOMPARE(reader.tell(), qsizetype(6));
}

void PdfByteReaderTest::testReadLine_crlf() {
    QByteArray data = "hello\r\nworld";
    PdfByteReader reader(data);
    auto line = reader.readLine();
    QCOMPARE(line, QByteArrayView("hello"));
    QCOMPARE(reader.tell(), qsizetype(7));
}

void PdfByteReaderTest::testReadLine_crOnly() {
    QByteArray data = "hello\rworld";
    PdfByteReader reader(data);
    auto line = reader.readLine();
    QCOMPARE(line, QByteArrayView("hello"));
    QCOMPARE(reader.tell(), qsizetype(6));
}

void PdfByteReaderTest::testReadLine_noLineEnding() {
    QByteArray data = "hello";
    PdfByteReader reader(data);
    auto line = reader.readLine();
    QCOMPARE(line, QByteArrayView("hello"));
    QCOMPARE(reader.tell(), qsizetype(5));
    QCOMPARE(reader.eof(), true);
}

void PdfByteReaderTest::testReadLine_emptyInput() {
    QByteArray data;
    PdfByteReader reader(data);
    auto line = reader.readLine();
    QCOMPARE(line.size(), qsizetype(0));
}

void PdfByteReaderTest::testReadLine_atEof() {
    QByteArray data = "A";
    PdfByteReader reader(data);
    reader.readByte();
    auto line = reader.readLine();
    QCOMPARE(line.size(), qsizetype(0));
}

void PdfByteReaderTest::testViewAt_returnsSlice() {
    QByteArray data = "ABCDE";
    PdfByteReader reader(data);
    auto view = reader.viewAt(2, 2);
    QCOMPARE(view, QByteArrayView("CD"));
    QCOMPARE(reader.tell(), qsizetype(0));
}

void PdfByteReaderTest::testViewAt_beyondData_returnsEmpty() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    auto view = reader.viewAt(0, 10);
    QCOMPARE(view.size(), qsizetype(0));
}

void PdfByteReaderTest::testViewAt_negativePos_returnsEmpty() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    auto view = reader.viewAt(-1, 1);
    QCOMPARE(view.size(), qsizetype(0));
}

void PdfByteReaderTest::testViewAt_negativeCount_returnsEmpty() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    auto view = reader.viewAt(0, -1);
    QCOMPARE(view.size(), qsizetype(0));
}

void PdfByteReaderTest::testViewAt_zeroCount_returnsEmpty() {
    QByteArray data = "AB";
    PdfByteReader reader(data);
    auto view = reader.viewAt(0, 0);
    QCOMPARE(view.size(), qsizetype(0));
}

void PdfByteReaderTest::testViewAt_doesNotMutatePosition() {
    QByteArray data = "ABCDE";
    PdfByteReader reader(data);
    reader.seek(1);
    Q_UNUSED(reader.viewAt(3, 1));
    QCOMPARE(reader.tell(), qsizetype(1));
}

void PdfByteReaderTest::testEof_initiallyFalse() {
    QByteArray data = "A";
    PdfByteReader reader(data);
    QCOMPARE(reader.eof(), false);
}

void PdfByteReaderTest::testEof_afterReadingAll() {
    QByteArray data = "A";
    PdfByteReader reader(data);
    reader.readByte();
    QCOMPARE(reader.eof(), true);
}

void PdfByteReaderTest::testEof_emptyInput_isTrue() {
    QByteArray data;
    PdfByteReader reader(data);
    QCOMPARE(reader.eof(), true);
}

void PdfByteReaderTest::testRemaining_returnsLeftBytes() {
    QByteArray data = "ABCDE";
    PdfByteReader reader(data);
    QCOMPARE(reader.remaining(), qsizetype(5));
    reader.skipBytes(2);
    QCOMPARE(reader.remaining(), qsizetype(3));
}

void PdfByteReaderTest::testRemaining_emptyInput() {
    QByteArray data;
    PdfByteReader reader(data);
    QCOMPARE(reader.remaining(), qsizetype(0));
}

void PdfByteReaderTest::testSkipAsciiWhitespace_skipsOnlyWhitespace() {
    QByteArray data = " \t\r\n%not-comment";
    PdfByteReader reader(data);
    reader.skipAsciiWhitespace();
    QCOMPARE(reader.tell(), qsizetype(4));
    auto b = reader.peekByte();
    QVERIFY(b.has_value());
    QCOMPARE(b.value(), uint8_t('%'));
}

void PdfByteReaderTest::testSkipAsciiWhitespace_skipsNullAndFormFeed() {
    QByteArray data(3, '\0');
    data[1] = '\x0C';
    data[2] = 'X';
    PdfByteReader reader(data);
    reader.skipAsciiWhitespace();
    QCOMPARE(reader.tell(), qsizetype(2));
    auto b = reader.peekByte();
    QVERIFY(b.has_value());
    QCOMPARE(b.value(), uint8_t('X'));
}

void PdfByteReaderTest::testSkipAsciiWhitespace_atEof() {
    QByteArray data = "  ";
    PdfByteReader reader(data);
    reader.skipAsciiWhitespace();
    QCOMPARE(reader.eof(), true);
}

void PdfByteReaderTest::testSkipWhitespaceAndComments_skipsPercentComments() {
    QByteArray data = "%comment\nhello";
    PdfByteReader reader(data);
    reader.skipWhitespaceAndComments();
    QCOMPARE(reader.tell(), qsizetype(9));
    auto b = reader.peekByte();
    QVERIFY(b.has_value());
    QCOMPARE(b.value(), uint8_t('h'));
}

void PdfByteReaderTest::testTell_returnsPositionType() {
    QByteArray data = "ABC";
    PdfByteReader reader(data);
    qsizetype pos = reader.tell();
    QCOMPARE(pos, qsizetype(0));
    static_assert(std::is_same_v<decltype(pos), qsizetype>, "tell() must return qsizetype");
}

void PdfByteReaderTest::testRemaining_typeIsQsizetype() {
    QByteArray data = "ABC";
    PdfByteReader reader(data);
    qsizetype rem = reader.remaining();
    static_assert(std::is_same_v<decltype(rem), qsizetype>, "remaining() must return qsizetype");
}

void PdfByteReaderTest::testDataView_returnsFullData() {
    QByteArray data = "ABCDE";
    PdfByteReader reader(data);
    QCOMPARE(reader.dataView(), QByteArrayView(data));
}

void PdfByteReaderTest::testNonOwningView_lifetime() {
    QByteArray data = "ABC";
    PdfByteReader reader(data);
    auto view = reader.dataView();
    QCOMPARE(view.size(), qsizetype(3));
    QCOMPARE(view, QByteArrayView("ABC"));
}

QTEST_MAIN(PdfByteReaderTest)
#include "PdfByteReaderTest.moc"
