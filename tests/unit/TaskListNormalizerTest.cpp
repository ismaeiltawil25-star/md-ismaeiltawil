#include <QTest>
#include <QRegularExpression>
#include "renderers/TaskListNormalizer.h"

class TaskListNormalizerTest : public QObject
{
    Q_OBJECT

private slots:
    void testUncheckedReplacement();
    void testCheckedReplacement();
    void testNoCheckboxNoChange();
    void testUncheckedReorderedAttrs();
    void testCheckedReorderedAttrs();
    void testExtraAttr();
    void testNonTaskCheckboxUnchanged();
};

void TaskListNormalizerTest::testUncheckedReplacement()
{
    QString input = "<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled>";
    QString result = TaskListNormalizer::normalizeTaskLists(input);
    QVERIFY2(!result.contains("<input"), "<input> must be replaced for unchecked");
    QVERIFY2(result.contains(QStringLiteral("☐")), "unchecked must be replaced with ☐");
}

void TaskListNormalizerTest::testCheckedReplacement()
{
    QString input = "<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled checked>";
    QString result = TaskListNormalizer::normalizeTaskLists(input);
    QVERIFY2(!result.contains("<input"), "<input> must be replaced for checked");
    QVERIFY2(result.contains(QStringLiteral("☑")), "checked must be replaced with ☑");
}

void TaskListNormalizerTest::testNoCheckboxNoChange()
{
    QString input = "<p>Just a paragraph without a checkbox.</p>";
    QString result = TaskListNormalizer::normalizeTaskLists(input);
    QCOMPARE(result, input);
}

void TaskListNormalizerTest::testUncheckedReorderedAttrs()
{
    QString input = "<input class=\"task-list-item-checkbox\" type=\"checkbox\" disabled>";
    QString result = TaskListNormalizer::normalizeTaskLists(input);
    QVERIFY2(!result.contains("<input"), "reordered attrs must be replaced for unchecked");
    QVERIFY2(result.contains(QStringLiteral("☐")), "reordered unchecked must produce ☐");
}

void TaskListNormalizerTest::testCheckedReorderedAttrs()
{
    QString input = "<input class=\"task-list-item-checkbox\" type=\"checkbox\" disabled checked>";
    QString result = TaskListNormalizer::normalizeTaskLists(input);
    QVERIFY2(!result.contains("<input"), "reordered attrs must be replaced for checked");
    QVERIFY2(result.contains(QStringLiteral("☑")), "reordered checked must produce ☑");
}

void TaskListNormalizerTest::testExtraAttr()
{
    QString input = "<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled data-x=\"1\">";
    QString result = TaskListNormalizer::normalizeTaskLists(input);
    QVERIFY2(!result.contains("<input"), "extra attr must be replaced for unchecked");
    QVERIFY2(result.contains(QStringLiteral("☐")), "extra attr must produce ☐");
}

void TaskListNormalizerTest::testNonTaskCheckboxUnchanged()
{
    QString input = "<input type=\"checkbox\" disabled>";
    QString result = TaskListNormalizer::normalizeTaskLists(input);
    QVERIFY2(result.contains(QStringLiteral("<input")), "non-task checkbox must remain as <input>");
    QVERIFY2(result.contains(QStringLiteral("type=\"checkbox\"")), "non-task checkbox type must survive");
    QVERIFY2(!result.contains(QStringLiteral("☐")), "non-task checkbox must not produce ☐");
}

QTEST_MAIN(TaskListNormalizerTest)
#include "TaskListNormalizerTest.moc"
