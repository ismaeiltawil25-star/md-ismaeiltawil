#include <QTest>
#include <QFileInfo>
#include <QTemporaryFile>
#include "services/DocumentLoader.h"

class DocumentLoaderTest : public QObject
{
    Q_OBJECT

private slots:
    void testLoadSample();
    void testLoadEmpty();
    void testLoadMissing();
    void testLoadTooLarge();
};

void DocumentLoaderTest::testLoadSample()
{
    const QString path = QString(TEST_DATA_DIR) + "/sample.md";
    auto result = DocumentLoader::load(path);

    QVERIFY(result.isValid());
    QCOMPARE(result.document.fileName, QString("sample.md"));
    QCOMPARE(result.document.filePath, QFileInfo(path).absoluteFilePath());
    QVERIFY(!result.document.rawText.isEmpty());
}

void DocumentLoaderTest::testLoadEmpty()
{
    const QString path = QString(TEST_DATA_DIR) + "/empty.md";
    auto result = DocumentLoader::load(path);

    QVERIFY(result.isValid());
    QCOMPARE(result.document.fileName, QString("empty.md"));
    QVERIFY(result.document.rawText.isEmpty());
}

void DocumentLoaderTest::testLoadMissing()
{
    auto result = DocumentLoader::load("C:/this/file/does/not/exist.md");

    QVERIFY(!result.isValid());
    QVERIFY(!result.errorMessage.isEmpty());
}

void DocumentLoaderTest::testLoadTooLarge()
{
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    QByteArray data(DocumentLoader::MaxFileSize + 1, 'x');
    tmpFile.write(data);
    tmpFile.flush();

    auto result = DocumentLoader::load(tmpFile.fileName());

    QVERIFY(!result.isValid());
    QVERIFY(result.errorMessage.contains("too large"));
}

QTEST_MAIN(DocumentLoaderTest)
#include "DocumentLoaderTest.moc"
