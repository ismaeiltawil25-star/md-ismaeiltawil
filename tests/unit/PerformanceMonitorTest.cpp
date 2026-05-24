#include <QTest>
#include "utils/PerformanceMonitor.h"

class PerformanceMonitorTest : public QObject
{
    Q_OBJECT

private slots:
    void testTimerRecordsNonNegativeElapsed();
    void testClearResetsAccumulated();
};

void PerformanceMonitorTest::testTimerRecordsNonNegativeElapsed()
{
    PerformanceMonitor::instance().clear();
    {
        PerformanceMonitor::Timer t("test_op");
    }
    qint64 elapsed = PerformanceMonitor::instance().lastTime("test_op");
    QVERIFY2(elapsed >= 0, "Timer must record non-negative elapsed time");
}

void PerformanceMonitorTest::testClearResetsAccumulated()
{
    PerformanceMonitor::instance().clear();
    {
        PerformanceMonitor::Timer t("clear_op");
    }
    QVERIFY(PerformanceMonitor::instance().lastTime("clear_op") >= 0);
    PerformanceMonitor::instance().clear();
    QCOMPARE(PerformanceMonitor::instance().lastTime("clear_op"), 0);
}

QTEST_MAIN(PerformanceMonitorTest)
#include "PerformanceMonitorTest.moc"
