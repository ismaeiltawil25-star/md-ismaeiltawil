// Tests for mdbr::markdown::FenceTracker (CommonMark fenced code block rules).

#include <QTest>
#include "services/markdown/FenceTracker.h"

using mdbr::markdown::FenceTracker;

class FenceTrackerTest : public QObject
{
    Q_OBJECT

private slots:
    void tracksFences_data();
    void tracksFences();
};

// Each row: a document (lines separated by '\n') and the expected
// consume() result for every line, as a string of '1' (in fence) / '0'.
void FenceTrackerTest::tracksFences_data()
{
    QTest::addColumn<QString>("document");
    QTest::addColumn<QString>("expected");

    QTest::newRow("backtick fence")       << "a\n```\ncode\n```\nb"       << "01110";
    QTest::newRow("tilde fence")          << "~~~\ncode\n~~~\nb"          << "1110";
    QTest::newRow("info string")          << "```cpp\nint x;\n```"        << "111";
    QTest::newRow("indented 3 spaces")    << "   ```\ncode\n   ```\nb"    << "1110";
    QTest::newRow("indented 4 is code")   << "    ```\ncode"              << "00";
    QTest::newRow("too short")            << "``\ncode\n``"               << "000";
    QTest::newRow("tilde not closed by backticks") << "~~~\n```\nx\n~~~\nb" << "11110";
    QTest::newRow("shorter close ignored") << "````\n```\nx\n````\nb"     << "11110";
    QTest::newRow("longer close accepted") << "```\nx\n`````\nb"          << "1110";
    QTest::newRow("close with text is content") << "```\n``` x\nmore\n```\nb" << "11110";
    QTest::newRow("backtick in info string is not a fence") << "``` a`b\ntext" << "00";
    QTest::newRow("tilde info may contain backtick") << "~~~ a`b\ntext\n~~~" << "111";
    QTest::newRow("unclosed fence runs to end") << "```\na\nb"            << "111";
}

void FenceTrackerTest::tracksFences()
{
    QFETCH(QString, document);
    QFETCH(QString, expected);

    const QStringList lines = document.split(u'\n');
    QCOMPARE(lines.size(), expected.size());

    FenceTracker tracker;
    QString actual;
    for (const QString &line : lines)
        actual += tracker.consume(line) ? u'1' : u'0';

    QCOMPARE(actual, expected);
}

QTEST_APPLESS_MAIN(FenceTrackerTest)
#include "FenceTrackerTest.moc"
