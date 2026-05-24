#include <QTest>
#include "renderers/MarkdownHtmlRenderer.h"
#include "renderers/RenderPipelineStage.h"

class RenderPipelineContractTest : public QObject
{
    Q_OBJECT

private slots:
    void testStageOrder();
    void testAllStagesReportSuccess();
    void testDiagnosticsCount();
    void testDiagnosticsIncludeTiming();
};

void RenderPipelineContractTest::testStageOrder()
{
    auto result = MarkdownHtmlRenderer::render("# Hello\n");
    QVERIFY(result.isValid());
    QVERIFY(result.diagnostics.size() >= 7);

    QCOMPARE(result.diagnostics[0].stage, RenderStage::Md4c);
    QCOMPARE(result.diagnostics[1].stage, RenderStage::BlockquoteWrap);
    QCOMPARE(result.diagnostics[2].stage, RenderStage::CodeBlockWrap);
    QCOMPARE(result.diagnostics[3].stage, RenderStage::HtmlNormalize);
    QCOMPARE(result.diagnostics[4].stage, RenderStage::RtlProcessing);
    QCOMPARE(result.diagnostics[5].stage, RenderStage::RtlBlockquoteOrder);
    QCOMPARE(result.diagnostics[6].stage, RenderStage::TaskListNormalize);
}

void RenderPipelineContractTest::testAllStagesReportSuccess()
{
    auto result = MarkdownHtmlRenderer::render("# Hello\n");
    QVERIFY(result.isValid());

    for (const auto &diag : result.diagnostics)
        QVERIFY2(diag.success, "All stages must report success for valid input");
}

void RenderPipelineContractTest::testDiagnosticsCount()
{
    auto result = MarkdownHtmlRenderer::render("# Hello\n");
    QVERIFY(result.isValid());
    QCOMPARE(result.diagnostics.size(), 7);
}

void RenderPipelineContractTest::testDiagnosticsIncludeTiming()
{
    auto result = MarkdownHtmlRenderer::render("# Hello\n");
    QVERIFY(result.isValid());
    QCOMPARE(result.diagnostics.size(), 7);
    for (const auto &diag : result.diagnostics) {
        QVERIFY2(diag.elapsedMs >= 0,
                 "every stage must record elapsedMs >= 0");
    }
}

QTEST_MAIN(RenderPipelineContractTest)
#include "RenderPipelineContractTest.moc"
