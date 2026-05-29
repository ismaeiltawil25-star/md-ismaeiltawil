#include <QtTest/QTest>
#include "PdfContentStreamTokenizer.h"

class PdfContentStreamTokenizerTest : public QObject {
    Q_OBJECT

private slots:
    void testParseEmpty();
    void testParseWhitespaceOnly();
    void testParseSimpleText();
    void testParseGraphics();
    void testParseTJ();
    void testParseTm();
    void testParseComment();
    void testParseDanglingOperands();
    void testParseUnterminatedString();
    void testParseUnterminatedStringNoOperator();
    void testParseInlineImageUnsupported();
    void testParseMultipleOperators();
    void testParseRealNumbers();
    void testParseNegativeIntegers();
    void testParseOperatorOffsets();
    void testParseEmptyOperandOperator();
    void testParseBoolOperand();
    void testParseNullOperand();
    void testParseHexStringOperand();
    void testParseNestedArray();
    void testParseDictionaryOperand();
    void testParseUnterminatedArray();
    void testParseUnterminatedDictionary();
    void testParseUnexpectedDelimiter();
    void testParseStreamKeywordInContent();
    void testParseEndStreamKeywordInContent();
    void testParseDepthExceeded();
    void testParseIDasOperator();
    void testParseKeywordInArray();
    void testParseDictKeyNotName();
    void testParseArraySpan();
    void testParseDictionarySpan();
};

void PdfContentStreamTokenizerTest::testParseEmpty() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView(""));
    QVERIFY(r.ok());
    QVERIFY(r.operators.isEmpty());
}

void PdfContentStreamTokenizerTest::testParseWhitespaceOnly() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("  \n\r\n  "));
    QVERIFY(r.ok());
    QVERIFY(r.operators.isEmpty());
}

void PdfContentStreamTokenizerTest::testParseSimpleText() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT /F1 12 Tf 72 720 Td (Hello) Tj ET"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 5);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
    QCOMPARE(r.operators[1].name, QByteArray("Tf"));
    QCOMPARE(r.operators[2].name, QByteArray("Td"));
    QCOMPARE(r.operators[3].name, QByteArray("Tj"));
    QCOMPARE(r.operators[4].name, QByteArray("ET"));
    QCOMPARE(r.operators[1].operands.size(), 2);
    QCOMPARE(r.operators[3].operands.size(), 1);
}

void PdfContentStreamTokenizerTest::testParseGraphics() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("q 1 0 0 1 0 0 cm 0 0 100 100 re S Q"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 5);
    QCOMPARE(r.operators[0].name, QByteArray("q"));
    QCOMPARE(r.operators[1].name, QByteArray("cm"));
    QCOMPARE(r.operators[2].name, QByteArray("re"));
    QCOMPARE(r.operators[3].name, QByteArray("S"));
    QCOMPARE(r.operators[4].name, QByteArray("Q"));
    QVERIFY(r.operators[0].operands.isEmpty());
    QCOMPARE(r.operators[1].operands.size(), 6);
    QCOMPARE(r.operators[2].operands.size(), 4);
    QVERIFY(r.operators[3].operands.isEmpty());
    QVERIFY(r.operators[4].operands.isEmpty());
}

void PdfContentStreamTokenizerTest::testParseTJ() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT [(H) 20 (ello)] TJ ET"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 3);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
    QCOMPARE(r.operators[1].name, QByteArray("TJ"));
    QCOMPARE(r.operators[2].name, QByteArray("ET"));
    QCOMPARE(r.operators[1].operands.size(), 1);
    QCOMPARE(r.operators[1].operands[0].type, PdfContentOperandType::Array);
    QCOMPARE(r.operators[1].operands[0].arrayElements.size(), 3);
}

void PdfContentStreamTokenizerTest::testParseTm() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("1 0 0 1 72 720 Tm"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("Tm"));
    QCOMPARE(r.operators[0].operands.size(), 6);
}

void PdfContentStreamTokenizerTest::testParseComment() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("% comment\nBT ET"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 2);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
    QCOMPARE(r.operators[1].name, QByteArray("ET"));
}

void PdfContentStreamTokenizerTest::testParseDanglingOperands() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT /F1 12 Tf 72 720"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::DanglingOperands);
    QCOMPARE(r.operators.size(), 2);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
    QCOMPARE(r.operators[1].name, QByteArray("Tf"));
    QCOMPARE(r.danglingOperands.size(), 2);
}

void PdfContentStreamTokenizerTest::testParseUnterminatedString() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT (Hello Tj ET"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::UnterminatedString);
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
}

void PdfContentStreamTokenizerTest::testParseUnterminatedStringNoOperator() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("(Hello Tj"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::UnterminatedString);
    QCOMPARE(r.operators.size(), 0);
}

void PdfContentStreamTokenizerTest::testParseInlineImageUnsupported() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT /F1 10 Tf BI /W 10"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::UnsupportedInlineImage);
    QCOMPARE(r.operators.size(), 2);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
    QCOMPARE(r.operators[1].name, QByteArray("Tf"));
}

void PdfContentStreamTokenizerTest::testParseMultipleOperators() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("q BT /F1 10 Tf ET Q"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 5);
    QCOMPARE(r.operators[0].name, QByteArray("q"));
    QCOMPARE(r.operators[1].name, QByteArray("BT"));
    QCOMPARE(r.operators[2].name, QByteArray("Tf"));
    QCOMPARE(r.operators[3].name, QByteArray("ET"));
    QCOMPARE(r.operators[4].name, QByteArray("Q"));
}

void PdfContentStreamTokenizerTest::testParseRealNumbers() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("1.5 2.5 m"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators[0].operands.size(), 2);
    QCOMPARE(r.operators[0].operands[0].type, PdfContentOperandType::Real);
    QCOMPARE(r.operators[0].operands[1].type, PdfContentOperandType::Real);
}

void PdfContentStreamTokenizerTest::testParseNegativeIntegers() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("-10 20 m"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators[0].operands.size(), 2);
    QCOMPARE(r.operators[0].operands[0].type, PdfContentOperandType::Integer);
    QCOMPARE(r.operators[0].operands[0].intValue, int64_t(-10));
    QCOMPARE(r.operators[0].operands[1].type, PdfContentOperandType::Integer);
    QCOMPARE(r.operators[0].operands[1].intValue, int64_t(20));
}

void PdfContentStreamTokenizerTest::testParseOperatorOffsets() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("10 20 m"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("m"));
    QVERIFY(r.operators[0].offset > 0);
}

void PdfContentStreamTokenizerTest::testParseEmptyOperandOperator() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("q Q"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 2);
    QCOMPARE(r.operators[0].name, QByteArray("q"));
    QCOMPARE(r.operators[1].name, QByteArray("Q"));
    QVERIFY(r.operators[0].operands.isEmpty());
    QVERIFY(r.operators[1].operands.isEmpty());
}

void PdfContentStreamTokenizerTest::testParseBoolOperand() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("true false BT ET"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 2);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
    QCOMPARE(r.operators[0].operands.size(), 2);
    QCOMPARE(r.operators[0].operands[0].type, PdfContentOperandType::Boolean);
    QVERIFY(r.operators[0].operands[0].boolValue);
    QCOMPARE(r.operators[0].operands[1].type, PdfContentOperandType::Boolean);
    QVERIFY(!r.operators[0].operands[1].boolValue);
}

void PdfContentStreamTokenizerTest::testParseNullOperand() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("null BT ET"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 2);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
    QCOMPARE(r.operators[0].operands.size(), 1);
    QCOMPARE(r.operators[0].operands[0].type, PdfContentOperandType::Null);
}

void PdfContentStreamTokenizerTest::testParseHexStringOperand() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("<48656C6C6F> Tj ET"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 2);
    QCOMPARE(r.operators[0].name, QByteArray("Tj"));
    QCOMPARE(r.operators[0].operands.size(), 1);
    QCOMPARE(r.operators[0].operands[0].type, PdfContentOperandType::String);
    QCOMPARE(r.operators[0].operands[0].bytes, QByteArray("Hello"));
}

void PdfContentStreamTokenizerTest::testParseNestedArray() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("[[1 2] 3] Tm"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].operands.size(), 1);
    QCOMPARE(r.operators[0].operands[0].type, PdfContentOperandType::Array);
    QCOMPARE(r.operators[0].operands[0].arrayElements.size(), 2);
    QCOMPARE(r.operators[0].operands[0].arrayElements[0].type, PdfContentOperandType::Array);
    QCOMPARE(r.operators[0].operands[0].arrayElements[1].type, PdfContentOperandType::Integer);
}

void PdfContentStreamTokenizerTest::testParseDictionaryOperand() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("/Span << /ActualText (text) >> BDC EMC"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 2);
    QCOMPARE(r.operators[0].name, QByteArray("BDC"));
    QCOMPARE(r.operators[0].operands.size(), 2);
    QCOMPARE(r.operators[0].operands[0].type, PdfContentOperandType::Name);
    QCOMPARE(r.operators[0].operands[1].type, PdfContentOperandType::Dictionary);
    QCOMPARE(r.operators[0].operands[1].dictionaryEntries.size(), 1);
}

void PdfContentStreamTokenizerTest::testParseUnterminatedArray() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT [1 2 3"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::UnterminatedArray);
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
}

void PdfContentStreamTokenizerTest::testParseUnterminatedDictionary() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT << /Foo /Bar"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::UnterminatedDictionary);
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
}

void PdfContentStreamTokenizerTest::testParseUnexpectedDelimiter() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT ) Tj ET"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::UnexpectedToken);
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
}

void PdfContentStreamTokenizerTest::testParseStreamKeywordInContent() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT stream Tj ET"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::TokenizerError);
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
}

void PdfContentStreamTokenizerTest::testParseEndStreamKeywordInContent() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT endstream Tj ET"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::TokenizerError);
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
}

void PdfContentStreamTokenizerTest::testParseDepthExceeded() {
    PdfContentStreamTokenizer tokenizer;
    QByteArray deep;
    for (int i = 0; i < 70; ++i) deep.append('[');
    for (int i = 0; i < 65; ++i) deep.append(']');
    deep.append(" Tm");
    auto r = tokenizer.parse(QByteArrayView(deep));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::DepthExceeded);
}

void PdfContentStreamTokenizerTest::testParseIDasOperator() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT ID Tj ET"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 4);
    QCOMPARE(r.operators[0].name, QByteArray("BT"));
    QCOMPARE(r.operators[1].name, QByteArray("ID"));
    QCOMPARE(r.operators[2].name, QByteArray("Tj"));
    QCOMPARE(r.operators[3].name, QByteArray("ET"));
}

void PdfContentStreamTokenizerTest::testParseKeywordInArray() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("[1 Tm] TJ"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::UnexpectedToken);
}

void PdfContentStreamTokenizerTest::testParseDictKeyNotName() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("<< 42 /Value >> BDC"));
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamTokenizerWarning::UnexpectedToken);
}

void PdfContentStreamTokenizerTest::testParseArraySpan() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("BT [1 2] Tj ET"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators[1].operands.size(), 1);
    const auto &arr = r.operators[1].operands[0];
    QCOMPARE(arr.type, PdfContentOperandType::Array);
    QVERIFY(arr.offset > 0);
    QVERIFY(arr.length > 0);
}

void PdfContentStreamTokenizerTest::testParseDictionarySpan() {
    PdfContentStreamTokenizer tokenizer;
    auto r = tokenizer.parse(QByteArrayView("/Tag << /ActualText (text) >> BDC"));
    QVERIFY(r.ok());
    QCOMPARE(r.operators.size(), 1);
    QCOMPARE(r.operators[0].name, QByteArray("BDC"));
    QCOMPARE(r.operators[0].operands.size(), 2);
    const auto &dict = r.operators[0].operands[1];
    QCOMPARE(dict.type, PdfContentOperandType::Dictionary);
    QVERIFY(dict.offset > 0);
    QVERIFY(dict.length > 0);
}

QTEST_MAIN(PdfContentStreamTokenizerTest)
#include "PdfContentStreamTokenizerTest.moc"
