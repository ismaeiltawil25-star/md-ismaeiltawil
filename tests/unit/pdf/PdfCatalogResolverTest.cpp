#include <QtTest/QTest>
#include "PdfCatalogResolver.h"

class PdfCatalogResolverTest : public QObject {
    Q_OBJECT

private slots:
    void testResolveValidCatalog();
    void testResolveMissingTypeTolerated();
    void testResolveMissingRootRefZeroObj();
    void testResolveUnresolvedRoot();
    void testResolveRootNotDictionary();
    void testResolveInvalidCatalogTypePages();
    void testResolveInvalidCatalogTypeInt();
    void testResolveMissingPages();
    void testResolvePagesNotReference();
    void testResolvePagesZeroObj();
};

void PdfCatalogResolverTest::testResolveValidCatalog() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName(QByteArray("Catalog"), 0, 0));
    dict->entries.insert("Pages", PdfObject::makeReference(PdfRef{3, 0}, 0, 0));
    auto rootObj = PdfObject::makeDictionary(dict, 0, 0);

    PdfObjectStore store;
    store.add(1, 0, rootObj);

    PdfTrailer trailer;
    trailer.root = PdfRef{1, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QVERIFY(r.ok());
    QCOMPARE(r.catalog.pagesRef.objNum, 3);
    QCOMPARE(r.catalog.pagesRef.genNum, 0);
    QCOMPARE(r.catalog.rootRef.objNum, 1);
}

void PdfCatalogResolverTest::testResolveMissingTypeTolerated() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Pages", PdfObject::makeReference(PdfRef{3, 0}, 0, 0));
    auto rootObj = PdfObject::makeDictionary(dict, 0, 0);

    PdfObjectStore store;
    store.add(1, 0, rootObj);

    PdfTrailer trailer;
    trailer.root = PdfRef{1, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QVERIFY(r.ok());
    QCOMPARE(r.catalog.pagesRef.objNum, 3);
}

void PdfCatalogResolverTest::testResolveMissingRootRefZeroObj() {
    PdfObjectStore store;
    PdfTrailer trailer;
    trailer.root = PdfRef{0, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QCOMPARE(r.warning, PdfCatalogResolverWarning::MissingRootReference);
}

void PdfCatalogResolverTest::testResolveUnresolvedRoot() {
    PdfObjectStore store;
    PdfTrailer trailer;
    trailer.root = PdfRef{99, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QCOMPARE(r.warning, PdfCatalogResolverWarning::UnresolvedRoot);
}

void PdfCatalogResolverTest::testResolveRootNotDictionary() {
    auto obj = PdfObject::makeInteger(42, 0, 0);
    PdfObjectStore store;
    store.add(1, 0, obj);

    PdfTrailer trailer;
    trailer.root = PdfRef{1, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QCOMPARE(r.warning, PdfCatalogResolverWarning::RootNotDictionary);
}

void PdfCatalogResolverTest::testResolveInvalidCatalogTypePages() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName(QByteArray("Pages"), 0, 0));
    dict->entries.insert("Pages", PdfObject::makeReference(PdfRef{3, 0}, 0, 0));
    auto rootObj = PdfObject::makeDictionary(dict, 0, 0);

    PdfObjectStore store;
    store.add(1, 0, rootObj);

    PdfTrailer trailer;
    trailer.root = PdfRef{1, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QCOMPARE(r.warning, PdfCatalogResolverWarning::InvalidCatalogType);
}

void PdfCatalogResolverTest::testResolveInvalidCatalogTypeInt() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeInteger(42, 0, 0));
    dict->entries.insert("Pages", PdfObject::makeReference(PdfRef{3, 0}, 0, 0));
    auto rootObj = PdfObject::makeDictionary(dict, 0, 0);

    PdfObjectStore store;
    store.add(1, 0, rootObj);

    PdfTrailer trailer;
    trailer.root = PdfRef{1, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QCOMPARE(r.warning, PdfCatalogResolverWarning::InvalidCatalogType);
}

void PdfCatalogResolverTest::testResolveMissingPages() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName(QByteArray("Catalog"), 0, 0));
    auto rootObj = PdfObject::makeDictionary(dict, 0, 0);

    PdfObjectStore store;
    store.add(1, 0, rootObj);

    PdfTrailer trailer;
    trailer.root = PdfRef{1, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QCOMPARE(r.warning, PdfCatalogResolverWarning::MissingPagesReference);
}

void PdfCatalogResolverTest::testResolvePagesNotReference() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName(QByteArray("Catalog"), 0, 0));
    dict->entries.insert("Pages", PdfObject::makeInteger(42, 0, 0));
    auto rootObj = PdfObject::makeDictionary(dict, 0, 0);

    PdfObjectStore store;
    store.add(1, 0, rootObj);

    PdfTrailer trailer;
    trailer.root = PdfRef{1, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QCOMPARE(r.warning, PdfCatalogResolverWarning::InvalidPagesReference);
}

void PdfCatalogResolverTest::testResolvePagesZeroObj() {
    auto dict = QSharedPointer<PdfDictionary>::create();
    dict->entries.insert("Type", PdfObject::makeName(QByteArray("Catalog"), 0, 0));
    dict->entries.insert("Pages", PdfObject::makeReference(PdfRef{0, 0}, 0, 0));
    auto rootObj = PdfObject::makeDictionary(dict, 0, 0);

    PdfObjectStore store;
    store.add(1, 0, rootObj);

    PdfTrailer trailer;
    trailer.root = PdfRef{1, 0};

    PdfCatalogResolver resolver;
    auto r = resolver.resolve(trailer, store);
    QCOMPARE(r.warning, PdfCatalogResolverWarning::InvalidPagesReference);
}

QTEST_MAIN(PdfCatalogResolverTest)
#include "PdfCatalogResolverTest.moc"
