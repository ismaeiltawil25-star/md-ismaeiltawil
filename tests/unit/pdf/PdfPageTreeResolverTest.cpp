#include <QtTest/QTest>
#include "PdfPageTreeResolver.h"

class PdfPageTreeResolverTest : public QObject {
    Q_OBJECT

private:
    void addPage(PdfObjectStore &store, int objNum, int genNum);
    void addPages(PdfObjectStore &store, int objNum, int genNum,
                  QList<PdfRef> kids, int count = -1);

private slots:
    void testResolveSinglePage();
    void testResolveMultiplePages();
    void testResolveNestedPages();
    void testResolveMixedTree();
    void testResolveCountMatches();
    void testResolveCountMismatch();
    void testResolveMissingCount();
    void testResolveCountWrongType();
    void testResolveCountNegative();
    void testResolveCountZero();
    void testResolveEmptyKidsWithoutCount();
    void testResolveMissingTypeTolerated();
    void testResolveInvalidType();
    void testResolveRootMissingKids();
    void testResolveRootNoKidsNoType();
    void testResolveInvalidKids();
    void testResolveInvalidKidReference();
    void testResolveKidRefZeroObj();
    void testResolveUnresolvedKid();
    void testResolveKidNotDict();
    void testResolveCycleSelfRef();
    void testResolveCycleInPath();
    void testResolveSharedObjectNotCycle();
    void testResolveDepthExceeded();
    void testResolveCountWarningBlocked();
};

void PdfPageTreeResolverTest::addPage(PdfObjectStore &store, int objNum, int genNum) {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName(QByteArray("Page"), 0, 0));
    auto obj = PdfObject::makeDictionary(dict, 0, 0);
    store.add(objNum, genNum, obj);
}

void PdfPageTreeResolverTest::addPages(PdfObjectStore &store, int objNum, int genNum,
                                        QList<PdfRef> kids, int count) {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName(QByteArray("Pages"), 0, 0));
    if (count >= 0) dict->entries.insert("Count", PdfObject::makeInteger(count, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    for (auto &ref : kids)
        arr->elements.append(PdfObject::makeReference(ref, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    auto obj = PdfObject::makeDictionary(dict, 0, 0);
    store.add(objNum, genNum, obj);
}

void PdfPageTreeResolverTest::testResolveSinglePage() {
    PdfObjectStore store;
    addPages(store, 1, 0, {{2, 0}}, 1);
    addPage(store, 2, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    PdfPageTreeResolver resolver;
    auto r = resolver.resolve(catalog, store);
    QVERIFY(r.ok());
    QCOMPARE(r.tree.actualCount, 1);
    QCOMPARE(r.tree.pageRefs[0].objNum, 2);
    QCOMPARE(r.tree.declaredCount, 1);
}

void PdfPageTreeResolverTest::testResolveMultiplePages() {
    PdfObjectStore store;
    addPages(store, 1, 0, {{2, 0}, {3, 0}, {4, 0}}, 3);
    addPage(store, 2, 0); addPage(store, 3, 0); addPage(store, 4, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QVERIFY(r.ok());
    QCOMPARE(r.tree.actualCount, 3);
}

void PdfPageTreeResolverTest::testResolveNestedPages() {
    PdfObjectStore store;
    addPages(store, 2, 0, {{3, 0}}, 1);
    addPages(store, 1, 0, {{2, 0}}, 1);
    addPage(store, 3, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.tree.actualCount, 1);
}

void PdfPageTreeResolverTest::testResolveMixedTree() {
    PdfObjectStore store;
    addPages(store, 3, 0, {{4, 0}, {5, 0}}, 2);
    addPages(store, 1, 0, {{2, 0}, {3, 0}, {6, 0}}, 4);
    addPage(store, 2, 0); addPage(store, 4, 0); addPage(store, 5, 0); addPage(store, 6, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.tree.actualCount, 4);
    QCOMPARE(r.tree.pageRefs[0].objNum, 2);
    QCOMPARE(r.tree.pageRefs[1].objNum, 4);
    QCOMPARE(r.tree.pageRefs[2].objNum, 5);
    QCOMPARE(r.tree.pageRefs[3].objNum, 6);
}

void PdfPageTreeResolverTest::testResolveCountMatches() {
    PdfObjectStore store;
    addPages(store, 1, 0, {{2, 0}}, 1);
    addPage(store, 2, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QVERIFY(r.ok());
    QCOMPARE(r.tree.declaredCount, 1);
    QCOMPARE(r.tree.actualCount, 1);
}

void PdfPageTreeResolverTest::testResolveCountMismatch() {
    PdfObjectStore store;
    addPages(store, 1, 0, {{2, 0}}, 5);
    addPage(store, 2, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::CountMismatch);
    QCOMPARE(r.tree.actualCount, 1);
    QCOMPARE(r.tree.declaredCount, 5);
}

void PdfPageTreeResolverTest::testResolveMissingCount() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{2, 0}, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    addPage(store, 2, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::MissingCount);
    QCOMPARE(r.tree.actualCount, 1);
}

void PdfPageTreeResolverTest::testResolveCountWrongType() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeName("Wrong", 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{2, 0}, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    addPage(store, 2, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::InvalidCount);
}

void PdfPageTreeResolverTest::testResolveCountNegative() {
    PdfObjectStore store;
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeInteger(-1, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{2, 0}, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    addPage(store, 2, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::InvalidCount);
}

void PdfPageTreeResolverTest::testResolveCountZero() {
    PdfObjectStore store;
    addPages(store, 1, 0, {}, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QVERIFY(r.ok());
    QCOMPARE(r.tree.actualCount, 0);
    QCOMPARE(r.tree.declaredCount, 0);
}

void PdfPageTreeResolverTest::testResolveEmptyKidsWithoutCount() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(QSharedPointer<PdfArray>::create(), 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::MissingCount);
    QCOMPARE(r.tree.actualCount, 0);
}

void PdfPageTreeResolverTest::testResolveMissingTypeTolerated() {
    auto rootDict = QSharedPointer<PdfDictionary>::create();
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{2, 0}, 0, 0));
    rootDict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    rootDict->entries.insert("Count", PdfObject::makeInteger(1, 0, 0));

    auto pageDict = QSharedPointer<PdfDictionary>::create();
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(rootDict, 0, 0));
    store.add(2, 0, PdfObject::makeDictionary(pageDict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QVERIFY(r.ok());
    QCOMPARE(r.tree.actualCount, 1);
}

void PdfPageTreeResolverTest::testResolveInvalidType() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Catalog", 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(QSharedPointer<PdfArray>::create(), 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::InvalidPageType);
}

void PdfPageTreeResolverTest::testResolveRootMissingKids() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::MissingKids);
}

void PdfPageTreeResolverTest::testResolveRootNoKidsNoType() {
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(QSharedPointer<PdfDictionary>::create(), 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::MissingKids);
}

void PdfPageTreeResolverTest::testResolveInvalidKids() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Kids", PdfObject::makeInteger(42, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::InvalidKids);
}

void PdfPageTreeResolverTest::testResolveInvalidKidReference() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeInteger(1, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeInteger(42, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::InvalidKidReference);
}

void PdfPageTreeResolverTest::testResolveKidRefZeroObj() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeInteger(0, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{0, 0}, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::InvalidKidReference);
}

void PdfPageTreeResolverTest::testResolveUnresolvedKid() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeInteger(0, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{99, 0}, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::UnresolvedPageNode);
}

void PdfPageTreeResolverTest::testResolveKidNotDict() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeInteger(0, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{2, 0}, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    store.add(2, 0, PdfObject::makeInteger(42, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::InvalidPageNode);
}

void PdfPageTreeResolverTest::testResolveCycleSelfRef() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeInteger(1, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{1, 0}, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::CycleDetected);
}

void PdfPageTreeResolverTest::testResolveCycleInPath() {
    auto dictA = QSharedPointer<PdfDictionary>::create();
    dictA->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dictA->entries.insert("Count", PdfObject::makeInteger(1, 0, 0));
    auto arrA = QSharedPointer<PdfArray>::create();
    arrA->elements.append(PdfObject::makeReference(PdfRef{2, 0}, 0, 0));
    dictA->entries.insert("Kids", PdfObject::makeArray(arrA, 0, 0));

    auto dictB = QSharedPointer<PdfDictionary>::create();
    dictB->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    auto arrB = QSharedPointer<PdfArray>::create();
    arrB->elements.append(PdfObject::makeReference(PdfRef{1, 0}, 0, 0));
    dictB->entries.insert("Kids", PdfObject::makeArray(arrB, 0, 0));

    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dictA, 0, 0));
    store.add(2, 0, PdfObject::makeDictionary(dictB, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::CycleDetected);
}

void PdfPageTreeResolverTest::testResolveSharedObjectNotCycle() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeInteger(2, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeReference(PdfRef{2, 0}, 0, 0));
    arr->elements.append(PdfObject::makeReference(PdfRef{2, 0}, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));

    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    addPage(store, 2, 0);
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QVERIFY(r.ok());
    QCOMPARE(r.tree.actualCount, 2);
}

void PdfPageTreeResolverTest::testResolveDepthExceeded() {
    PdfObjectStore store;
    int refNum = 2;
    for (int d = 0; d < 130; ++d) {
        auto dict = QSharedPointer<PdfDictionary>::create();
        dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
        auto arr = QSharedPointer<PdfArray>::create();
        arr->elements.append(PdfObject::makeReference(PdfRef{refNum, 0}, 0, 0));
        dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
        store.add(refNum - 1, 0, PdfObject::makeDictionary(dict, 0, 0));
        refNum++;
    }
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::DepthExceeded);
}

void PdfPageTreeResolverTest::testResolveCountWarningBlocked() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName("Pages", 0, 0));
    dict->entries.insert("Count", PdfObject::makeInteger(999, 0, 0));
    auto arr = QSharedPointer<PdfArray>::create();
    arr->elements.append(PdfObject::makeInteger(42, 0, 0));
    dict->entries.insert("Kids", PdfObject::makeArray(arr, 0, 0));
    PdfObjectStore store;
    store.add(1, 0, PdfObject::makeDictionary(dict, 0, 0));
    PdfCatalog catalog; catalog.pagesRef = PdfRef{1, 0};
    auto r = PdfPageTreeResolver().resolve(catalog, store);
    QCOMPARE(r.warning, PdfPageTreeResolverWarning::InvalidKidReference);
}

QTEST_MAIN(PdfPageTreeResolverTest)
#include "PdfPageTreeResolverTest.moc"
