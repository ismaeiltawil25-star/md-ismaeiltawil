// Tests for mdbr::text::decode — encoding detection and decoding.
//
// All non-ASCII text below is written with \u escapes or raw hex bytes so the
// test does not depend on the compiler's source-file encoding.

#include <QTest>
#include "services/text/TextDecoder.h"

using mdbr::text::SourceEncoding;

namespace {

// "مرحبا بالعالم" (Hello world)
const QString kArabic = QStringLiteral(u"\u0645\u0631\u062D\u0628\u0627 \u0628\u0627\u0644\u0639\u0627\u0644\u0645");

QByteArray utf16Bytes(const QString &text, bool littleEndian)
{
    QByteArray out;
    for (const QChar c : text) {
        const char lo = static_cast<char>(c.unicode() & 0xFF);
        const char hi = static_cast<char>(c.unicode() >> 8);
        out.append(littleEndian ? lo : hi);
        out.append(littleEndian ? hi : lo);
    }
    return out;
}

} // namespace

class TextDecoderTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyInputIsEmptyUtf8();
    void asciiIsExactUtf8();
    void arabicUtf8IsExact();
    void utf8BomIsStripped();
    void utf16LittleEndianWithBom();
    void utf16BigEndianWithBom();
    void utf16WithoutBomIsGuessed();
    void windows1256ArabicIsGuessed();
    void windows1252GermanIsNotMistakenForArabic();
    void windows1252FrenchFallback();
    void decodeNeverLosesAsciiContent();
    void normalizeLineEndingsConvertsCrlfAndCr();
};

void TextDecoderTest::emptyInputIsEmptyUtf8()
{
    const auto result = mdbr::text::decode({});
    QVERIFY(result.text.isEmpty());
    QCOMPARE(result.encoding, SourceEncoding::Utf8);
    QVERIFY(!result.isGuessed);
}

void TextDecoderTest::asciiIsExactUtf8()
{
    const auto result = mdbr::text::decode("# Hello\n\nWorld");
    QCOMPARE(result.text, QStringLiteral("# Hello\n\nWorld"));
    QCOMPARE(result.encoding, SourceEncoding::Utf8);
    QVERIFY(!result.isGuessed);
}

void TextDecoderTest::arabicUtf8IsExact()
{
    const auto result = mdbr::text::decode(kArabic.toUtf8());
    QCOMPARE(result.text, kArabic);
    QCOMPARE(result.encoding, SourceEncoding::Utf8);
    QVERIFY(!result.isGuessed);
}

void TextDecoderTest::utf8BomIsStripped()
{
    const auto result = mdbr::text::decode(QByteArray("\xEF\xBB\xBF") + kArabic.toUtf8());
    QCOMPARE(result.text, kArabic);
    QCOMPARE(result.encoding, SourceEncoding::Utf8WithBom);
    QVERIFY(!result.isGuessed);
}

void TextDecoderTest::utf16LittleEndianWithBom()
{
    const auto result = mdbr::text::decode(QByteArray("\xFF\xFE") + utf16Bytes(kArabic, true));
    QCOMPARE(result.text, kArabic);
    QCOMPARE(result.encoding, SourceEncoding::Utf16LE);
    QVERIFY(!result.isGuessed);
}

void TextDecoderTest::utf16BigEndianWithBom()
{
    const auto result = mdbr::text::decode(QByteArray("\xFE\xFF") + utf16Bytes(kArabic, false));
    QCOMPARE(result.text, kArabic);
    QCOMPARE(result.encoding, SourceEncoding::Utf16BE);
}

void TextDecoderTest::utf16WithoutBomIsGuessed()
{
    const QString text = QStringLiteral("# Title\n\nSome plain paragraph.");
    const auto result = mdbr::text::decode(utf16Bytes(text, true));
    QCOMPARE(result.text, text);
    QCOMPARE(result.encoding, SourceEncoding::Utf16LE);
    QVERIFY(result.isGuessed);
}

void TextDecoderTest::windows1256ArabicIsGuessed()
{
    const QByteArray bytes = QByteArray::fromHex("e3d1cdc8c720c8c7e1dac7e1e3");
    const auto result = mdbr::text::decode(QByteArray("# ") + bytes);
    QCOMPARE(result.text, QStringLiteral("# ") + kArabic);
    QCOMPARE(result.encoding, SourceEncoding::Windows1256);
    QVERIFY(result.isGuessed);
}

void TextDecoderTest::windows1252GermanIsNotMistakenForArabic()
{
    // "Schöne Grüße aus München" — 0xF6/0xDF map to Arabic letters in 1256.
    const QByteArray bytes = QByteArray::fromHex("536368f66e65204772fcdf6520617573204dfc6e6368656e");
    const auto result = mdbr::text::decode(bytes);
    QCOMPARE(result.encoding, SourceEncoding::Windows1252);
    QCOMPARE(result.text, QStringLiteral(u"Sch\u00F6ne Gr\u00FC\u00DFe aus M\u00FCnchen"));
}

void TextDecoderTest::windows1252FrenchFallback()
{
    const auto result = mdbr::text::decode(QByteArray::fromHex("636166e9206372e86d65"));
    QCOMPARE(result.encoding, SourceEncoding::Windows1252);
    QCOMPARE(result.text, QStringLiteral(u"caf\u00E9 cr\u00E8me"));
}

void TextDecoderTest::decodeNeverLosesAsciiContent()
{
    // Whatever encoding is chosen for invalid input, ASCII must survive unchanged.
    const QByteArray bytes = QByteArray("start ") + QByteArray("\xC3\x28\xFF\x80") + QByteArray(" end");
    const auto result = mdbr::text::decode(bytes);
    QVERIFY(result.text.startsWith(QStringLiteral("start ")));
    QVERIFY(result.text.endsWith(QStringLiteral(" end")));
    QVERIFY(result.isGuessed);
}

void TextDecoderTest::normalizeLineEndingsConvertsCrlfAndCr()
{
    QCOMPARE(mdbr::text::normalizeLineEndings(QStringLiteral("a\r\nb\rc\nd")),
             QStringLiteral("a\nb\nc\nd"));
}

QTEST_APPLESS_MAIN(TextDecoderTest)
#include "TextDecoderTest.moc"
