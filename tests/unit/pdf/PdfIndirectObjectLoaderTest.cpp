#include <QtTest/QTest>
#include "PdfByteReader.h"
#include "PdfXRefParser.h"
#include "PdfIndirectObjectLoader.h"
#include "PdfRawStreamStore.h"

class PdfIndirectObjectLoaderTest : public QObject {
    Q_OBJECT

private slots:
    void testLoadSingleObject();
    void testLoadMultipleObjects();
    void testSkipFreeEntry();
    void testLoadObjectAtNonZeroOffset();
    void testInvalidOffset();
    void testParseFailure();
    void testMissingEndobjIsParseFailure();
    void testObjectNumberMismatch();
    void testGenerationMismatch();
    void testDuplicateEntryLoaded();
    void testMultipleSubsections();
    void testStreamObjectNotStored();
    void testInUseObjNumZeroSkipped();
    void testStoreRejectsInvalidKey();
    void testCountsAddUp();

    void testLoadStreamObject();
    void testLoadStreamObjectWithFilter();
    void testLoadStreamMissingLength();
    void testLoadStreamIndirectLength();
    void testLoadStreamBodyReaderFailure();
    void testLoadStreamMissingEndobj();
    void testLoadStreamMalformedEndobj();
    void testLoadStreamAndNormalObject();
    void testLoadStreamLegacyWithoutStreamStore();
    void testLoadStreamDictionaryStoredBeforeBody();
    void testLoadStreamDuplicateStreamKey();
    void testLoadStreamEndstreamEndobjFullPipeline();
};

void PdfIndirectObjectLoaderTest::testLoadSingleObject() {
    QByteArray data("5 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 5;
    sub.count = 1;
    PdfXRefEntry entry;
    entry.objNum = 5;
    entry.offset = 0;
    entry.genNum = 0;
    entry.type = PdfXRefEntryType::InUse;
    sub.entries.append(entry);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.inUseEntries, 1);
    QCOMPARE(store.count(), qsizetype(1));
    auto obj = store.resolve(5, 0);
    QVERIFY(obj.found());
    QCOMPARE(obj.object->kind, PdfObjectKind::Integer);
    QCOMPARE(obj.object->intValue, int64_t(42));
}

void PdfIndirectObjectLoaderTest::testLoadMultipleObjects() {
    // Object 1 at offset 0
    QByteArray data1("1 0 obj 10 endobj ");
    // Object 2 at offset ~offset of data1 end
    qsizetype off2 = data1.size();
    QByteArray data2("2 0 obj 20 endobj ");
    qsizetype off3 = off2 + data2.size();
    QByteArray data3("3 0 obj 30 endobj");
    QByteArray data = data1 + data2 + data3;

    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 1;
    sub.count = 3;
    for (int i = 0; i < 3; ++i) {
        PdfXRefEntry e;
        e.objNum = i + 1;
        e.genNum = 0;
        e.type = PdfXRefEntryType::InUse;
        e.offset = (i == 0) ? 0 : (i == 1) ? off2 : off3;
        sub.entries.append(e);
    }
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.loadedCount, 3);
    QCOMPARE(store.count(), qsizetype(3));
}

void PdfIndirectObjectLoaderTest::testSkipFreeEntry() {
    QByteArray data("1 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 0;
    sub.count = 3;

    PdfXRefEntry f1; f1.objNum = 0; f1.type = PdfXRefEntryType::Free; sub.entries.append(f1);
    PdfXRefEntry i1; i1.objNum = 1; i1.offset = 0; i1.genNum = 0; i1.type = PdfXRefEntryType::InUse; sub.entries.append(i1);
    PdfXRefEntry f2; f2.objNum = 2; f2.type = PdfXRefEntryType::Free; sub.entries.append(f2);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.freeCount, 2);
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.totalXRefEntries, 3);
    QCOMPARE(r.inUseEntries, 1);
}

void PdfIndirectObjectLoaderTest::testLoadObjectAtNonZeroOffset() {
    QByteArray data = QByteArray(42, 'x') + QByteArray("5 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 5;
    sub.count = 1;
    PdfXRefEntry e;
    e.objNum = 5; e.offset = 42; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.loadedCount, 1);
}

void PdfIndirectObjectLoaderTest::testInvalidOffset() {
    QByteArray data("1 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 1;
    sub.count = 1;
    PdfXRefEntry e;
    e.objNum = 1; e.offset = data.size() + 100; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::InvalidObjectOffset);
    QCOMPARE(r.skippedCount, 1);
    QCOMPARE(r.loadedCount, 0);
}

void PdfIndirectObjectLoaderTest::testParseFailure() {
    QByteArray data("garbage not an object");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 1;
    sub.count = 1;
    PdfXRefEntry e;
    e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::ParseFailure);
    QCOMPARE(r.skippedCount, 1);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfIndirectObjectLoaderTest::testMissingEndobjIsParseFailure() {
    QByteArray data("5 0 obj 42");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 5;
    sub.count = 1;
    PdfXRefEntry e;
    e.objNum = 5; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::ParseFailure);
    QCOMPARE(r.skippedCount, 1);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfIndirectObjectLoaderTest::testObjectNumberMismatch() {
    // xref says objNum=5 but actual object at offset is 6 0 obj
    QByteArray data("6 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 5;
    sub.count = 1;
    PdfXRefEntry e;
    e.objNum = 5; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::ObjectNumberMismatch);
    QCOMPARE(r.mismatchCount, 1);
    QCOMPARE(r.skippedCount, 1);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfIndirectObjectLoaderTest::testGenerationMismatch() {
    QByteArray data("5 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 5;
    sub.count = 1;
    PdfXRefEntry e;
    e.objNum = 5; e.offset = 0; e.genNum = 1; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::ObjectNumberMismatch);
    QCOMPARE(r.mismatchCount, 1);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfIndirectObjectLoaderTest::testDuplicateEntryLoaded() {
    QByteArray data("5 0 obj 10 endobj 5 0 obj 20 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub;
    sub.startObjNum = 5;
    sub.count = 2;

    PdfXRefEntry e1; e1.objNum = 5; e1.offset = 0; e1.genNum = 0; e1.type = PdfXRefEntryType::InUse; sub.entries.append(e1);
    PdfXRefEntry e2; e2.objNum = 5; e2.offset = qsizetype(QByteArray("5 0 obj 10 endobj ").size()); e2.genNum = 0; e2.type = PdfXRefEntryType::InUse; sub.entries.append(e2);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.loadedCount, 2);
    QCOMPARE(r.duplicateCount, 1);
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::DuplicateObjectKey);
    QCOMPARE(store.count(), qsizetype(1));
    QCOMPARE(store.resolve(5, 0).object->intValue, int64_t(20));
}

void PdfIndirectObjectLoaderTest::testMultipleSubsections() {
    QByteArray data1("1 0 obj 10 endobj ");
    QByteArray data2("3 0 obj 30 endobj");
    QByteArray data = data1 + data2;

    PdfByteReader reader(data);
    PdfXRefTable table;

    PdfXRefSubsection sub1; sub1.startObjNum = 1; sub1.count = 1;
    PdfXRefEntry e1; e1.objNum = 1; e1.offset = 0; e1.genNum = 0; e1.type = PdfXRefEntryType::InUse;
    sub1.entries.append(e1);

    PdfXRefSubsection sub2; sub2.startObjNum = 3; sub2.count = 1;
    PdfXRefEntry e2; e2.objNum = 3; e2.offset = qsizetype(data1.size()); e2.genNum = 0; e2.type = PdfXRefEntryType::InUse;
    sub2.entries.append(e2);

    table.subsections.append(sub1);
    table.subsections.append(sub2);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.loadedCount, 2);
    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(store.resolve(3, 0).found());
}

void PdfIndirectObjectLoaderTest::testStreamObjectNotStored() {
    QByteArray data("1 0 obj << /Length 5 >> stream");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::UnsupportedStream);
    QCOMPARE(r.unsupportedStreamCount, 1);
    QCOMPARE(r.skippedCount, 1);
    QCOMPARE(r.loadedCount, 0);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfIndirectObjectLoaderTest::testInUseObjNumZeroSkipped() {
    QByteArray data;
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 0; sub.count = 1;
    PdfXRefEntry e; e.objNum = 0; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::StoreInsertFailure);
    QCOMPARE(r.skippedCount, 1);
    QCOMPARE(r.loadedCount, 0);
}

void PdfIndirectObjectLoaderTest::testStoreRejectsInvalidKey() {
    QByteArray data("-5 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    // objNum=1 but parsed object will have negative objNum from parse
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    // parseIndirectObject validates objNum >= 0 and returns null for negative
    // So this ends up as ParseFailure
    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::ParseFailure);
    QCOMPARE(r.skippedCount, 1);
}

void PdfIndirectObjectLoaderTest::testCountsAddUp() {
    QByteArray data = QByteArray("1 0 obj 10 endobj ")
                     + QByteArray("2 0 obj 20 endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 0; sub.count = 3;

    PdfXRefEntry f1; f1.objNum = 0; f1.type = PdfXRefEntryType::Free; sub.entries.append(f1);
    PdfXRefEntry e1; e1.objNum = 1; e1.offset = 0; e1.genNum = 0; e1.type = PdfXRefEntryType::InUse; sub.entries.append(e1);
    PdfXRefEntry e2; e2.objNum = 2; e2.offset = qsizetype(QByteArray("1 0 obj 10 endobj ").size()); e2.genNum = 0; e2.type = PdfXRefEntryType::InUse; sub.entries.append(e2);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);
    QCOMPARE(r.totalXRefEntries, 3);
    QCOMPARE(r.inUseEntries, 2);
    QCOMPARE(r.freeCount, 1);
    QCOMPARE(r.loadedCount, 2);
    QCOMPARE(r.skippedCount, 0);
    QCOMPARE(r.mismatchCount, 0);
    QCOMPARE(r.duplicateCount, 0);
}

void PdfIndirectObjectLoaderTest::testLoadStreamObject() {
    QByteArray data("1 0 obj << /Length 5 >>\nstream\nAAAAA\nendstream endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.streamLoadedCount, 1);
    QCOMPARE(r.streamFailureCount, 0);
    QCOMPARE(r.unsupportedStreamCount, 0);

    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(streamStore.contains(1, 0));
    const PdfRawStreamObject *rs = streamStore.get(1, 0);
    QVERIFY(rs != nullptr);
    QCOMPARE(rs->rawBody, QByteArrayView("AAAAA"));
    QCOMPARE(rs->length, qsizetype(5));
}

void PdfIndirectObjectLoaderTest::testLoadStreamObjectWithFilter() {
    QByteArray data("1 0 obj << /Length 5 /Filter /FlateDecode >>\nstream\nAAAAA\nendstream endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.streamLoadedCount, 1);

    const PdfRawStreamObject *rs = streamStore.get(1, 0);
    QVERIFY(rs != nullptr);
    QCOMPARE(rs->filters.size(), 1);
    QCOMPARE(rs->filters[0], QByteArray("FlateDecode"));
}

void PdfIndirectObjectLoaderTest::testLoadStreamMissingLength() {
    QByteArray data("1 0 obj << /Type /XObject >>\nstream\nAAAAA\nendstream endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.streamFailureCount, 1);
    QCOMPARE(r.streamLoadedCount, 0);
    QCOMPARE(r.skippedCount, 0);

    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(!streamStore.contains(1, 0));
}

void PdfIndirectObjectLoaderTest::testLoadStreamIndirectLength() {
    QByteArray data("1 0 obj << /Length 5 0 R >>\nstream\nAAAAA\nendstream endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.streamFailureCount, 1);
    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(!streamStore.contains(1, 0));
}

void PdfIndirectObjectLoaderTest::testLoadStreamBodyReaderFailure() {
    QByteArray data("1 0 obj << /Length 999 >>\nstream\nAAAAA\nendstream endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.streamFailureCount, 1);
    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(!streamStore.contains(1, 0));
}

void PdfIndirectObjectLoaderTest::testLoadStreamMissingEndobj() {
    QByteArray data("1 0 obj << /Length 5 >>\nstream\nAAAAA\nendstream missing");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.streamFailureCount, 1);
    QCOMPARE(r.streamLoadedCount, 0);
    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(!streamStore.contains(1, 0));
}

void PdfIndirectObjectLoaderTest::testLoadStreamMalformedEndobj() {
    QByteArray data("1 0 obj << /Length 5 >>\nstream\nAAAAA\nendstream endobjXYZ extra");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.streamFailureCount, 1);
    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(!streamStore.contains(1, 0));
}

void PdfIndirectObjectLoaderTest::testLoadStreamAndNormalObject() {
    QByteArray data1("1 0 obj 42 endobj ");
    qsizetype off2 = data1.size();
    QByteArray data = data1 + QByteArray("2 0 obj << /Length 5 >>\nstream\nAAAAA\nendstream endobj");

    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 2;
    PdfXRefEntry e1; e1.objNum = 1; e1.offset = 0; e1.genNum = 0; e1.type = PdfXRefEntryType::InUse;
    sub.entries.append(e1);
    PdfXRefEntry e2; e2.objNum = 2; e2.offset = off2; e2.genNum = 0; e2.type = PdfXRefEntryType::InUse;
    sub.entries.append(e2);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 2);
    QCOMPARE(r.streamLoadedCount, 1);
    QCOMPARE(r.streamFailureCount, 0);
    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(store.resolve(2, 0).found());
    QVERIFY(streamStore.contains(2, 0));
}

void PdfIndirectObjectLoaderTest::testLoadStreamLegacyWithoutStreamStore() {
    QByteArray data("1 0 obj << /Length 5 >>\nstream\nAAAAA\nendstream endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store);

    QCOMPARE(r.warning, PdfIndirectObjectLoaderWarning::UnsupportedStream);
    QCOMPARE(r.unsupportedStreamCount, 1);
    QCOMPARE(r.skippedCount, 1);
    QCOMPARE(r.loadedCount, 0);
    QCOMPARE(store.count(), qsizetype(0));
}

void PdfIndirectObjectLoaderTest::testLoadStreamDictionaryStoredBeforeBody() {
    QByteArray data("1 0 obj << /Type /XObject >>\nstream\nAAAAA\nendstream endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.streamFailureCount, 1);
    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(!streamStore.contains(1, 0));
}

void PdfIndirectObjectLoaderTest::testLoadStreamDuplicateStreamKey() {
    QByteArray data("1 0 obj << /Length 5 >>\nstream\nAAAAA\nendstream endobj");

    // First load
    PdfByteReader reader1(data);
    PdfXRefTable table1;
    PdfXRefSubsection sub1; sub1.startObjNum = 1; sub1.count = 1;
    PdfXRefEntry e1; e1.objNum = 1; e1.offset = 0; e1.genNum = 0; e1.type = PdfXRefEntryType::InUse;
    sub1.entries.append(e1);
    table1.subsections.append(sub1);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    loader.load(reader1, table1, store, &streamStore);
    QCOMPARE(streamStore.count(), qsizetype(1));

    // Second load — same objNum, different stream
    QByteArray data2("1 0 obj << /Length 3 >>\nstream\nBBB\nendstream endobj");
    PdfByteReader reader2(data2);
    PdfXRefTable table2;
    PdfXRefSubsection sub2; sub2.startObjNum = 1; sub2.count = 1;
    PdfXRefEntry e2; e2.objNum = 1; e2.offset = 0; e2.genNum = 0; e2.type = PdfXRefEntryType::InUse;
    sub2.entries.append(e2);
    table2.subsections.append(sub2);

    auto r = loader.load(reader2, table2, store, &streamStore);
    QCOMPARE(r.streamLoadedCount, 1);
    QCOMPARE(r.duplicateStreamCount, 1);
    QCOMPARE(streamStore.count(), qsizetype(1));
    const PdfRawStreamObject *rs = streamStore.get(1, 0);
    QVERIFY(rs != nullptr);
    QCOMPARE(rs->rawBody, QByteArrayView("BBB"));
}

void PdfIndirectObjectLoaderTest::testLoadStreamEndstreamEndobjFullPipeline() {
    QByteArray data("1 0 obj << /Length 5 >>\nstream\nAAAAA\nendstream endobj");
    PdfByteReader reader(data);
    PdfXRefTable table;
    PdfXRefSubsection sub; sub.startObjNum = 1; sub.count = 1;
    PdfXRefEntry e; e.objNum = 1; e.offset = 0; e.genNum = 0; e.type = PdfXRefEntryType::InUse;
    sub.entries.append(e);
    table.subsections.append(sub);

    PdfObjectStore store;
    PdfRawStreamStore streamStore;
    PdfIndirectObjectLoader loader;
    auto r = loader.load(reader, table, store, &streamStore);

    QVERIFY(r.ok());
    QCOMPARE(r.loadedCount, 1);
    QCOMPARE(r.streamLoadedCount, 1);
    QCOMPARE(r.streamFailureCount, 0);
    QVERIFY(store.resolve(1, 0).found());
    QVERIFY(streamStore.contains(1, 0));

    const PdfRawStreamObject *rs = streamStore.get(1, 0);
    QVERIFY(rs != nullptr);
    QCOMPARE(rs->rawBody, QByteArrayView("AAAAA"));
    QCOMPARE(rs->length, qsizetype(5));
    QVERIFY(rs->hasIdentity);
    QCOMPARE(rs->identity.objNum, 1);
}

QTEST_MAIN(PdfIndirectObjectLoaderTest)
#include "PdfIndirectObjectLoaderTest.moc"
