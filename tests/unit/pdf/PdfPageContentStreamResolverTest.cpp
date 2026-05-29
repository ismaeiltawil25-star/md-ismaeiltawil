#include <QtTest/QTest>
#include <QtZlib/zlib.h>
#include "PdfPageContentStreamResolver.h"

static QByteArray zlibCompress(const QByteArray &input) {
    uLongf bound = compressBound(static_cast<uLongf>(input.size()));
    QByteArray compressed(static_cast<qsizetype>(bound), '\0');
    uLongf destLen = bound;
    int ret = compress2(
        reinterpret_cast<Bytef*>(compressed.data()), &destLen,
        reinterpret_cast<const Bytef*>(input.constData()),
        static_cast<uLongf>(input.size()),
        Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) return {};
    compressed.resize(static_cast<qsizetype>(destLen));
    return compressed;
}

static PdfObjectPtr makeDict(std::initializer_list<std::pair<const char *, PdfObjectPtr>> entries) {
    auto dict = QSharedPointer<PdfDictionary>::create();
    for (const auto &e : entries) {
        dict->entries.insert(QByteArray(e.first), e.second);
    }
    return PdfObject::makeDictionary(dict, 0, 0);
}

static PdfObjectPtr makeRef(int objNum, int genNum = 0) {
    return PdfObject::makeReference(PdfRef{objNum, genNum}, 0, 0);
}

static void addStreamToStore(PdfRawStreamStore &streamStore, int objNum, int genNum,
                              const QByteArray &rawData, const QList<QByteArray> &filters = {}) {
    PdfRawStreamObject s;
    s.rawBody = QByteArrayView(rawData);
    s.filters = filters;
    s.length = rawData.size();
    streamStore.add(objNum, genNum, s);
}

class PdfPageContentStreamResolverTest : public QObject {
    Q_OBJECT

private slots:
    void testResolveSingleStream();
    void testResolveArrayOfTwoStreams();
    void testResolveArrayPreservesOrder();
    void testResolveMissingContents();
    void testResolveEmptyArray();
    void testResolveIdentityStream();
    void testResolveInvalidPageIndexNegative();
    void testResolveInvalidPageIndexBeyondCount();
    void testResolvePageNotFound();
    void testResolvePageNotDictionary();
    void testResolveInvalidContentsTypeName();
    void testResolveInvalidContentsTypeDict();
    void testResolveUnresolvedContentReference();
    void testResolveContentNotStream();
    void testResolveRawStreamNotFound();
    void testResolveDecodeFailureCorrupt();
    void testResolveDecodeFailureUnsupportedFilter();
    void testResolveInvalidContentsEntry();
    void testResolveArrayFailWholePage();
    void testResolvePageIndexAndRef();
};

void PdfPageContentStreamResolverTest::testResolveSingleStream() {
    QByteArray orig("BT /F1 12 Tf (Hello) Tj ET");
    QByteArray compressed = zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", makeRef(10, 0)}});
    objStore.add(1, 0, pageDict);
    auto contentDict = makeDict({});
    objStore.add(10, 0, contentDict);
    addStreamToStore(streamStore, 10, 0, compressed, {{"FlateDecode"}});

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(r.ok());
    QCOMPARE(r.contentBytes, orig);
    QCOMPARE(r.pageIndex, 0);
    QCOMPARE(r.pageRef.objNum, 1);
}

void PdfPageContentStreamResolverTest::testResolveArrayOfTwoStreams() {
    QByteArray part1("BT /F1 12 Tf ");
    QByteArray part2("(Hello) Tj ET");
    QByteArray c1 = zlibCompress(part1);
    QByteArray c2 = zlibCompress(part2);
    QVERIFY(!c1.isEmpty() && !c2.isEmpty());

    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(makeRef(10, 0));
    arr->elements.append(makeRef(11, 0));

    auto pageDict = makeDict({{"Contents", PdfObject::makeArray(arr, 0, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, makeDict({}));
    objStore.add(11, 0, makeDict({}));
    addStreamToStore(streamStore, 10, 0, c1, {{"FlateDecode"}});
    addStreamToStore(streamStore, 11, 0, c2, {{"FlateDecode"}});

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(r.ok());
    QCOMPARE(r.contentBytes, part1 + part2);
}

void PdfPageContentStreamResolverTest::testResolveArrayPreservesOrder() {
    QByteArray first("first");
    QByteArray second("second");
    QByteArray c1 = zlibCompress(first);
    QByteArray c2 = zlibCompress(second);
    QVERIFY(!c1.isEmpty() && !c2.isEmpty());

    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(makeRef(10, 0));
    arr->elements.append(makeRef(11, 0));

    auto pageDict = makeDict({{"Contents", PdfObject::makeArray(arr, 0, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, makeDict({}));
    objStore.add(11, 0, makeDict({}));
    addStreamToStore(streamStore, 10, 0, c1, {{"FlateDecode"}});
    addStreamToStore(streamStore, 11, 0, c2, {{"FlateDecode"}});

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(r.ok());
    QVERIFY(r.contentBytes.startsWith("first"));
    QVERIFY(r.contentBytes.endsWith("second"));
}

void PdfPageContentStreamResolverTest::testResolveMissingContents() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Type", PdfObject::makeName(QByteArray("Page"), 0, 0)}});
    objStore.add(1, 0, pageDict);

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(r.ok());
    QVERIFY(r.contentBytes.isEmpty());
    QCOMPARE(r.pageIndex, 0);
}

void PdfPageContentStreamResolverTest::testResolveEmptyArray() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto arr = QSharedPointer<PdfArray>::create();
    auto pageDict = makeDict({{"Contents", PdfObject::makeArray(arr, 0, 0)}});
    objStore.add(1, 0, pageDict);

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(r.ok());
    QVERIFY(r.contentBytes.isEmpty());
}

void PdfPageContentStreamResolverTest::testResolveIdentityStream() {
    QByteArray orig("BT ET");
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", makeRef(10, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, makeDict({}));
    addStreamToStore(streamStore, 10, 0, orig);  // no filters → identity

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(r.ok());
    QCOMPARE(r.contentBytes, orig);
}

void PdfPageContentStreamResolverTest::testResolveInvalidPageIndexNegative() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;
    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, -1);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::InvalidPageIndex);
    QCOMPARE(r.pageIndex, -1);
}

void PdfPageContentStreamResolverTest::testResolveInvalidPageIndexBeyondCount() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;
    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 1);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::InvalidPageIndex);
    QCOMPARE(r.pageIndex, 1);
}

void PdfPageContentStreamResolverTest::testResolvePageNotFound() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;
    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{99, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::PageNotFound);
}

void PdfPageContentStreamResolverTest::testResolvePageNotDictionary() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    objStore.add(1, 0, PdfObject::makeInteger(42, 0, 0));

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::PageNotDictionary);
}

void PdfPageContentStreamResolverTest::testResolveInvalidContentsTypeName() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", PdfObject::makeName(QByteArray("Foo"), 0, 0)}});
    objStore.add(1, 0, pageDict);

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::InvalidContentsType);
}

void PdfPageContentStreamResolverTest::testResolveInvalidContentsTypeDict() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto inlineDict = makeDict({{"Length", PdfObject::makeInteger(5, 0, 0)}});
    auto pageDict = makeDict({{"Contents", inlineDict}});
    objStore.add(1, 0, pageDict);

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::InvalidContentsType);
}

void PdfPageContentStreamResolverTest::testResolveUnresolvedContentReference() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", makeRef(10, 0)}});
    objStore.add(1, 0, pageDict);

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::UnresolvedContentReference);
}

void PdfPageContentStreamResolverTest::testResolveContentNotStream() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", makeRef(10, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, PdfObject::makeInteger(42, 0, 0));

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::ContentNotStream);
}

void PdfPageContentStreamResolverTest::testResolveRawStreamNotFound() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", makeRef(10, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, makeDict({}));

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::RawStreamNotFound);
}

void PdfPageContentStreamResolverTest::testResolveDecodeFailureCorrupt() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", makeRef(10, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, makeDict({}));
    addStreamToStore(streamStore, 10, 0, QByteArray(100, 'X'), {{"FlateDecode"}});

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::DecodeFailure);
    QCOMPARE(r.decoderWarning, PdfStreamDecoderWarning::DecodeFailure);
}

void PdfPageContentStreamResolverTest::testResolveDecodeFailureUnsupportedFilter() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", makeRef(10, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, makeDict({}));
    addStreamToStore(streamStore, 10, 0, QByteArray("AAAAA"), {{"ASCIIHexDecode"}});

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::DecodeFailure);
    QCOMPARE(r.decoderWarning, PdfStreamDecoderWarning::UnsupportedFilter);
}

void PdfPageContentStreamResolverTest::testResolveInvalidContentsEntry() {
    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(makeRef(10, 0));
    arr->elements.append(PdfObject::makeInteger(42, 0, 0));

    auto pageDict = makeDict({{"Contents", PdfObject::makeArray(arr, 0, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, makeDict({}));
    addStreamToStore(streamStore, 10, 0, QByteArray("valid"));

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::InvalidContentsEntry);
}

void PdfPageContentStreamResolverTest::testResolveArrayFailWholePage() {
    QByteArray part1("BT ET");
    QByteArray c1 = zlibCompress(part1);
    QVERIFY(!c1.isEmpty());

    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(makeRef(10, 0));
    arr->elements.append(makeRef(11, 0));

    auto pageDict = makeDict({{"Contents", PdfObject::makeArray(arr, 0, 0)}});
    objStore.add(1, 0, pageDict);
    objStore.add(10, 0, makeDict({}));
    objStore.add(11, 0, makeDict({}));
    addStreamToStore(streamStore, 10, 0, c1, {{"FlateDecode"}});
    addStreamToStore(streamStore, 11, 0, QByteArray(50, 'Z'), {{"FlateDecode"}});

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.actualCount = 1;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 0);

    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfPageContentStreamResolverWarning::DecodeFailure);
    QVERIFY(r.contentBytes.isEmpty());
    QCOMPARE(r.pageIndex, 0);
    QCOMPARE(r.pageRef.objNum, 1);
}

void PdfPageContentStreamResolverTest::testResolvePageIndexAndRef() {
    QByteArray orig("content");
    QByteArray compressed = zlibCompress(orig);
    QVERIFY(!compressed.isEmpty());

    PdfObjectStore objStore;
    PdfRawStreamStore streamStore;
    PdfStreamDecoder decoder;

    auto pageDict = makeDict({{"Contents", makeRef(10, 0)}});
    objStore.add(5, 0, pageDict);
    objStore.add(10, 0, makeDict({}));
    addStreamToStore(streamStore, 10, 0, compressed, {{"FlateDecode"}});

    PdfPageTree tree;
    tree.pageRefs.append(PdfRef{1, 0});
    tree.pageRefs.append(PdfRef{5, 0});
    tree.actualCount = 2;

    PdfPageContentStreamResolver resolver(objStore, streamStore, decoder);
    auto r = resolver.resolve(tree, 1);

    QVERIFY(r.ok());
    QCOMPARE(r.pageIndex, 1);
    QCOMPARE(r.pageRef.objNum, 5);
    QCOMPARE(r.pageRef.genNum, 0);
}

QTEST_MAIN(PdfPageContentStreamResolverTest)
#include "PdfPageContentStreamResolverTest.moc"
