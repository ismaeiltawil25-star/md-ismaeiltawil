#include <QTest>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include "app/document/DocumentOpenPipeline.h"

class DocumentOpenPipelineTest : public QObject
{
    Q_OBJECT

private slots:
    void testValidMarkdownReturnsHtml();
    void testYamlFrontMatterStripped();
    void testDefinitionListsConverted();
    void testMissingFileReturnsError();
    void testBaseUrlPointsToFolder();
    void testRenderFailureFallsBackToPlainText();
    void testFileNameExtracted();
    void testDiagnosticsPassThroughPipeline();
    void testGuessedEncodingIsReported();
    void testUtf8FileHasNoEncodingNotice();
};

void DocumentOpenPipelineTest::testValidMarkdownReturnsHtml()
{
    const QString path = QString(TEST_DATA_DIR) + "/sample.md";
    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(path);

    QVERIFY(result.isValid());
    QCOMPARE(result.tabState.contentMode, ContentMode::Html);
    QVERIFY(!result.tabState.originalHtml.isEmpty());
    QVERIFY(!result.fileName.isEmpty());
}

void DocumentOpenPipelineTest::testYamlFrontMatterStripped()
{
    const QString path = QString(TEST_DATA_DIR) + "/frontmatter.md";
    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(path);

    QVERIFY(result.isValid());
    QVERIFY(!result.tabState.originalHtml.contains("---"));
    QVERIFY(!result.tabState.originalHtml.contains("title:"));
}

void DocumentOpenPipelineTest::testDefinitionListsConverted()
{
    const QString path = QString(TEST_DATA_DIR) + "/definition-list.md";
    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(path);

    QVERIFY(result.isValid());
    QVERIFY(result.tabState.originalHtml.contains("definition-table"));
}

void DocumentOpenPipelineTest::testMissingFileReturnsError()
{
    DocumentOpenPipeline pipeline;
    auto result = pipeline.open("C:/nonexistent-test-file-12345.md");

    QVERIFY(!result.isValid());
    QVERIFY(!result.errorMessage.isEmpty());
}

void DocumentOpenPipelineTest::testBaseUrlPointsToFolder()
{
    const QString path = QString(TEST_DATA_DIR) + "/sample.md";
    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(path);

    QVERIFY(result.isValid());
    QUrl expectedBase = QUrl::fromLocalFile(QFileInfo(path).absolutePath() + '/');
    QCOMPARE(result.baseUrl.toString(), expectedBase.toString());
}

void DocumentOpenPipelineTest::testRenderFailureFallsBackToPlainText()
{
    auto failRenderer = [](const QString &) -> PipelineRenderResult {
        return { QString(), "Render failed intentionally" };
    };

    DocumentOpenPipeline pipeline(failRenderer);

    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    QByteArray content = "# Fallback\n\nThis should appear as plain text.";
    tmpFile.write(content);
    tmpFile.flush();

    auto result = pipeline.open(tmpFile.fileName());

    QVERIFY(result.isValid());
    QCOMPARE(result.tabState.contentMode, ContentMode::Plain);
    QVERIFY(!result.tabState.originalPlainText.isEmpty());
    QVERIFY(!result.renderWarning.isEmpty());
}

void DocumentOpenPipelineTest::testFileNameExtracted()
{
    const QString path = QString(TEST_DATA_DIR) + "/sample.md";
    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(path);
    QCOMPARE(result.fileName, QString("sample.md"));
}

void DocumentOpenPipelineTest::testDiagnosticsPassThroughPipeline()
{
    const QString path = QString(TEST_DATA_DIR) + "/sample.md";
    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(path);

    QVERIFY(result.isValid());
    QVERIFY(!result.diagnostics.isEmpty());
    QCOMPARE(result.diagnostics.size(), 13); // every stage of RenderPipelineStage.h
    // Verify each diagnostic has a valid stage enum value
    QCOMPARE(result.diagnostics[0].stage, RenderStage::Md4c);
    QCOMPARE(result.diagnostics.last().stage, RenderStage::TaskListNormalize);
    // Timing must be non-negative
    for (const auto &diag : result.diagnostics)
        QVERIFY(diag.elapsedMs >= 0);
}

void DocumentOpenPipelineTest::testGuessedEncodingIsReported()
{
    QTemporaryFile tmpFile(QDir::tempPath() + "/pipeline-XXXXXX.md");
    QVERIFY(tmpFile.open());
    // "# " + Arabic "hello world" encoded in Windows-1256
    tmpFile.write(QByteArray("# ") + QByteArray::fromHex("e3d1cdc8c720c8c7e1dac7e1e3"));
    tmpFile.flush();

    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(tmpFile.fileName());

    QVERIFY(result.isValid());
    QVERIFY(result.encodingNotice.contains("Windows-1256"));
    QVERIFY(result.tabState.originalHtml.contains(QChar(0x0645)));
}

void DocumentOpenPipelineTest::testUtf8FileHasNoEncodingNotice()
{
    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(QString(TEST_DATA_DIR) + "/sample.md");

    QVERIFY(result.isValid());
    QVERIFY(result.encodingNotice.isEmpty());
}

QTEST_MAIN(DocumentOpenPipelineTest)
#include "DocumentOpenPipelineTest.moc"
