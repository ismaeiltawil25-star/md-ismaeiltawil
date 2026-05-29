#include "PdfStreamBodyReader.h"

int PdfStreamBodyReader::consumeSingleEol(PdfByteReader &reader) {
    if (reader.eof()) return 0;

    uint8_t c = reader.peekByte().value();
    if (c == 0x0A) {
        reader.readByte();
        return 1;
    }
    if (c == 0x0D) {
        reader.readByte();
        if (!reader.eof() && reader.peekByte().value() == 0x0A) {
            reader.readByte();
            return 2;
        }
        return 1;
    }
    return 0;
}

bool PdfStreamBodyReader::isPdfBoundary(uint8_t byte) {
    if (byte == 0x00 || byte == 0x09 || byte == 0x0A ||
        byte == 0x0C || byte == 0x0D || byte == 0x20) return true;
    if (byte == '(' || byte == ')' || byte == '<' || byte == '>' ||
        byte == '[' || byte == ']' || byte == '{' || byte == '}' ||
        byte == '/' || byte == '%') return true;
    return false;
}

PdfStreamBodyResult PdfStreamBodyReader::readBody(PdfByteReader &reader,
                                                    qsizetype streamKeywordOffset,
                                                    qsizetype length) {
    const qsizetype originalPos = reader.tell();

    // 1. Validate streamKeywordOffset
    if (streamKeywordOffset < 0 || streamKeywordOffset >= reader.dataView().size()) {
        PdfStreamBodyResult result;
        result.warning = PdfStreamBodyReaderWarning::InvalidStreamOffset;
        result.diagnosticMessage = QStringLiteral("Invalid stream keyword offset: %1").arg(streamKeywordOffset);
        return result;
    }

    // 2. Validate length
    if (length < 0) {
        PdfStreamBodyResult result;
        result.warning = PdfStreamBodyReaderWarning::InvalidLength;
        result.diagnosticMessage = QStringLiteral("Invalid stream length: %1").arg(length);
        return result;
    }

    // 3. Non-mutating keyword check
    QByteArrayView marker = reader.viewAt(streamKeywordOffset, 6);
    if (marker.size() < 6 || marker != QByteArrayView("stream")) {
        PdfStreamBodyResult result;
        result.warning = PdfStreamBodyReaderWarning::MissingStreamKeyword;
        result.diagnosticMessage = QStringLiteral("Missing 'stream' keyword at offset %1").arg(streamKeywordOffset);
        return result;
    }

    // 4. Seek and consume "stream"
    reader.seek(streamKeywordOffset);
    reader.readBytes(6);

    // 5. Required EOL after "stream"
    int eolAfterStream = consumeSingleEol(reader);
    if (eolAfterStream == 0) {
        reader.seek(originalPos);
        PdfStreamBodyResult result;
        result.warning = PdfStreamBodyReaderWarning::MissingStreamEol;
        result.diagnosticMessage = QStringLiteral("Missing end-of-line after 'stream' keyword");
        return result;
    }

    // 6. Body availability check
    const qsizetype bodyStart = reader.tell();
    const qsizetype available = reader.dataView().size() - bodyStart;

    if (available < length) {
        reader.seek(originalPos);
        PdfStreamBodyResult result;
        result.warning = PdfStreamBodyReaderWarning::DataTooShort;
        result.diagnosticMessage = QStringLiteral("Stream body too short: expected %1 bytes, found %2")
            .arg(length).arg(available);
        return result;
    }

    // 7. Read body
    QByteArrayView body;
    if (length > 0) {
        body = reader.readBytes(length).value();
    }

    // 8. Post-body validation
    // 8a. Optional single EOL before endstream
    consumeSingleEol(reader);

    // 8b. endstream check
    const qsizetype endstreamCheckPos = reader.tell();
    QByteArrayView endstreamCandidate = reader.viewAt(endstreamCheckPos, 9);

    if (endstreamCandidate.size() < 9) {
        PdfStreamBodyResult result;
        result.warning = PdfStreamBodyReaderWarning::MissingEndstream;
        result.diagnosticMessage = QStringLiteral("Unexpected end of file before 'endstream'");
        result.body = body;
        return result;
    }

    if (endstreamCandidate != QByteArrayView("endstream")) {
        PdfStreamBodyResult result;
        result.warning = PdfStreamBodyReaderWarning::InvalidEndstreamBoundary;
        result.diagnosticMessage = QStringLiteral("Invalid content before 'endstream' boundary");
        result.body = body;
        return result;
    }

    // 8c. Consume "endstream" and check boundary
    reader.readBytes(9);

    if (!reader.eof()) {
        uint8_t nextByte = reader.peekByte().value();
        if (!isPdfBoundary(nextByte)) {
            PdfStreamBodyResult result;
            result.warning = PdfStreamBodyReaderWarning::InvalidEndstreamBoundary;
            result.diagnosticMessage = QStringLiteral("'endstream' not followed by word boundary");
            result.body = body;
            return result;
        }
    }

    PdfStreamBodyResult result;
    result.body = body;
    return result;
}
