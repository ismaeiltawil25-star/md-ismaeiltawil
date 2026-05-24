#include "DocumentLoader.h"
#include "utils/ErrorLogger.h"

#include <QFile>
#include <QFileInfo>

DocumentLoader::Result DocumentLoader::load(const QString &filePath)
{
    Result result;

    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        result.errorMessage = QString("File not found:\n%1").arg(filePath);
        ErrorLogger::instance().logError("DocumentLoader", result.errorMessage);
        return result;
    }

    if (info.size() > MaxFileSize) {
        result.errorMessage = QString("File too large (%1 MB max):\n%2")
            .arg(MaxFileSize / (1024 * 1024)).arg(filePath);
        ErrorLogger::instance().logWarning("DocumentLoader", result.errorMessage);
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = QString("Could not open file:\n%1").arg(filePath);
        ErrorLogger::instance().logError("DocumentLoader", result.errorMessage);
        return result;
    }

    MarkdownDocument doc;
    doc.filePath = filePath;
    doc.fileName = info.fileName();
    doc.rawText = QString::fromUtf8(file.readAll());

    file.close();

    ErrorLogger::instance().logInfo("DocumentLoader",
        QString("Loaded: %1 (%2 bytes)").arg(filePath).arg(doc.rawText.size()));

    result.document = doc;
    return result;
}
