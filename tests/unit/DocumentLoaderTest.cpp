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
    void testLoadWindows1256ArabicFile();
    void testLoadNormalizesLineEndings();
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

void DocumentLoaderTest::testLoadWindows1256ArabicFile()
{
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    // "# " + Arabic "hello world" encoded in Windows-1256
    tmpFile.write(QByteArray("# ") + QByteArray::fromHex("e3d1cdc8c720c8c7e1dac7e1e3"));
    tmpFile.flush();

    auto result = DocumentLoader::load(tmpFile.fileName());

    QVERIFY(result.isValid());
    QCOMPARE(result.document.encodingName, QStringLiteral("Windows-1256"));
    QVERIFY(result.document.encodingGuessed);
    QVERIFY(result.document.rawText.contains(QChar(0x0645)));
    QVERIFY(!result.document.rawText.contains(QChar(0xFFFD)));
}

void DocumentLoaderTest::testLoadNormalizesLineEndings()
{
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    tmpFile.write("line1\r\nline2\rline3\n");
    tmpFile.flush();

    auto result = DocumentLoader::load(tmpFile.fileName());

    QVERIFY(result.isValid());
    QCOMPARE(result.document.rawText, QStringLiteral("line1\nline2\nline3\n"));
    QCOMPARE(result.document.encodingName, QStringLiteral("UTF-8"));
    QVERIFY(!result.document.encodingGuessed);
}

QTEST_MAIN(DocumentLoaderTest)
#include "DocumentLoaderTest.moc"
