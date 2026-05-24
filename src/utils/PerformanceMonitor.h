#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <QString>
#include <QElapsedTimer>
#include <QHash>

class PerformanceMonitor
{
public:
    static PerformanceMonitor &instance();

    class Timer
    {
    public:
        explicit Timer(const QString &name);
        ~Timer();

    private:
        QString m_name;
        QElapsedTimer m_timer;
    };

    void report(const QString &name, qint64 elapsedMs);

private:
    PerformanceMonitor() = default;
    ~PerformanceMonitor() = default;
    PerformanceMonitor(const PerformanceMonitor &) = delete;
    PerformanceMonitor &operator=(const PerformanceMonitor &) = delete;

    QHash<QString, qint64> m_totalTimes;
    QHash<QString, int> m_callCounts;
};

#endif
