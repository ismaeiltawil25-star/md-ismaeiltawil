#include <QtTest/QTest>
#include "PdfByteReader.h"
#include "PdfTokenizer.h"

class PdfTokenizerTest : public QObject {
    Q_OBJECT

private slots:
    // Integer
    void testIntegerPositive();
    void testIntegerNegative();
    void testIntegerZero();
    void testIntegerSigned();
    // Real
    void testRealSimple();
    void testRealLeadingDot();
    void testRealNegative();
    void testRealSignDot();
    void testRealTrailingDot();
    // Malformed numbers
    void testMultipleDots_isInvalid();
    void testSignOnly_isInvalid();
    void testSignDotNoDigits_isInvalid();
    void testIntegerOverflow_isInvalid();
    // No splitting
    void testObj42_isKeyword();
    void testKeywordR();
    void testKeywordNull();
    void testBoolTrue();
    void testBoolFalse();
    void testStreamKeyword();
    void testEndstreamKeyword();
    // Names
    void testNameSimple();
    void testNameEmpty();
    void testNameHexEscape();
    void testNameHexEscapeHash();
    void testNameMalformedEscape_hashOnly();
    void testNameMalformedEscape_hashDigit();
    void testNameMalformedEscape_hashInMiddle();
    void testNameMalformedEscape_atEof();
    void testNameStopsAtDelimiter();
    // String literals
    void testStringSimple();
    void testStringBalancedParens();
    void testStringEmpty();
    void testStringEscape_n();
    void testStringEscape_r();
    void testStringEscape_t();
    void testStringEscape_backslash();
    void testStringEscape_parens();
    void testStringOctal();
    void testStringOctal3digit();
    void testStringLineContinuationCRLF();
    void testStringLineContinuationLF();
    void testStringUnterminated();
    // Hex strings
    void testHexStringHello();
    void testHexStringOdd();
    void testHexStringSingle6();
    void testHexStringSingleF();
    void testHexStringEmpty();
    void testHexStringWhitespace();
    void testHexStringMixedCase();
    void testHexStringNonHexChar();
    void testHexStringUnterminated();
    // Delimiters
    void testArrayStartEnd();
    void testDictStartEnd();
    void testDictStartAdjacent();
    void testSingleRightAngle_isInvalid();
    void testLeftBrace_isInvalid();
    void testRightBrace_isInvalid();
    void testUnmatchedRightParen_isInvalid();
    // Whitespace and comments
    void testWhitespaceSkipped();
    void testNewlineSkipped();
    void testCommentsSkipped();
    void testCommentCRLF();
    void testCommentOnly();
    void testWhitespaceOnly();
    // Tokenizer API
    void testPeekReturnsSame();
    void testConsumeAfterPeek();
    void testConsumeWithoutPeek();
    void testConsumeAtEof();
    void testEndOfFile();
    void testEndOfFileAfterAll();
    // Sequences
    void testSequence();
    void testIndirectRefTokens();
    void testDictWithArray();
    // Offset and length
    void testOffsetInteger();
    void testOffsetName();
    void testOffsetNameWithEscape();
    void testOffsetDictStart();
    void testInvalidOffsetAndBytes();
};

// ============================================================
// Integer tests
// ============================================================

void PdfTokenizerTest::testIntegerPositive() {
    QByteArray data("42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Integer);
    QCOMPARE(result.token.intValue, int64_t(42));
    QCOMPARE(result.warning, PdfTokenizerWarning::None);
}

void PdfTokenizerTest::testIntegerNegative() {
    QByteArray data("-17 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Integer);
    QCOMPARE(result.token.intValue, int64_t(-17));
    QCOMPARE(result.warning, PdfTokenizerWarning::None);
}

void PdfTokenizerTest::testIntegerZero() {
    QByteArray data("0 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Integer);
    QCOMPARE(result.token.intValue, int64_t(0));
}

void PdfTokenizerTest::testIntegerSigned() {
    QByteArray data("+5 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Integer);
    QCOMPARE(result.token.intValue, int64_t(5));
}

// ============================================================
// Real tests
// ============================================================

void PdfTokenizerTest::testRealSimple() {
    QByteArray data("3.14 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Real);
    QCOMPARE(result.token.realValue, 3.14);
    QCOMPARE(result.warning, PdfTokenizerWarning::None);
}

void PdfTokenizerTest::testRealLeadingDot() {
    QByteArray data(".5 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Real);
    QCOMPARE(result.token.realValue, 0.5);
}

void PdfTokenizerTest::testRealNegative() {
    QByteArray data("-0.5 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Real);
    QCOMPARE(result.token.realValue, -0.5);
}

void PdfTokenizerTest::testRealSignDot() {
    QByteArray data("-.5 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Real);
    QCOMPARE(result.token.realValue, -0.5);
}

void PdfTokenizerTest::testRealTrailingDot() {
    QByteArray data("42. ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Real);
    QCOMPARE(result.token.realValue, 42.0);
}

// ============================================================
// Malformed number tests
// ============================================================

void PdfTokenizerTest::testMultipleDots_isInvalid() {
    QByteArray data("3.14.15 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::MalformedNumber);
}

void PdfTokenizerTest::testSignOnly_isInvalid() {
    QByteArray data("+ ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::MalformedNumber);
}

void PdfTokenizerTest::testSignDotNoDigits_isInvalid() {
    QByteArray data("+. ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::MalformedNumber);
}

void PdfTokenizerTest::testIntegerOverflow_isInvalid() {
    QByteArray data("999999999999999999999999999999 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::IntegerOverflow);
    QVERIFY(!result.diagnosticMessage.isEmpty());
}

// ============================================================
// No-splitting tests
// ============================================================

void PdfTokenizerTest::testObj42_isKeyword() {
    QByteArray data("obj42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Keyword);
    QCOMPARE(result.token.bytes, QByteArray("obj42"));
}

void PdfTokenizerTest::testKeywordR() {
    QByteArray data("R ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Keyword);
    QCOMPARE(result.token.bytes, QByteArray("R"));
}

void PdfTokenizerTest::testKeywordNull() {
    QByteArray data("null ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Keyword);
    QCOMPARE(result.token.bytes, QByteArray("null"));
}

void PdfTokenizerTest::testBoolTrue() {
    QByteArray data("true ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Bool);
    QCOMPARE(result.token.boolValue, true);
}

void PdfTokenizerTest::testBoolFalse() {
    QByteArray data("false ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Bool);
    QCOMPARE(result.token.boolValue, false);
}

void PdfTokenizerTest::testStreamKeyword() {
    QByteArray data("stream ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StreamKeyword);
}

void PdfTokenizerTest::testEndstreamKeyword() {
    QByteArray data("endstream ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::EndStreamKeyword);
}

// ============================================================
// Name tests
// ============================================================

void PdfTokenizerTest::testNameSimple() {
    QByteArray data("/Type ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray("Type"));
    QCOMPARE(result.warning, PdfTokenizerWarning::None);
}

void PdfTokenizerTest::testNameEmpty() {
    QByteArray data("/ ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray());
}

void PdfTokenizerTest::testNameHexEscape() {
    QByteArray data("/Page#20One ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray("Page One"));
    QCOMPARE(result.warning, PdfTokenizerWarning::None);
}

void PdfTokenizerTest::testNameHexEscapeHash() {
    QByteArray data("/#23#4E ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray("#N"));
    QCOMPARE(result.warning, PdfTokenizerWarning::None);
}

void PdfTokenizerTest::testNameMalformedEscape_hashOnly() {
    QByteArray data("/# ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray("#"));
    QCOMPARE(result.warning, PdfTokenizerWarning::MalformedNameEscape);
}

void PdfTokenizerTest::testNameMalformedEscape_hashDigit() {
    QByteArray data("/#2 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray("#2"));
    QCOMPARE(result.warning, PdfTokenizerWarning::MalformedNameEscape);
}

void PdfTokenizerTest::testNameMalformedEscape_hashInMiddle() {
    QByteArray data("/AB#2Z ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray("AB#2Z"));
    QCOMPARE(result.warning, PdfTokenizerWarning::MalformedNameEscape);
}

void PdfTokenizerTest::testNameMalformedEscape_atEof() {
    QByteArray data("/AB#");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray("AB#"));
    QCOMPARE(result.warning, PdfTokenizerWarning::MalformedNameEscape);
}

void PdfTokenizerTest::testNameStopsAtDelimiter() {
    QByteArray data("/Type[ ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.bytes, QByteArray("Type"));
}

// ============================================================
// String literal tests
// ============================================================

void PdfTokenizerTest::testStringSimple() {
    QByteArray data("(Hello)");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("Hello"));
    QCOMPARE(result.warning, PdfTokenizerWarning::None);
}

void PdfTokenizerTest::testStringBalancedParens() {
    QByteArray data("(foo (bar) baz)");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("foo (bar) baz"));
}

void PdfTokenizerTest::testStringEmpty() {
    QByteArray data("()");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray());
}

void PdfTokenizerTest::testStringEscape_n() {
    QByteArray data;
    data += '(';
    data += QByteArray("Line\\nBreak");
    data += ')';
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("Line\nBreak", 10));
}

void PdfTokenizerTest::testStringEscape_r() {
    QByteArray data;
    data += '(';
    data += QByteArray("Line\\rBreak");
    data += ')';
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("Line\rBreak", 10));
}

void PdfTokenizerTest::testStringEscape_t() {
    QByteArray data;
    data += '(';
    data += QByteArray("Tab\\there");
    data += ')';
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("Tab\there", 8));
}

void PdfTokenizerTest::testStringEscape_backslash() {
    QByteArray data;
    data += '(';
    data += QByteArray("Path\\\\", 6);
    data += ')';
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("Path\\", 5));
}

void PdfTokenizerTest::testStringEscape_parens() {
    QByteArray data;
    data += '(';
    data += QByteArray("\\(hello\\)");
    data += ')';
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("(hello)"));
}

void PdfTokenizerTest::testStringOctal() {
    QByteArray data;
    data += '(';
    data += QByteArray("\\101", 4);
    data += ')';
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("A"));
}

void PdfTokenizerTest::testStringOctal3digit() {
    QByteArray data;
    data += '(';
    data += QByteArray("\\377", 4);
    data += ')';
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes.size(), 1);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[0]), 0xFFu);
}

void PdfTokenizerTest::testStringLineContinuationCRLF() {
    QByteArray data;
    data += '(';
    data += QByteArray("a\\", 2);
    data += QByteArray("\r\n", 2);
    data += QByteArray("b)", 2);
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("ab"));
}

void PdfTokenizerTest::testStringLineContinuationLF() {
    QByteArray data;
    data += '(';
    data += QByteArray("a\\", 2);
    data += QByteArray("\n", 1);
    data += QByteArray("b)", 2);
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("ab"));
}

void PdfTokenizerTest::testStringUnterminated() {
    QByteArray data("(hello");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::StringLiteral);
    QCOMPARE(result.token.bytes, QByteArray("hello"));
    QCOMPARE(result.warning, PdfTokenizerWarning::UnterminatedString);
}

// ============================================================
// Hex string tests
// ============================================================

void PdfTokenizerTest::testHexStringHello() {
    QByteArray data("<48656C6C6F>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::HexString);
    QCOMPARE(result.token.bytes, QByteArray("Hello"));
    QCOMPARE(result.warning, PdfTokenizerWarning::None);
}

void PdfTokenizerTest::testHexStringOdd() {
    QByteArray data("<48656C6C6>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::HexString);
    QCOMPARE(result.token.bytes.size(), 5);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[0]), 0x48u);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[1]), 0x65u);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[2]), 0x6Cu);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[3]), 0x6Cu);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[4]), 0x60u);
}

void PdfTokenizerTest::testHexStringSingle6() {
    QByteArray data("<6>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::HexString);
    QCOMPARE(result.token.bytes.size(), 1);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[0]), 0x60u);
}

void PdfTokenizerTest::testHexStringSingleF() {
    QByteArray data("<F>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::HexString);
    QCOMPARE(result.token.bytes.size(), 1);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[0]), 0xF0u);
}

void PdfTokenizerTest::testHexStringEmpty() {
    QByteArray data("<>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::HexString);
    QCOMPARE(result.token.bytes.size(), 0);
}

void PdfTokenizerTest::testHexStringWhitespace() {
    QByteArray data("<48 65 6C>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::HexString);
    QCOMPARE(result.token.bytes, QByteArray("Hel"));
}

void PdfTokenizerTest::testHexStringMixedCase() {
    QByteArray data("<aBcDeF>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::HexString);
    QCOMPARE(result.token.bytes.size(), 3);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[0]), 0xABu);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[1]), 0xCDu);
    QCOMPARE(static_cast<unsigned char>(result.token.bytes[2]), 0xEFu);
}

void PdfTokenizerTest::testHexStringNonHexChar() {
    QByteArray data("<ABXG>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::NonHexCharInHexString);
    QVERIFY(!result.diagnosticMessage.isEmpty());
}

void PdfTokenizerTest::testHexStringUnterminated() {
    QByteArray data("<4865");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::HexString);
    QCOMPARE(result.token.bytes, QByteArray("He"));
    QCOMPARE(result.warning, PdfTokenizerWarning::UnterminatedHexString);
}

// ============================================================
// Delimiter tests
// ============================================================

void PdfTokenizerTest::testArrayStartEnd() {
    QByteArray data("[] ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto r1 = tokenizer.nextToken();
    QCOMPARE(r1.token.type, PdfTokenType::ArrayStart);
    auto r2 = tokenizer.nextToken();
    QCOMPARE(r2.token.type, PdfTokenType::ArrayEnd);
}

void PdfTokenizerTest::testDictStartEnd() {
    QByteArray data("<<>> ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto r1 = tokenizer.nextToken();
    QCOMPARE(r1.token.type, PdfTokenType::DictStart);
    auto r2 = tokenizer.nextToken();
    QCOMPARE(r2.token.type, PdfTokenType::DictEnd);
}

void PdfTokenizerTest::testDictStartAdjacent() {
    QByteArray data("<<dict>> ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto r1 = tokenizer.nextToken();
    QCOMPARE(r1.token.type, PdfTokenType::DictStart);
    auto r2 = tokenizer.nextToken();
    QCOMPARE(r2.token.type, PdfTokenType::Keyword);
    QCOMPARE(r2.token.bytes, QByteArray("dict"));
    auto r3 = tokenizer.nextToken();
    QCOMPARE(r3.token.type, PdfTokenType::DictEnd);
}

void PdfTokenizerTest::testSingleRightAngle_isInvalid() {
    QByteArray data("> ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::UnmatchedRightAngle);
    QVERIFY(!result.diagnosticMessage.isEmpty());
}

void PdfTokenizerTest::testLeftBrace_isInvalid() {
    QByteArray data("{ ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::UnexpectedDelimiter);
    QVERIFY(!result.diagnosticMessage.isEmpty());
}

void PdfTokenizerTest::testRightBrace_isInvalid() {
    QByteArray data("} ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::UnexpectedDelimiter);
    QVERIFY(!result.diagnosticMessage.isEmpty());
}

void PdfTokenizerTest::testUnmatchedRightParen_isInvalid() {
    QByteArray data(") ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.warning, PdfTokenizerWarning::UnexpectedDelimiter);
    QVERIFY(!result.diagnosticMessage.isEmpty());
}

// ============================================================
// Whitespace and comment tests
// ============================================================

void PdfTokenizerTest::testWhitespaceSkipped() {
    QByteArray data("  42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Integer);
    QCOMPARE(result.token.intValue, int64_t(42));
}

void PdfTokenizerTest::testNewlineSkipped() {
    QByteArray data("\r\n42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Integer);
    QCOMPARE(result.token.intValue, int64_t(42));
}

void PdfTokenizerTest::testCommentsSkipped() {
    QByteArray data("%comment\n42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Integer);
    QCOMPARE(result.token.intValue, int64_t(42));
}

void PdfTokenizerTest::testCommentCRLF() {
    QByteArray data("%c\r\n42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Integer);
    QCOMPARE(result.token.intValue, int64_t(42));
}

void PdfTokenizerTest::testCommentOnly() {
    QByteArray data("%c\n");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::EndOfFile);
}

void PdfTokenizerTest::testWhitespaceOnly() {
    QByteArray data("  \t\r\n");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::EndOfFile);
}

// ============================================================
// Tokenizer API tests
// ============================================================

void PdfTokenizerTest::testPeekReturnsSame() {
    QByteArray data("42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto r1 = tokenizer.peekToken();
    auto r2 = tokenizer.peekToken();
    QCOMPARE(r1.token.type, PdfTokenType::Integer);
    QCOMPARE(r1.token.intValue, int64_t(42));
    QCOMPARE(r2.token.type, PdfTokenType::Integer);
    QCOMPARE(r2.token.intValue, int64_t(42));
}

void PdfTokenizerTest::testConsumeAfterPeek() {
    QByteArray data("42 0 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto peeked = tokenizer.peekToken();
    QCOMPARE(peeked.token.type, PdfTokenType::Integer);
    QCOMPARE(peeked.token.intValue, int64_t(42));
    tokenizer.consumeToken();
    auto next = tokenizer.nextToken();
    QCOMPARE(next.token.type, PdfTokenType::Integer);
    QCOMPARE(next.token.intValue, int64_t(0));
}

void PdfTokenizerTest::testConsumeWithoutPeek() {
    QByteArray data("42 0 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    tokenizer.consumeToken();
    auto next = tokenizer.nextToken();
    QCOMPARE(next.token.type, PdfTokenType::Integer);
    QCOMPARE(next.token.intValue, int64_t(0));
}

void PdfTokenizerTest::testConsumeAtEof() {
    QByteArray data;
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    tokenizer.consumeToken();
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::EndOfFile);
}

void PdfTokenizerTest::testEndOfFile() {
    QByteArray data;
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::EndOfFile);
}

void PdfTokenizerTest::testEndOfFileAfterAll() {
    QByteArray data("42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    tokenizer.nextToken();
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::EndOfFile);
}

// ============================================================
// Sequence tests
// ============================================================

void PdfTokenizerTest::testSequence() {
    QByteArray data("42 0 obj << /Type /Catalog >> endobj ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);

    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Integer);    // 42
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Integer);    // 0
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Keyword);    // obj
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::DictStart);  // <<
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Name);       // /Type
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Name);       // /Catalog
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::DictEnd);    // >>
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Keyword);    // endobj
}

void PdfTokenizerTest::testIndirectRefTokens() {
    QByteArray data("42 0 R ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);

    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Integer);   // 42
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Integer);   // 0
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Keyword);   // R
}

void PdfTokenizerTest::testDictWithArray() {
    QByteArray data("<< /MediaBox [0 0 612 792] >> ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);

    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::DictStart);
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Name);
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::ArrayStart);
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Integer);    // 0
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Integer);    // 0
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Integer);    // 612
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::Integer);    // 792
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::ArrayEnd);
    QCOMPARE(tokenizer.nextToken().token.type, PdfTokenType::DictEnd);
}

// ============================================================
// Offset and length tests
// ============================================================

void PdfTokenizerTest::testOffsetInteger() {
    QByteArray data("42 ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.offset, qsizetype(0));
    QCOMPARE(result.token.length, qsizetype(2));
}

void PdfTokenizerTest::testOffsetName() {
    QByteArray data("/Type ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.offset, qsizetype(0));
    QCOMPARE(result.token.length, qsizetype(5));
}

void PdfTokenizerTest::testOffsetNameWithEscape() {
    QByteArray data("/Page#20One ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Name);
    QCOMPARE(result.token.offset, qsizetype(0));
    QCOMPARE(result.token.length, qsizetype(11));
    QCOMPARE(result.token.bytes, QByteArray("Page One"));
}

void PdfTokenizerTest::testOffsetDictStart() {
    QByteArray data("<< ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.offset, qsizetype(0));
    QCOMPARE(result.token.length, qsizetype(2));
}

void PdfTokenizerTest::testInvalidOffsetAndBytes() {
    QByteArray data("{ }");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    auto result = tokenizer.nextToken();
    QCOMPARE(result.token.type, PdfTokenType::Invalid);
    QCOMPARE(result.token.offset, qsizetype(0));
    QCOMPARE(result.token.length, qsizetype(1));
    QCOMPARE(result.token.bytes, QByteArray("{"));
}

QTEST_MAIN(PdfTokenizerTest)
#include "PdfTokenizerTest.moc"
