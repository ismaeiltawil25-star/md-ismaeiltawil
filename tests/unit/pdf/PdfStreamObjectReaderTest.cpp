#include <QtTest/QTest>
#include "PdfByteReader.h"
#include "PdfStreamObjectReader.h"

static PdfObjectPtr makeDict(std::initializer_list<std::pair<const char *, PdfObjectPtr>> entries) {
    auto dict = QSharedPointer<PdfDictionary>::create();
    for (const auto &e : entries) {
        dict->entries.insert(QByteArray(e.first), e.second);
    }
    return PdfObject::makeDictionary(dict, 0, 0);
}

static PdfObjectPtr makeRef(int objNum, int genNum) {
    return PdfObject::makeReference(PdfRef{objNum, genNum}, 0, 0);
}

class PdfStreamObjectReaderTest : public QObject {
    Q_OBJECT

private slots:
    // Success
    void testReadDirectLength();
    void testReadZeroLength();
    void testReadFilterName();
    void testReadFilterArray();
    void testReadFilterMixedArray();
    void testReadDecodeParmsPresent();
    void testReadDecodeParmsAbsent();
    void testReadFilterAbsent();
    void testReadIdentityProvided();
    void testReadIdentityDefault();
    void testReadIdentityExplicitZero();
    void testReadPreservesDictionary();
    void testReadBodyContainsStreamKeyword();
    void testReadBodyContainsEndstreamKeyword();

    // Dictionary validation
    void testReadNullDictionary();
    void testReadInvalidDictionary();

    // Length validation
    void testReadMissingLength();
    void testReadNegativeLength();
    void testReadInvalidLengthTypeReal();
    void testReadInvalidLengthTypeName();
    void testReadInvalidLengthTypeString();
    void testReadInvalidLengthTypeBool();
    void testReadInvalidLengthTypeNull();
    void testReadUnsupportedIndirectLength();
    void testReadLengthOutOfRange();

    // Stream offset
    void testReadInvalidStreamOffsetNegative();
    void testReadInvalidStreamOffsetBeyondSize();

    // Body reader failures
    void testReadBodyReaderMissingStreamKeyword();
    void testReadBodyReaderDataTooShort();
    void testReadBodyReaderMissingEndstream();
    void testReadBodyReaderInvalidEndstreamBoundary();

    // Metadata preserved on length failure
    void testReadMetadataPreservedOnLengthFailure();

    // Zero-length missing endstream
    void testReadZeroLengthMissingEndstream();
};

// Success

void PdfStreamObjectReaderTest::testReadDirectLength() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.stream.rawBody, QByteArrayView("AAAAA"));
    QCOMPARE(result.stream.length, qsizetype(5));
    QVERIFY(result.stream.filters.isEmpty());
}

void PdfStreamObjectReaderTest::testReadZeroLength() {
    QByteArray data("stream\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(0, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QVERIFY(result.stream.rawBody.isEmpty());
    QCOMPARE(result.stream.length, qsizetype(0));
}

void PdfStreamObjectReaderTest::testReadFilterName() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({
        {"Length", PdfObject::makeInteger(5, 0, 0)},
        {"Filter", PdfObject::makeName(QByteArray("FlateDecode"), 0, 0)}
    });

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.stream.filters.size(), 1);
    QCOMPARE(result.stream.filters[0], QByteArray("FlateDecode"));
}

void PdfStreamObjectReaderTest::testReadFilterArray() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeName(QByteArray("ASCII85Decode"), 0, 0));
    arr->elements.append(PdfObject::makeName(QByteArray("FlateDecode"), 0, 0));
    auto filterValue = PdfObject::makeArray(arr, 0, 0);

    auto dict = makeDict({
        {"Length", PdfObject::makeInteger(5, 0, 0)},
        {"Filter", filterValue}
    });

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.stream.filters.size(), 2);
    QCOMPARE(result.stream.filters[0], QByteArray("ASCII85Decode"));
    QCOMPARE(result.stream.filters[1], QByteArray("FlateDecode"));
}

void PdfStreamObjectReaderTest::testReadFilterMixedArray() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeName(QByteArray("FlateDecode"), 0, 0));
    arr->elements.append(PdfObject::makeInteger(42, 0, 0));
    arr->elements.append(PdfObject::makeName(QByteArray("ASCII85Decode"), 0, 0));
    auto filterValue = PdfObject::makeArray(arr, 0, 0);

    auto dict = makeDict({
        {"Length", PdfObject::makeInteger(5, 0, 0)},
        {"Filter", filterValue}
    });

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.stream.filters.size(), 2);
    QCOMPARE(result.stream.filters[0], QByteArray("FlateDecode"));
    QCOMPARE(result.stream.filters[1], QByteArray("ASCII85Decode"));
}

void PdfStreamObjectReaderTest::testReadDecodeParmsPresent() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto parms = QSharedPointer<PdfDictionary>::create();
    parms->entries.insert("Columns", PdfObject::makeInteger(4, 0, 0));
    auto parmsObj = PdfObject::makeDictionary(parms, 0, 0);

    auto dict = makeDict({
        {"Length", PdfObject::makeInteger(5, 0, 0)},
        {"DecodeParms", parmsObj}
    });

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QVERIFY(result.stream.decodeParms != nullptr);
    QCOMPARE(result.stream.decodeParms->kind, PdfObjectKind::Dictionary);
}

void PdfStreamObjectReaderTest::testReadDecodeParmsAbsent() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QVERIFY(result.stream.decodeParms == nullptr);
}

void PdfStreamObjectReaderTest::testReadFilterAbsent() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QVERIFY(result.stream.filters.isEmpty());
}

void PdfStreamObjectReaderTest::testReadIdentityProvided() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0, PdfRef{12, 0});
    QVERIFY(result.ok());
    QVERIFY(result.stream.hasIdentity);
    QCOMPARE(result.stream.identity.objNum, 12);
    QCOMPARE(result.stream.identity.genNum, 0);
}

void PdfStreamObjectReaderTest::testReadIdentityDefault() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QVERIFY(!result.stream.hasIdentity);
}

void PdfStreamObjectReaderTest::testReadIdentityExplicitZero() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0, PdfRef{0, 0});
    QVERIFY(result.ok());
    QVERIFY(!result.stream.hasIdentity);
}

void PdfStreamObjectReaderTest::testReadPreservesDictionary() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.stream.dictionary, dict);
}

void PdfStreamObjectReaderTest::testReadBodyContainsStreamKeyword() {
    QByteArray data("stream\nAAstreamAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(11, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.stream.rawBody, QByteArrayView("AAstreamAA\n"));
}

void PdfStreamObjectReaderTest::testReadBodyContainsEndstreamKeyword() {
    QByteArray data("stream\nAAendstreamAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(13, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
    QCOMPARE(result.stream.rawBody, QByteArrayView("AAendstreamAA"));
}

// Dictionary validation

void PdfStreamObjectReaderTest::testReadNullDictionary() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);

    auto result = PdfStreamObjectReader::read(reader, nullptr, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::NullDictionary);
}

void PdfStreamObjectReaderTest::testReadInvalidDictionary() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto intObj = PdfObject::makeInteger(42, 0, 0);

    auto result = PdfStreamObjectReader::read(reader, intObj, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::InvalidDictionary);
}

// Length validation

void PdfStreamObjectReaderTest::testReadMissingLength() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Type", PdfObject::makeName(QByteArray("XObject"), 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::MissingLength);
    QVERIFY(result.stream.rawBody.isEmpty());
}

void PdfStreamObjectReaderTest::testReadNegativeLength() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(-5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::NegativeLength);
}

void PdfStreamObjectReaderTest::testReadInvalidLengthTypeReal() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeReal(3.14, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::InvalidLengthType);
}

void PdfStreamObjectReaderTest::testReadInvalidLengthTypeName() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeName(QByteArray("Foo"), 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::InvalidLengthType);
}

void PdfStreamObjectReaderTest::testReadInvalidLengthTypeString() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeString(QByteArray("abc"), 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::InvalidLengthType);
}

void PdfStreamObjectReaderTest::testReadInvalidLengthTypeBool() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeBool(true, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::InvalidLengthType);
}

void PdfStreamObjectReaderTest::testReadInvalidLengthTypeNull() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeNull(0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::InvalidLengthType);
}

void PdfStreamObjectReaderTest::testReadUnsupportedIndirectLength() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", makeRef(5, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::UnsupportedIndirectLength);
}

void PdfStreamObjectReaderTest::testReadLengthOutOfRange() {
    // The LengthOutOfRange guard exists in PdfStreamObjectReader for platforms
    // where qsizetype is narrower than int64_t. On this 64-bit platform,
    // qsizetype == int64_t, so no in-range int64_t can exceed qsizetype.
    // The guard is present in code but cannot be triggered here.
    QSKIP("LengthOutOfRange is platform-guard only: qsizetype == int64_t on this platform");
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});
    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(result.ok());
}

// Stream offset

void PdfStreamObjectReaderTest::testReadInvalidStreamOffsetNegative() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, -1);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::InvalidStreamOffset);
}

void PdfStreamObjectReaderTest::testReadInvalidStreamOffsetBeyondSize() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, data.size() + 10);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::InvalidStreamOffset);
}

// Body reader failures

void PdfStreamObjectReaderTest::testReadBodyReaderMissingStreamKeyword() {
    QByteArray data("hello\nbody\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(4, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::BodyReaderFailure);
    QCOMPARE(result.bodyWarning, PdfStreamBodyReaderWarning::MissingStreamKeyword);
    QVERIFY(result.stream.rawBody.isEmpty());
}

void PdfStreamObjectReaderTest::testReadBodyReaderDataTooShort() {
    QByteArray data("stream\nAA");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::BodyReaderFailure);
    QCOMPARE(result.bodyWarning, PdfStreamBodyReaderWarning::DataTooShort);
    QVERIFY(result.stream.rawBody.isEmpty());
}

void PdfStreamObjectReaderTest::testReadBodyReaderMissingEndstream() {
    QByteArray data("stream\nAAAAA");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::BodyReaderFailure);
    QCOMPARE(result.bodyWarning, PdfStreamBodyReaderWarning::MissingEndstream);
    QCOMPARE(result.stream.rawBody, QByteArrayView("AAAAA"));
}

void PdfStreamObjectReaderTest::testReadBodyReaderInvalidEndstreamBoundary() {
    QByteArray data("stream\nAAAAA endstream");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::BodyReaderFailure);
    QCOMPARE(result.bodyWarning, PdfStreamBodyReaderWarning::InvalidEndstreamBoundary);
    QCOMPARE(result.stream.rawBody, QByteArrayView("AAAAA"));
}

// Metadata preserved on length failure

void PdfStreamObjectReaderTest::testReadMetadataPreservedOnLengthFailure() {
    QByteArray data("stream\nAAAAA\nendstream");
    PdfByteReader reader(data);
    auto dict = makeDict({
        {"Filter", PdfObject::makeName(QByteArray("FlateDecode"), 0, 0)}
    });

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::MissingLength);
    QCOMPARE(result.stream.filters.size(), 1);
    QCOMPARE(result.stream.filters[0], QByteArray("FlateDecode"));
}

// Zero-length missing endstream

void PdfStreamObjectReaderTest::testReadZeroLengthMissingEndstream() {
    QByteArray data("stream\n");
    PdfByteReader reader(data);
    auto dict = makeDict({{"Length", PdfObject::makeInteger(0, 0, 0)}});

    auto result = PdfStreamObjectReader::read(reader, dict, 0);
    QVERIFY(!result.ok());
    QCOMPARE(result.warning, PdfStreamObjectReaderWarning::BodyReaderFailure);
    QCOMPARE(result.bodyWarning, PdfStreamBodyReaderWarning::MissingEndstream);
    QVERIFY(result.stream.rawBody.isEmpty());
}

QTEST_MAIN(PdfStreamObjectReaderTest)
#include "PdfStreamObjectReaderTest.moc"
