#include "PerformanceMonitor.h"

#include <QDebug>
#include <QDateTime>

PerformanceMonitor &PerformanceMonitor::instance()
{
    static PerformanceMonitor instance;
    return instance;
}

PerformanceMonitor::Timer::Timer(const QString &name)
    : m_name(name)
{
    m_timer.start();
}

PerformanceMonitor::Timer::~Timer()
{
    instance().report(m_name, m_timer.elapsed());
}

void PerformanceMonitor::report(const QString &name, qint64 elapsedMs)
{
    m_totalTimes[name] += elapsedMs;
    m_callCounts[name]++;
    m_lastTimes[name] = elapsedMs;

    if (elapsedMs > 100) {
        qDebug() << "PERF:" << name << "took" << elapsedMs << "ms"
                 << "(avg:" << m_totalTimes[name] / m_callCounts[name] << "ms)";
    }
}

qint64 PerformanceMonitor::lastTime(const QString &name) const
{
    return m_lastTimes.value(name, 0);
}

void PerformanceMonitor::clear()
{
    m_totalTimes.clear();
    m_callCounts.clear();
    m_lastTimes.clear();
}
