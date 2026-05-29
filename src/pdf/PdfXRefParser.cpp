#include "PdfXRefParser.h"
#include <QByteArray>
#include <QList>
#include <limits>

PdfXRefParserResult PdfXRefParser::parse(PdfByteReader &reader, qsizetype startxref) {
    PdfXRefParserResult result;
    qsizetype originalPos = reader.tell();
    qsizetype totalSize = reader.dataView().size();

    if (startxref < 0 || startxref >= totalSize) {
        reader.seek(originalPos);
        result.warning = PdfXRefParserWarning::InvalidStartxref;
        result.diagnosticMessage = QStringLiteral("Invalid startxref offset: %1 (size=%2)")
            .arg(startxref).arg(totalSize);
        return result;
    }

    reader.seek(startxref);
    result.table.startxref = startxref;

    reader.skipAsciiWhitespace();
    auto xrefLine = reader.readLine();
    if (xrefLine.trimmed() != "xref") {
        result.warning = PdfXRefParserWarning::MissingXrefKeyword;
        result.diagnosticMessage = QStringLiteral("Expected 'xref' keyword");
        return result;
    }

    while (true) {
        reader.skipAsciiWhitespace();
        if (reader.eof()) break;

        qsizetype lineStart = reader.tell();
        auto line = reader.readLine();
        auto trimmed = line.trimmed();

        if (trimmed == "trailer") {
            result.trailerOffset = lineStart;
            reader.seek(lineStart);
            return result;
        }

        if (line.size() > 0 && line[0] == 't' && trimmed != "trailer") {
            result.warning = PdfXRefParserWarning::UnexpectedToken;
            result.diagnosticMessage = QStringLiteral("Unexpected token after xref entries");
            return result;
        }

        // Parse subsection header: exactly two unsigned integer fields
        QByteArray headerData = trimmed.toByteArray();
        QList<QByteArray> tokens = headerData.split(' ');
        tokens.removeAll(QByteArray());

        if (tokens.size() != 2) {
            result.warning = PdfXRefParserWarning::InvalidSubsectionHeader;
            result.diagnosticMessage = QStringLiteral("Expected two integers in subsection header");
            return result;
        }

        bool ok1 = false, ok2 = false;
        int64_t startObjNum = tokens[0].toLongLong(&ok1);
        int64_t count = tokens[1].toLongLong(&ok2);

        if (!ok1 || !ok2 || startObjNum < 0 || count < 0) {
            result.warning = PdfXRefParserWarning::InvalidSubsectionHeader;
            result.diagnosticMessage = QStringLiteral("Invalid subsection header values");
            return result;
        }

        if (startObjNum > std::numeric_limits<int>::max() ||
            count > std::numeric_limits<int>::max()) {
            result.warning = PdfXRefParserWarning::InvalidSubsectionHeader;
            result.diagnosticMessage = QStringLiteral("Subsection header values out of range");
            return result;
        }

        int startNum = static_cast<int>(startObjNum);
        int entryCount = static_cast<int>(count);

        if (startNum > std::numeric_limits<int>::max() - entryCount) {
            result.warning = PdfXRefParserWarning::InvalidSubsectionHeader;
            result.diagnosticMessage = QStringLiteral("Subsection header overflow");
            return result;
        }

        PdfXRefSubsection subsection;
        subsection.startObjNum = startNum;
        subsection.count = entryCount;

        for (int i = 0; i < entryCount; ++i) {
            if (reader.eof()) {
                result.table.subsections.append(subsection);
                result.warning = PdfXRefParserWarning::TruncatedEntry;
                result.diagnosticMessage = QStringLiteral("Unexpected end of data while reading xref entries");
                return result;
            }

            auto entryLine = reader.readLine();
            if (entryLine.size() != 18) {
                result.warning = PdfXRefParserWarning::MalformedEntry;
                result.diagnosticMessage = QStringLiteral("XRef entry line must be exactly 18 bytes");
                continue;
            }

            // Validate: 10 digits, space, 5 digits, space, n/f
            bool valid = true;
            for (int j = 0; j < 10; ++j) {
                char c = entryLine[j];
                if (c < '0' || c > '9') { valid = false; break; }
            }
            if (valid && entryLine[10] != ' ') valid = false;
            if (valid) {
                for (int j = 11; j < 16; ++j) {
                    char c = entryLine[j];
                    if (c < '0' || c > '9') { valid = false; break; }
                }
            }
            if (valid && entryLine[16] != ' ') valid = false;
            if (valid && entryLine[17] != 'n' && entryLine[17] != 'f') valid = false;

            if (!valid) {
                result.warning = PdfXRefParserWarning::MalformedEntry;
                result.diagnosticMessage = QStringLiteral("Malformed xref entry");
                continue;
            }

            PdfXRefEntry entry;
            entry.offset = parseDecimal10(entryLine.left(10));
            entry.genNum = static_cast<int>(parseDecimal10(entryLine.mid(11, 5)));
            entry.type = (entryLine[17] == 'n') ? PdfXRefEntryType::InUse : PdfXRefEntryType::Free;
            subsection.entries.append(entry);
        }

        result.table.subsections.append(subsection);
    }

    return result;
}

qsizetype PdfXRefParser::parseDecimal10(const QByteArrayView &view) {
    qsizetype value = 0;
    for (qsizetype i = 0; i < view.size(); ++i) {
        value = value * 10 + (view[i] - '0');
    }
    return value;
}
