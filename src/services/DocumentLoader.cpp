#include "DocumentLoader.h"
#include "services/text/TextDecoder.h"
#include "utils/ErrorLogger.h"

#include <QFile>
#include <QFileInfo>

#include <utility>

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
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = QString("Could not open file:\n%1").arg(filePath);
        ErrorLogger::instance().logError("DocumentLoader", result.errorMessage);
        return result;
    }

    // Read raw bytes (no QIODevice::Text): line endings are normalized only after
    // decoding, otherwise byte-level CR/LF translation could corrupt UTF-16 input.
    const QByteArray bytes = file.readAll();
    file.close();

    auto decoded = mdbr::text::decode(bytes);

    MarkdownDocument doc;
    doc.filePath = filePath;
    doc.fileName = info.fileName();
    doc.rawText = mdbr::text::normalizeLineEndings(std::move(decoded.text));
    doc.encodingName = mdbr::text::encodingName(decoded.encoding);
    doc.encodingGuessed = decoded.isGuessed;

    const QString summary = QString("Loaded: %1 (%2 bytes, %3%4)")
        .arg(filePath).arg(bytes.size()).arg(doc.encodingName,
             doc.encodingGuessed ? QStringLiteral(", guessed") : QString());
    if (doc.encodingGuessed)
        ErrorLogger::instance().logWarning("DocumentLoader", summary);
    else
        ErrorLogger::instance().logInfo("DocumentLoader", summary);

    result.document = doc;
    return result;
}
