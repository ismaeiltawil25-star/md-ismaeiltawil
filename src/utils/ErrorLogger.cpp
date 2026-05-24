#include "ErrorLogger.h"

#include <QDebug>
#include <QDateTime>

ErrorLogger &ErrorLogger::instance()
{
    static ErrorLogger instance;
    return instance;
}

void ErrorLogger::log(ErrorLevel level, const QString &message)
{
    if (static_cast<int>(level) < static_cast<int>(m_minLevel))
        return;

    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    const QString levelStr = levelToString(level);

    QString formatted = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    if (level == ErrorLevel::Critical || level == ErrorLevel::Error) {
        qCritical() << formatted;
    } else if (level == ErrorLevel::Warning) {
        qWarning() << formatted;
    } else {
        qDebug() << formatted;
    }
}

void ErrorLogger::logError(const QString &context, const QString &message)
{
    log(ErrorLevel::Error, QString("[%1] %2").arg(context, message));
}

void ErrorLogger::logWarning(const QString &context, const QString &message)
{
    log(ErrorLevel::Warning, QString("[%1] %2").arg(context, message));
}

void ErrorLogger::logInfo(const QString &context, const QString &message)
{
    log(ErrorLevel::Info, QString("[%1] %2").arg(context, message));
}

QString ErrorLogger::levelToString(ErrorLevel level) const
{
    switch (level) {
    case ErrorLevel::Debug:    return "DEBUG";
    case ErrorLevel::Info:     return "INFO";
    case ErrorLevel::Warning:  return "WARNING";
    case ErrorLevel::Error:    return "ERROR";
    case ErrorLevel::Critical: return "CRITICAL";
    default:                   return "UNKNOWN";
    }
}
