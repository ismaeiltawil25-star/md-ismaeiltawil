#include <QtTest/QTest>
#include "PdfRawStreamStore.h"

class PdfRawStreamStoreTest : public QObject {
    Q_OBJECT

private slots:
    void testAddAndGet();
    void testAddAndGetByRef();
    void testContains();
    void testGetMissing();
    void testCount();
    void testClear();
    void testIsEmpty();
    void testOverwriteDuplicate();
    void testInvalidKeyZeroObjNum();
    void testInvalidKeyNegativeGenNum();
    void testRetrievedBodyMatchesOriginalBuffer();
    void testStoreDoesNotOwnBytes();
};

void PdfRawStreamStoreTest::testAddAndGet() {
    PdfRawStreamStore store;
    PdfRawStreamObject stream;
    stream.length = 5;
    auto result = store.add(5, 0, stream);
    QVERIFY(result.stored);
    QCOMPARE(result.warning, PdfRawStreamStoreWarning::None);

    const PdfRawStreamObject *retrieved = store.get(5, 0);
    QVERIFY(retrieved != nullptr);
    QCOMPARE(retrieved->length, qsizetype(5));
}

void PdfRawStreamStoreTest::testAddAndGetByRef() {
    PdfRawStreamStore store;
    PdfRawStreamObject stream;
    stream.length = 42;
    store.add(PdfRef{3, 0}, stream);

    const PdfRawStreamObject *r1 = store.get(3, 0);
    const PdfRawStreamObject *r2 = store.get(PdfRef{3, 0});
    QVERIFY(r1 != nullptr);
    QVERIFY(r2 != nullptr);
    QCOMPARE(r1->length, r2->length);
}

void PdfRawStreamStoreTest::testContains() {
    PdfRawStreamStore store;
    QVERIFY(!store.contains(1, 0));

    PdfRawStreamObject stream;
    store.add(1, 0, stream);
    QVERIFY(store.contains(1, 0));
    QVERIFY(!store.contains(2, 0));
}

void PdfRawStreamStoreTest::testGetMissing() {
    PdfRawStreamStore store;
    const PdfRawStreamObject *retrieved = store.get(99, 0);
    QVERIFY(retrieved == nullptr);
}

void PdfRawStreamStoreTest::testCount() {
    PdfRawStreamStore store;
    QCOMPARE(store.count(), qsizetype(0));

    PdfRawStreamObject s;
    store.add(1, 0, s);
    store.add(2, 0, s);
    store.add(3, 0, s);
    QCOMPARE(store.count(), qsizetype(3));
}

void PdfRawStreamStoreTest::testClear() {
    PdfRawStreamStore store;
    PdfRawStreamObject s;
    store.add(1, 0, s);
    store.add(2, 0, s);
    QCOMPARE(store.count(), qsizetype(2));

    store.clear();
    QCOMPARE(store.count(), qsizetype(0));
    QVERIFY(store.isEmpty());
}

void PdfRawStreamStoreTest::testIsEmpty() {
    PdfRawStreamStore store;
    QVERIFY(store.isEmpty());

    PdfRawStreamObject s;
    store.add(1, 0, s);
    QVERIFY(!store.isEmpty());
}

void PdfRawStreamStoreTest::testOverwriteDuplicate() {
    PdfRawStreamStore store;
    PdfRawStreamObject s1;
    s1.length = 10;
    store.add(1, 0, s1);

    PdfRawStreamObject s2;
    s2.length = 20;
    auto result = store.add(1, 0, s2);
    QVERIFY(result.stored);
    QCOMPARE(result.warning, PdfRawStreamStoreWarning::DuplicateStreamKey);

    const PdfRawStreamObject *retrieved = store.get(1, 0);
    QVERIFY(retrieved != nullptr);
    QCOMPARE(retrieved->length, qsizetype(20));
    QCOMPARE(store.count(), qsizetype(1));
}

void PdfRawStreamStoreTest::testInvalidKeyZeroObjNum() {
    PdfRawStreamStore store;
    PdfRawStreamObject stream;
    auto result = store.add(0, 0, stream);
    QVERIFY(!result.stored);
    QCOMPARE(result.warning, PdfRawStreamStoreWarning::InvalidStreamKey);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfRawStreamStoreTest::testInvalidKeyNegativeGenNum() {
    PdfRawStreamStore store;
    PdfRawStreamObject stream;
    auto result = store.add(1, -1, stream);
    QVERIFY(!result.stored);
    QCOMPARE(result.warning, PdfRawStreamStoreWarning::InvalidStreamKey);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfRawStreamStoreTest::testRetrievedBodyMatchesOriginalBuffer() {
    QByteArray data("AAAAA");
    PdfRawStreamStore store;
    PdfRawStreamObject stream;
    stream.rawBody = QByteArrayView(data);
    store.add(1, 0, stream);

    const PdfRawStreamObject *retrieved = store.get(1, 0);
    QVERIFY(retrieved != nullptr);
    QCOMPARE(retrieved->rawBody, QByteArrayView("AAAAA"));
}

void PdfRawStreamStoreTest::testStoreDoesNotOwnBytes() {
    QByteArray data("test");
    PdfRawStreamStore store;
    PdfRawStreamObject stream;
    stream.rawBody = QByteArrayView(data);
    store.add(1, 0, stream);

    const PdfRawStreamObject *retrieved = store.get(1, 0);
    QVERIFY(retrieved != nullptr);
    // Body is a non-owning view — it points into the original data
    QVERIFY(retrieved->rawBody.data() == data.data());
}

QTEST_MAIN(PdfRawStreamStoreTest)
#include "PdfRawStreamStoreTest.moc"
