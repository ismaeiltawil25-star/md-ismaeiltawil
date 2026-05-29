#include <QtTest/QTest>
#include "PdfObjectStore.h"

class PdfObjectStoreTest : public QObject {
    Q_OBJECT

private slots:
    void testEmptyStore();
    void testAddAndResolve();
    void testResolveMissing();
    void testAddMultipleDifferentKeys();
    void testAddSameObjDiffGen();
    void testDuplicateInsertWarns();
    void testAddNullRejected();
    void testAddInvalidNegativeObjNum();
    void testAddInvalidNegativeGenNum();
    void testAddInvalidZeroObjNum();
    void testAddInvalidZeroObjNumPositiveGen();
    void testResolveInvalidNegativeKey();
    void testResolveInvalidZeroKey();
    void testContainsInvalidKey();
    void testRemoveExisting();
    void testRemoveMissingNoOp();
    void testClear();
    void testSelfReference();
};

void PdfObjectStoreTest::testEmptyStore() {
    PdfObjectStore store;
    QCOMPARE(store.count(), qsizetype(0));
    QVERIFY(store.isEmpty());
    QVERIFY(!store.contains(5, 0));

    auto result = store.resolve(5, 0);
    QVERIFY(result.object == nullptr);
    QVERIFY(!result.found());
    QCOMPARE(result.warning, PdfObjectStoreWarning::MissingReference);
}

void PdfObjectStoreTest::testAddAndResolve() {
    PdfObjectStore store;
    auto obj = PdfObject::makeInteger(42, 0, 0);

    auto addResult = store.add(5, 0, obj);
    QVERIFY(addResult.ok());
    QVERIFY(store.contains(5, 0));
    QCOMPARE(store.count(), qsizetype(1));
    QVERIFY(!store.isEmpty());

    auto resolveResult = store.resolve(5, 0);
    QVERIFY(resolveResult.found());
    QVERIFY(resolveResult.object != nullptr);
    QCOMPARE(resolveResult.object->intValue, int64_t(42));
    QCOMPARE(resolveResult.object, obj);
}

void PdfObjectStoreTest::testResolveMissing() {
    PdfObjectStore store;
    auto result = store.resolve(99, 0);
    QVERIFY(result.object == nullptr);
    QVERIFY(!result.found());
    QCOMPARE(result.warning, PdfObjectStoreWarning::MissingReference);
    QVERIFY(!store.contains(99, 0));
}

void PdfObjectStoreTest::testAddMultipleDifferentKeys() {
    PdfObjectStore store;
    auto o1 = PdfObject::makeInteger(1, 0, 0);
    auto o2 = PdfObject::makeInteger(2, 0, 0);
    auto o3 = PdfObject::makeInteger(3, 0, 0);

    store.add(1, 0, o1);
    store.add(2, 0, o2);
    store.add(3, 0, o3);
    QCOMPARE(store.count(), qsizetype(3));

    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(store.resolve(2, 0).found());
    QVERIFY(store.resolve(3, 0).found());
    QCOMPARE(store.resolve(1, 0).object->intValue, int64_t(1));
    QCOMPARE(store.resolve(2, 0).object->intValue, int64_t(2));
    QCOMPARE(store.resolve(3, 0).object->intValue, int64_t(3));
}

void PdfObjectStoreTest::testAddSameObjDiffGen() {
    PdfObjectStore store;
    auto o1 = PdfObject::makeInteger(1, 0, 0);
    auto o2 = PdfObject::makeInteger(2, 0, 0);

    store.add(5, 0, o1);
    store.add(5, 1, o2);
    QCOMPARE(store.count(), qsizetype(2));

    QCOMPARE(store.resolve(5, 0).object->intValue, int64_t(1));
    QCOMPARE(store.resolve(5, 1).object->intValue, int64_t(2));
}

void PdfObjectStoreTest::testDuplicateInsertWarns() {
    PdfObjectStore store;
    auto o1 = PdfObject::makeInteger(1, 0, 0);
    auto o2 = PdfObject::makeInteger(2, 0, 0);

    auto r1 = store.add(5, 0, o1);
    QVERIFY(r1.ok());

    auto r2 = store.add(5, 0, o2);
    QCOMPARE(r2.warning, PdfObjectStoreWarning::DuplicateObjectKey);
    QCOMPARE(store.count(), qsizetype(1));
    QCOMPARE(store.resolve(5, 0).object->intValue, int64_t(2));
}

void PdfObjectStoreTest::testAddNullRejected() {
    PdfObjectStore store;
    auto result = store.add(5, 0, nullptr);
    QCOMPARE(result.warning, PdfObjectStoreWarning::NullObject);
    QCOMPARE(store.count(), qsizetype(0));
    QVERIFY(!store.contains(5, 0));
}

void PdfObjectStoreTest::testAddInvalidNegativeObjNum() {
    PdfObjectStore store;
    auto obj = PdfObject::makeInteger(42, 0, 0);
    auto result = store.add(-1, 0, obj);
    QCOMPARE(result.warning, PdfObjectStoreWarning::InvalidObjectKey);
    QCOMPARE(store.count(), qsizetype(0));
    QVERIFY(!store.contains(-1, 0));
}

void PdfObjectStoreTest::testAddInvalidNegativeGenNum() {
    PdfObjectStore store;
    auto obj = PdfObject::makeInteger(42, 0, 0);
    auto result = store.add(1, -1, obj);
    QCOMPARE(result.warning, PdfObjectStoreWarning::InvalidObjectKey);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfObjectStoreTest::testAddInvalidZeroObjNum() {
    PdfObjectStore store;
    auto obj = PdfObject::makeInteger(42, 0, 0);
    auto result = store.add(0, 0, obj);
    QCOMPARE(result.warning, PdfObjectStoreWarning::InvalidObjectKey);
    QCOMPARE(store.count(), qsizetype(0));
    QVERIFY(!store.contains(0, 0));
}

void PdfObjectStoreTest::testAddInvalidZeroObjNumPositiveGen() {
    PdfObjectStore store;
    auto obj = PdfObject::makeInteger(42, 0, 0);
    auto result = store.add(0, 5, obj);
    QCOMPARE(result.warning, PdfObjectStoreWarning::InvalidObjectKey);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfObjectStoreTest::testResolveInvalidNegativeKey() {
    PdfObjectStore store;
    auto result = store.resolve(-1, 0);
    QVERIFY(result.object == nullptr);
    QVERIFY(!result.found());
    QCOMPARE(result.warning, PdfObjectStoreWarning::InvalidObjectKey);
}

void PdfObjectStoreTest::testResolveInvalidZeroKey() {
    PdfObjectStore store;
    auto result = store.resolve(0, 0);
    QVERIFY(result.object == nullptr);
    QVERIFY(!result.found());
    QCOMPARE(result.warning, PdfObjectStoreWarning::InvalidObjectKey);
}

void PdfObjectStoreTest::testContainsInvalidKey() {
    PdfObjectStore store;
    QVERIFY(!store.contains(-1, 0));
    QVERIFY(!store.contains(0, 0));
    QVERIFY(!store.contains(1, -1));
}

void PdfObjectStoreTest::testRemoveExisting() {
    PdfObjectStore store;
    auto obj = PdfObject::makeInteger(42, 0, 0);
    store.add(5, 0, obj);
    QCOMPARE(store.count(), qsizetype(1));

    store.remove(5, 0);
    QCOMPARE(store.count(), qsizetype(0));
    QVERIFY(!store.resolve(5, 0).found());
}

void PdfObjectStoreTest::testRemoveMissingNoOp() {
    PdfObjectStore store;
    store.remove(99, 0);
    store.remove(0, 0);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfObjectStoreTest::testClear() {
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeInteger(1, 0, 0));
    store.add(2, 0, PdfObject::makeInteger(2, 0, 0));
    store.add(3, 0, PdfObject::makeInteger(3, 0, 0));
    QCOMPARE(store.count(), qsizetype(3));

    store.clear();
    QCOMPARE(store.count(), qsizetype(0));
    QVERIFY(store.isEmpty());
    QVERIFY(!store.resolve(1, 0).found());
    QVERIFY(!store.resolve(2, 0).found());
    QVERIFY(!store.resolve(3, 0).found());
}

void PdfObjectStoreTest::testSelfReference() {
    PdfObjectStore store;
    PdfRef ref;
    ref.objNum = 5;
    ref.genNum = 0;
    auto refObj = PdfObject::makeReference(ref, 0, 0);
    store.add(5, 0, refObj);
    QCOMPARE(store.count(), qsizetype(1));

    auto result = store.resolve(5, 0);
    QVERIFY(result.found());
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Reference);
    QCOMPARE(result.object->refValue.objNum, 5);
    QCOMPARE(result.object->refValue.genNum, 0);
}

QTEST_MAIN(PdfObjectStoreTest)
#include "PdfObjectStoreTest.moc"
