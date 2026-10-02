// Robustness and performance guard for the whole Markdown pipeline.
//
//  * fuzz:         thousands of random documents (seeded, reproducible) must
//                  render without crashing and return a valid result;
//  * pathological: inputs known to trigger quadratic behaviour in naive
//                  parsers must render within a time budget;
//  * throughput:   a realistic 1 MB document must stay well under a second.
//
// Budgets are deliberately generous (CI machines are slow); they exist to catch
// algorithmic regressions (10x-1000x), not to benchmark.
//
// To reproduce a fuzz failure, the failing seed and document index are printed.

#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QTest>

#include "RenderPipelineTestHelpers.h"
#include "services/text/TextDecoder.h"

using mdbr::test::preprocess;

namespace {

constexpr quint32 kFuzzSeed = 20261002;
constexpr int kFuzzDocuments = 400;
constexpr qint64 kPathologicalBudgetMs = 3000;
constexpr qint64 kOneMegabyteBudgetMs = 3000;

const QStringList kFragments = {
    "# Heading\n", "## Sub *heading*\n", "Plain paragraph text.\n", "\n", "\n\n",
    "---\n", "...\n", "title: value\n", "Term\n", ": description `code`\n",
    "```\n", "```cpp\n", "~~~\n", "    indented code\n", "> quote\n", ">> nested quote\n",
    "- item\n", "  - nested item\n", "1. ordered\n", "- [ ] task\n", "- [x] done\n",
    "| a | b |\n", "|---|---|\n", "| 1 | 2 |\n", "**bold** ", "*em* ", "~~del~~ ",
    "`code` ", "[link](https://example.com) ", "https://example.com/x ", "www.example.org ",
    "<div>", "</div>", "<details>", "<summary>S</summary>", "</details>", "<kbd>K</kbd>",
    "$x^2$ ", "$$\n\\sum_{i}\n$$\n", "\\frac{", "==hi== ", "^2^ ", ":rocket: ", "[[Page]] ",
    "[^1]", "[^1]: note\n", "> [!WARNING]\n", "```python\n", "```cpp\n", "# Heading\n",
    "<mark>M</mark>", "<br>", "<p>", "</p>", "<", ">", "&amp; ", "&", "\\*", "*", "_", "[", "]",
    "(", ")", "!", "|", "`", "~", ":", "\t", "\r\n", "\r",
    QStringLiteral(u"\u0645\u0631\u062D\u0628\u0627 "), // Arabic word
    QStringLiteral(u"\u05E9\u05DC\u05D5\u05DD "),       // Hebrew word
    QStringLiteral(u"\U0001F600 "),                     // emoji (surrogate pair)
    QStringLiteral(u"\uFEFF"), QStringLiteral(u"\u0000"), QStringLiteral(u"\uFFFD"),
};

QString randomDocument(QRandomGenerator &rng)
{
    QString doc;
    const int parts = rng.bounded(1, 80);
    for (int i = 0; i < parts; ++i)
        doc += kFragments.at(rng.bounded(kFragments.size()));
    return doc;
}

qint64 timeFullPipelineMs(const QString &markdown, MarkdownHtmlRenderer::Result *out = nullptr)
{
    QElapsedTimer timer;
    timer.start();
    auto result = MarkdownHtmlRenderer::render(preprocess(markdown));
    const qint64 elapsed = timer.elapsed();
    if (out)
        *out = std::move(result);
    return elapsed;
}

QString realisticDocument(qsizetype minimumBytes)
{
    const QString block = QStringLiteral(
        "## Section\n\nParagraph with **bold**, *italic*, `code` and a [link](https://example.com).\n\n"
        u"\u0647\u0630\u0647 \u0641\u0642\u0631\u0629 \u0639\u0631\u0628\u064A\u0629 with English.\n\n"
        "- item one\n- item two\n  - nested\n- [x] done\n\n> A quote.\n\n"
        "```python\ndef f(x):\n    return x\n```\n\n| a | b |\n|---|---|\n| 1 | 2 |\n\n"
        "Term\n: Definition text.\n\n");
    QString doc;
    while (doc.size() < minimumBytes)
        doc += block;
    return doc;
}

} // namespace

class RenderRobustnessTest : public QObject
{
    Q_OBJECT

private slots:
    void fuzzRandomMarkdownNeverFails();
    void fuzzRandomBytesThroughDecoder();
    void pathologicalInputsStayFast_data();
    void pathologicalInputsStayFast();
    void oneMegabyteDocumentIsFast();
};

void RenderRobustnessTest::fuzzRandomMarkdownNeverFails()
{
    QRandomGenerator rng(kFuzzSeed);
    for (int i = 0; i < kFuzzDocuments; ++i) {
        const QString doc = randomDocument(rng);
        MarkdownHtmlRenderer::Result result;
        timeFullPipelineMs(doc, &result);
        if (!result.isValid())
            QFAIL(qPrintable(QString("seed %1, document %2 failed: %3\n--- input ---\n%4")
                                 .arg(kFuzzSeed).arg(i).arg(result.errorMessage, doc)));
    }
}

void RenderRobustnessTest::fuzzRandomBytesThroughDecoder()
{
    QRandomGenerator rng(kFuzzSeed + 1);
    for (int i = 0; i < kFuzzDocuments; ++i) {
        QByteArray bytes(rng.bounded(0, 2048), Qt::Uninitialized);
        for (char &b : bytes)
            b = static_cast<char>(rng.bounded(256));

        const auto decoded = mdbr::text::decode(bytes);
        const QString text = mdbr::text::normalizeLineEndings(decoded.text);
        QVERIFY2(!text.contains(u'\r'), "line endings must be normalized");

        const auto result = MarkdownHtmlRenderer::render(preprocess(text));
        QVERIFY2(result.isValid(), qPrintable(QString("random bytes, document %1").arg(i)));
    }
}

void RenderRobustnessTest::pathologicalInputsStayFast_data()
{
    QTest::addColumn<QString>("markdown");

    QTest::newRow("deep blockquote")    << QString(2000, u'>') + " text\n";
    QTest::newRow("deep list")          << [] { QString s; for (int i = 0; i < 500; ++i) s += QString(i * 2, u' ') + "- item\n"; return s; }();
    QTest::newRow("one huge line")      << QString(1'000'000, u'a');
    QTest::newRow("many brackets")      << QString(100'000, u'[');
    QTest::newRow("many emphasis")      << QString(100'000, u'*');
    QTest::newRow("many backticks")     << QString(100'000, u'`');
    QTest::newRow("many angle")         << QString(100'000, u'<');
    QTest::newRow("unclosed fence")     << "```\n" + QString("line\n").repeated(100'000);
    QTest::newRow("many definitions")   << QString("Term\n: Desc\n\n").repeated(20'000);
    QTest::newRow("many divs")          << QString("<div>x</div>\n\n").repeated(20'000);
    QTest::newRow("nested divs")        << QString("<div>").repeated(5'000) + "x" + QString("</div>").repeated(5'000);
    QTest::newRow("many quotes")        << QString("> quote\n\n").repeated(20'000);
    QTest::newRow("arabic paragraphs")  << QString(QStringLiteral(u"\u0645\u0631\u062D\u0628\u0627 \u0628\u0627\u0644\u0639\u0627\u0644\u0645\n\n")).repeated(20'000);
    QTest::newRow("arabic quotes")      << QString(QStringLiteral(u"> \u0645\u0631\u062D\u0628\u0627\n\n")).repeated(20'000);
    QTest::newRow("deep arabic quote")  << QString(2000, u'>') + QStringLiteral(u" \u0645\u0631\u062D\u0628\u0627\n");
    QTest::newRow("math deep braces")   << "$" + QString(50'000, u'{') + "x$";
    QTest::newRow("many dollars")       << QString(100'000, u'$');
    QTest::newRow("many math spans")    << QString("$x^2$ ").repeated(20'000);
    QTest::newRow("many highlights")    << QString("==a== ").repeated(30'000);
    QTest::newRow("many carets")        << QString(100'000, u'^');
    QTest::newRow("many colons")        << QString(":smile:").repeated(20'000) + QString(100'000, u':');
    QTest::newRow("many footnotes")     << QString("x[^1] ").repeated(20'000) + "\n\n[^1]: note\n";
    QTest::newRow("many definitions fn") << [] { QString s; for (int i = 0; i < 5000; ++i) s += QString("r[^%1] ").arg(i); s += "\n\n"; for (int i = 0; i < 5000; ++i) s += QString("[^%1]: n%1\n").arg(i); return s; }();
    QTest::newRow("many wiki links")    << QString("[[Page]] ").repeated(20'000);
    QTest::newRow("many alerts")        << QString("> [!NOTE]\n> x\n\n").repeated(5'000);
    QTest::newRow("many headings")      << QString("## Same title\n\n").repeated(10'000);
    QTest::newRow("big cpp block")      << "```cpp\n" + QString("int x = 42; // c \"s\"\n").repeated(8'000) + "```\n";
    QTest::newRow("unclosed comment")   << "```cpp\n/*" + QString("text\n").repeated(30'000) + "```\n";
    QTest::newRow("table 5000 rows")   << "| a | b |\n|---|---|\n" + QString("| 1 | 2 |\n").repeated(5'000);
}

void RenderRobustnessTest::pathologicalInputsStayFast()
{
    QFETCH(QString, markdown);
    MarkdownHtmlRenderer::Result result;
    const qint64 elapsed = timeFullPipelineMs(markdown, &result);

    QVERIFY(result.isValid());
    QVERIFY2(elapsed < kPathologicalBudgetMs,
             qPrintable(QString("took %1 ms (budget %2 ms)").arg(elapsed).arg(kPathologicalBudgetMs)));
}

void RenderRobustnessTest::oneMegabyteDocumentIsFast()
{
    const QString doc = realisticDocument(1'000'000);
    MarkdownHtmlRenderer::Result result;
    const qint64 elapsed = timeFullPipelineMs(doc, &result);

    QVERIFY(result.isValid());
    qInfo("1 MB document rendered in %lld ms", elapsed);
    QVERIFY2(elapsed < kOneMegabyteBudgetMs,
             qPrintable(QString("took %1 ms (budget %2 ms)").arg(elapsed).arg(kOneMegabyteBudgetMs)));
}

QTEST_APPLESS_MAIN(RenderRobustnessTest)
#include "RenderRobustnessTest.moc"
