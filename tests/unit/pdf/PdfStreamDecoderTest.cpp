#include <QtTest/QTest>
#include "PdfStreamDecoder.h"
#include "PdfZlibTestHelpers.h"

static PdfRawStreamObject makeStream(QByteArrayView rawBody,
                                      const QList<QByteArray> &filters = {},
                                      PdfObjectPtr decodeParms = nullptr) {
    PdfRawStreamObject s;
    s.rawBody = rawBody;
    s.filters = filters;
    s.decodeParms = decodeParms;
    return s;
}

static PdfObjectPtr makeDictObj(std::initializer_list<std::pair<const char *, PdfObjectPtr>> entries) {
    auto dict = QSharedPointer<PdfDictionary>::create();
    for (const auto &e : entries) {
        dict->entries.insert(QByteArray(e.first), e.second);
    }
    return PdfObject::makeDictionary(dict, 0, 0);
}

class PdfStreamDecoderTest : public QObject {
    Q_OBJECT

private slots:
    void testDecodeIdentity();
    void testDecodeEmptyIdentity();
    void testDecodeIdentityTooLarge();
    void testDecodeFlate();
    void testDecodeFlateAliasFl();
    void testDecodeFlateEmpty();
    void testDecodeFlateCorrupt();
    void testDecodeFlateTrailingBytes();
    void testDecodeFlateWithinLimit();
    void testDecodeFlateExceedsLimit();
    void testDecodeFlateExactLimitStreamEnd();
    void testDecodeFlateExactLimitProbeExtraByte();
    void testDecodeFlateLimitPlusOne();
    void testDecodeFlateTruncatedAtLimit();
    void testDecodeFlateZeroLimitNonEmpty();
    void testDecodeUnsupportedFilter();
    void testDecodeUnsupportedFilterChain();
    void testDecodeUnsupportedFilterIgnoresPredictor();
    void testDecodeFilterChainIgnoresPredictor();
    void testDecodeFilterArraySingleFlate();
    void testDecodePredictor1();
    void testDecodePredictorAbsent();
    void testDecodePredictor2();
    void testDecodePredictor0();
    void testDecodePredictorNegative();
    void testDecodePredictorName();
    void testDecodeNonDictDecodeParms();
    void testDecodeNullDecodeParms();
    void testDecodeMaxOutputZeroEmptyIdentity();
    void testDecodeMaxOutputZeroNonEmptyIdentity();
    void testDecodeDecodeParmsArraySingular();
    void testDecodeDecodeParmsArrayUnsupportedPredictor();
    void testDecodeDecodeParmsArrayMultiElement();
    void testDecodeFilterArrayDecodeParmsDict();
    void testDecodeFilterArrayDecodeParmsArraySingular();
    void testDecodeFilterArrayDecodeParmsMultiArray();
    void testDecodeDecodeParmsAbsentKey();
};

// Identity

void PdfStreamDecoderTest::testDecodeIdentity() {
    PdfStreamDecoder decoder;
    auto s = makeStream(QByteArrayView("AAAAA"));
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, QByteArray("AAAAA"));
}

void PdfStreamDecoderTest::testDecodeEmptyIdentity() {
    PdfStreamDecoder decoder;
    auto s = makeStream(QByteArrayView(""));
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QVERIFY(r.decodedBytes.isEmpty());
}

void PdfStreamDecoderTest::testDecodeIdentityTooLarge() {
    PdfStreamDecoder decoder(10);
    auto s = makeStream(QByteArrayView("12345678901234"));
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::OutputTooLarge);
}

// Flate

void PdfStreamDecoderTest::testDecodeFlate() {
    PdfStreamDecoder decoder;
    QByteArray orig("Hello World! Hello PDF stream decoding.");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeFlateAliasFl() {
    PdfStreamDecoder decoder;
    QByteArray orig("Alias test data");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto s = makeStream(QByteArrayView(compressed), {{"Fl"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeFlateEmpty() {
    PdfStreamDecoder decoder;
    auto s = makeStream(QByteArrayView(""), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QVERIFY(r.decodedBytes.isEmpty());
}

void PdfStreamDecoderTest::testDecodeFlateCorrupt() {
    PdfStreamDecoder decoder;
    QByteArray random(100, 'X');
    auto s = makeStream(QByteArrayView(random), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::DecodeFailure);
}

void PdfStreamDecoderTest::testDecodeFlateTrailingBytes() {
    PdfStreamDecoder decoder;
    QByteArray orig("Trailing test");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());
    QByteArray withTrailing = compressed + QByteArray("garbage trailing bytes");

    auto s = makeStream(QByteArrayView(withTrailing), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

// Output limit

void PdfStreamDecoderTest::testDecodeFlateWithinLimit() {
    QByteArray orig(20, 'A');
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    PdfStreamDecoder decoder(100);
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeFlateExceedsLimit() {
    QByteArray orig(20, 'A');
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    PdfStreamDecoder decoder(10);
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::OutputTooLarge);
}

void PdfStreamDecoderTest::testDecodeFlateExactLimitStreamEnd() {
    QByteArray orig(20, 'A');
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    PdfStreamDecoder decoder(20);
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeFlateExactLimitProbeExtraByte() {
    QByteArray orig(20, 'A');
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    PdfStreamDecoder decoder(19);
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::OutputTooLarge);
}

void PdfStreamDecoderTest::testDecodeFlateLimitPlusOne() {
    QByteArray orig(20, 'A');
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    PdfStreamDecoder decoder(21);
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeFlateTruncatedAtLimit() {
    QByteArray orig(50, 'A');
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());
    // Truncate the compressed data to make it incomplete
    compressed.truncate(compressed.size() / 2);

    PdfStreamDecoder decoder(100);
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::DecodeFailure);
}

void PdfStreamDecoderTest::testDecodeFlateZeroLimitNonEmpty() {
    QByteArray orig(20, 'A');
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    PdfStreamDecoder decoder(0);
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::OutputTooLarge);
}

// Unsupported filter / filter chain

void PdfStreamDecoderTest::testDecodeUnsupportedFilter() {
    PdfStreamDecoder decoder;
    auto s = makeStream(QByteArrayView("AAAAA"), {{"ASCIIHexDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedFilter);
}

void PdfStreamDecoderTest::testDecodeUnsupportedFilterChain() {
    PdfStreamDecoder decoder;
    auto s = makeStream(QByteArrayView("AAAAA"), {{"FlateDecode", "ASCII85Decode"}});
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedFilterChain);
}

void PdfStreamDecoderTest::testDecodeUnsupportedFilterIgnoresPredictor() {
    PdfStreamDecoder decoder;
    auto dp = makeDictObj({{"Predictor", PdfObject::makeInteger(2, 0, 0)}});
    auto s = makeStream(QByteArrayView("AAAAA"), {{"ASCIIHexDecode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedFilter);
}

void PdfStreamDecoderTest::testDecodeFilterChainIgnoresPredictor() {
    PdfStreamDecoder decoder;
    auto dp = makeDictObj({{"Predictor", PdfObject::makeInteger(2, 0, 0)}});
    auto s = makeStream(QByteArrayView("AAAAA"), {{"FlateDecode", "ASCII85Decode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedFilterChain);
}

void PdfStreamDecoderTest::testDecodeFilterArraySingleFlate() {
    PdfStreamDecoder decoder;
    QByteArray orig("Single filter array test");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

// Predictor

void PdfStreamDecoderTest::testDecodePredictor1() {
    PdfStreamDecoder decoder;
    QByteArray orig("Predictor1 test");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto dp = makeDictObj({{"Predictor", PdfObject::makeInteger(1, 0, 0)}});
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodePredictorAbsent() {
    PdfStreamDecoder decoder;
    QByteArray orig("No predictor");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}});
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodePredictor2() {
    PdfStreamDecoder decoder;
    QByteArray compressed = PdfTestHelpers::zlibCompress(QByteArray("x"));
    QVERIFY(!compressed.isEmpty());

    auto dp = makeDictObj({{"Predictor", PdfObject::makeInteger(2, 0, 0)}});
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedPredictor);
}

void PdfStreamDecoderTest::testDecodePredictor0() {
    PdfStreamDecoder decoder;
    QByteArray compressed = PdfTestHelpers::zlibCompress(QByteArray("x"));
    QVERIFY(!compressed.isEmpty());

    auto dp = makeDictObj({{"Predictor", PdfObject::makeInteger(0, 0, 0)}});
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedPredictor);
}

void PdfStreamDecoderTest::testDecodePredictorNegative() {
    PdfStreamDecoder decoder;
    QByteArray compressed = PdfTestHelpers::zlibCompress(QByteArray("x"));
    QVERIFY(!compressed.isEmpty());

    auto dp = makeDictObj({{"Predictor", PdfObject::makeInteger(-1, 0, 0)}});
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedPredictor);
}

void PdfStreamDecoderTest::testDecodePredictorName() {
    PdfStreamDecoder decoder;
    QByteArray compressed = PdfTestHelpers::zlibCompress(QByteArray("x"));
    QVERIFY(!compressed.isEmpty());

    auto dp = makeDictObj({{"Predictor", PdfObject::makeName(QByteArray("Foo"), 0, 0)}});
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedPredictor);
}

// Non-dict / null decode parms

void PdfStreamDecoderTest::testDecodeNonDictDecodeParms() {
    PdfStreamDecoder decoder;
    QByteArray orig("Non-dict parms");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto dp = PdfObject::makeInteger(42, 0, 0);  // Integer, not a Dictionary
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeNullDecodeParms() {
    PdfStreamDecoder decoder;
    QByteArray orig("Null parms");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto dp = PdfObject::makeNull(0, 0);
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

// Max output zero

void PdfStreamDecoderTest::testDecodeMaxOutputZeroEmptyIdentity() {
    PdfStreamDecoder decoder(0);
    auto s = makeStream(QByteArrayView(""));
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QVERIFY(r.decodedBytes.isEmpty());
}

void PdfStreamDecoderTest::testDecodeMaxOutputZeroNonEmptyIdentity() {
    PdfStreamDecoder decoder(0);
    auto s = makeStream(QByteArrayView("A"));
    auto r = decoder.decode(s);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfStreamDecoderWarning::OutputTooLarge);
}

// DecodeParms array variants

void PdfStreamDecoderTest::testDecodeDecodeParmsArraySingular() {
    PdfStreamDecoder decoder;
    QByteArray orig("DP array singular");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto inner = makeDictObj({{"Predictor", PdfObject::makeInteger(1, 0, 0)}});
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(inner);
    auto dp = PdfObject::makeArray(arr, 0, 0);

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeDecodeParmsArrayUnsupportedPredictor() {
    PdfStreamDecoder decoder;
    QByteArray compressed = PdfTestHelpers::zlibCompress(QByteArray("x"));
    QVERIFY(!compressed.isEmpty());

    auto inner = makeDictObj({{"Predictor", PdfObject::makeInteger(2, 0, 0)}});
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(inner);
    auto dp = PdfObject::makeArray(arr, 0, 0);

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedPredictor);
}

void PdfStreamDecoderTest::testDecodeDecodeParmsArrayMultiElement() {
    PdfStreamDecoder decoder;
    QByteArray compressed = PdfTestHelpers::zlibCompress(QByteArray("x"));
    QVERIFY(!compressed.isEmpty());

    auto p1 = makeDictObj({{"Predictor", PdfObject::makeInteger(1, 0, 0)}});
    auto p2 = makeDictObj({{"Predictor", PdfObject::makeInteger(1, 0, 0)}});
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(p1);
    arr->elements.append(p2);
    auto dp = PdfObject::makeArray(arr, 0, 0);

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedPredictor);
}

// Filter array + DecodeParms

void PdfStreamDecoderTest::testDecodeFilterArrayDecodeParmsDict() {
    PdfStreamDecoder decoder;
    QByteArray orig("Filter array dict DP");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto dp = makeDictObj({{"Predictor", PdfObject::makeInteger(1, 0, 0)}});
    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeFilterArrayDecodeParmsArraySingular() {
    PdfStreamDecoder decoder;
    QByteArray orig("Filter array singular DP array");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto inner = makeDictObj({{"Predictor", PdfObject::makeInteger(1, 0, 0)}});
    auto dpArr = QSharedPointer<PdfArray>::create();
    dpArr->elements.append(inner);
    auto dp = PdfObject::makeArray(dpArr, 0, 0);

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

void PdfStreamDecoderTest::testDecodeFilterArrayDecodeParmsMultiArray() {
    PdfStreamDecoder decoder;
    QByteArray compressed = PdfTestHelpers::zlibCompress(QByteArray("x"));
    QVERIFY(!compressed.isEmpty());

    auto p1 = makeDictObj({{"Predictor", PdfObject::makeInteger(1, 0, 0)}});
    auto p2 = makeDictObj({{"Predictor", PdfObject::makeInteger(1, 0, 0)}});
    auto dpArr = QSharedPointer<PdfArray>::create();
    dpArr->elements.append(p1);
    dpArr->elements.append(p2);
    auto dp = PdfObject::makeArray(dpArr, 0, 0);

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, dp);
    auto r = decoder.decode(s);
    QCOMPARE(r.warning, PdfStreamDecoderWarning::UnsupportedPredictor);
}

void PdfStreamDecoderTest::testDecodeDecodeParmsAbsentKey() {
    PdfStreamDecoder decoder;
    QByteArray orig("No DP key");
    QByteArray compressed = PdfTestHelpers::zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    auto s = makeStream(QByteArrayView(compressed), {{"FlateDecode"}}, nullptr);
    auto r = decoder.decode(s);
    QVERIFY(r.ok());
    QCOMPARE(r.decodedBytes, orig);
}

QTEST_MAIN(PdfStreamDecoderTest)
#include "PdfStreamDecoderTest.moc"
