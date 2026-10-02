#include <QTest>
#include "renderers/MarkdownHtmlRenderer.h"
#include "renderers/RenderPipelineStage.h"

// The full pipeline, in execution order (see RenderPipelineStage.h).
static const QList<RenderStage> kExpectedStages = {
    RenderStage::Md4c,
    RenderStage::Alerts, RenderStage::Math, RenderStage::WikiLinks,
    RenderStage::HeadingAnchors, RenderStage::InlineExtensions, RenderStage::SyntaxHighlight,
    RenderStage::BlockquoteWrap, RenderStage::CodeBlockWrap, RenderStage::HtmlNormalize,
    RenderStage::RtlProcessing, RenderStage::RtlBlockquoteOrder, RenderStage::TaskListNormalize,
};

class RenderPipelineContractTest : public QObject
{
    Q_OBJECT

private slots:
    void testStageOrder();
    void testDisabledExtensionsSkipTheirStages();
    void testAllStagesReportSuccess();
    void testDiagnosticsCount();
    void testDiagnosticsIncludeTiming();
};

void RenderPipelineContractTest::testStageOrder()
{
    auto result = MarkdownHtmlRenderer::render("# Hello\n");
    QVERIFY(result.isValid());
    QCOMPARE(result.diagnostics.size(), kExpectedStages.size());
    for (qsizetype i = 0; i < kExpectedStages.size(); ++i)
        QCOMPARE(result.diagnostics[i].stage, kExpectedStages[i]);
}

void RenderPipelineContractTest::testDisabledExtensionsSkipTheirStages()
{
    auto result = MarkdownHtmlRenderer::render("# Hello\n", mdbr::MarkdownExtensions::none());
    QVERIFY(result.isValid());
    QCOMPARE(result.diagnostics.size(), kExpectedStages.size() - 6);
    QCOMPARE(result.diagnostics[1].stage, RenderStage::BlockquoteWrap);
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
    QCOMPARE(result.diagnostics.size(), kExpectedStages.size());
}

void RenderPipelineContractTest::testDiagnosticsIncludeTiming()
{
    auto result = MarkdownHtmlRenderer::render("# Hello\n");
    QVERIFY(result.isValid());
    QCOMPARE(result.diagnostics.size(), kExpectedStages.size());
    for (const auto &diag : result.diagnostics) {
        QVERIFY2(diag.elapsedMs >= 0,
                 "every stage must record elapsedMs >= 0");
    }
}

QTEST_MAIN(RenderPipelineContractTest)
#include "RenderPipelineContractTest.moc"
