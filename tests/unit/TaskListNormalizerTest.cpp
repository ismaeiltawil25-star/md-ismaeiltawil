#include <QTest>
#include "renderers/TaskListNormalizer.h"

class TaskListNormalizerTest : public QObject
{
    Q_OBJECT

private slots:
    void testUncheckedReplacement();
    void testCheckedReplacement();
    void testNoCheckboxNoChange();
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

QTEST_MAIN(TaskListNormalizerTest)
#include "TaskListNormalizerTest.moc"
