#include <QTest>
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

QTEST_MAIN(DocumentOpenPipelineTest)
#include "DocumentOpenPipelineTest.moc"
