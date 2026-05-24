#ifndef ERRORLOGGER_H
#define ERRORLOGGER_H

#include <QString>
#include <memory>

enum class ErrorLevel
{
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class ErrorLogger
{
public:
    static ErrorLogger &instance();

    void log(ErrorLevel level, const QString &message);
    void logError(const QString &context, const QString &message);
    void logWarning(const QString &context, const QString &message);
    void logInfo(const QString &context, const QString &message);

    void setMinimumLevel(ErrorLevel level) { m_minLevel = level; }

private:
    ErrorLogger() = default;
    ~ErrorLogger() = default;
    ErrorLogger(const ErrorLogger &) = delete;
    ErrorLogger &operator=(const ErrorLogger &) = delete;

    ErrorLevel m_minLevel = ErrorLevel::Info;

    QString levelToString(ErrorLevel level) const;
};

#endif
