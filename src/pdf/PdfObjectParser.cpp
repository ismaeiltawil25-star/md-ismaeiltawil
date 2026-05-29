#include "PdfObjectParser.h"

PdfObjectParser::PdfObjectParser(PdfTokenizer &tokenizer)
    : m_tokenizer(tokenizer) {}

void PdfObjectParser::ensureLookahead(int minSize) {
    while (m_buffer.size() < minSize) {
        m_buffer.append(m_tokenizer.nextToken());
    }
}

PdfTokenResult PdfObjectParser::peek(int offset) {
    ensureLookahead(offset + 1);
    return m_buffer.at(offset);
}

PdfTokenResult PdfObjectParser::take() {
    if (!m_buffer.isEmpty()) {
        return m_buffer.takeFirst();
    }
    return m_tokenizer.nextToken();
}

bool PdfObjectParser::isValidObjectNumber(int64_t n) {
    return n >= 0 && n <= static_cast<int64_t>(std::numeric_limits<int>::max());
}

PdfObjectResult PdfObjectParser::parseObject() {
    return parseObject(0);
}

PdfObjectResult PdfObjectParser::parseObject(int depth) {
    if (depth > MAX_DEPTH) {
        PdfObjectResult r;
        r.warning = PdfObjectParserWarning::DepthExceeded;
        r.diagnosticMessage = QStringLiteral("Maximum nesting depth exceeded");
        return r;
    }

    auto tok = take();

    switch (tok.token.type) {
    case PdfTokenType::Keyword:
        if (tok.token.bytes == "null") {
            PdfObjectResult r;
            r.object = PdfObject::makeNull(tok.token.offset, tok.token.length);
            return r;
        }
        break;

    case PdfTokenType::Integer: {
        PdfObjectResult r;
        r.object = PdfObject::makeInteger(tok.token.intValue, tok.token.offset, tok.token.length);
        return r;
    }

    case PdfTokenType::Real: {
        PdfObjectResult r;
        r.object = PdfObject::makeReal(tok.token.realValue, tok.token.offset, tok.token.length);
        return r;
    }

    case PdfTokenType::Bool: {
        PdfObjectResult r;
        r.object = PdfObject::makeBool(tok.token.boolValue, tok.token.offset, tok.token.length);
        return r;
    }

    case PdfTokenType::Name: {
        PdfObjectResult r;
        r.object = PdfObject::makeName(tok.token.bytes, tok.token.offset, tok.token.length);
        return r;
    }

    case PdfTokenType::StringLiteral:
    case PdfTokenType::HexString: {
        PdfObjectResult r;
        r.object = PdfObject::makeString(tok.token.bytes, tok.token.offset, tok.token.length);
        return r;
    }

    case PdfTokenType::ArrayStart:
        return parseArray(depth + 1);

    case PdfTokenType::DictStart:
        return parseDictionary(depth + 1);

    default:
        break;
    }

    PdfObjectResult r;
    r.warning = PdfObjectParserWarning::UnexpectedToken;
    r.diagnosticMessage = QStringLiteral("Unexpected token");
    return r;
}

PdfObjectResult PdfObjectParser::parseValue(int depth) {
    ensureLookahead(3);
    auto t0 = peek(0);
    auto t1 = peek(1);
    auto t2 = peek(2);

    if (t0.token.type == PdfTokenType::Integer &&
        t1.token.type == PdfTokenType::Integer &&
        t2.token.type == PdfTokenType::Keyword && t2.token.bytes == "R") {

        int64_t objNum = t0.token.intValue;
        int64_t genNum = t1.token.intValue;
        take(); take(); take();

        if (!isValidObjectNumber(objNum) || !isValidObjectNumber(genNum)) {
            PdfObjectResult r;
            r.warning = PdfObjectParserWarning::InvalidIndirectObjectHeader;
            r.diagnosticMessage = QStringLiteral("Invalid reference numbers: %1 %2 R")
                .arg(objNum).arg(genNum);
            return r;
        }

        PdfRef ref;
        ref.objNum = static_cast<int>(objNum);
        ref.genNum = static_cast<int>(genNum);
        qsizetype off = t0.token.offset;
        qsizetype endOff = t2.token.offset + t2.token.length;
        PdfObjectResult r;
        r.object = PdfObject::makeReference(ref, off, endOff - off);
        return r;
    }

    return parseObject(depth);
}

PdfObjectResult PdfObjectParser::parseArray(int depth) {
    if (depth > MAX_DEPTH) {
        PdfObjectResult r;
        r.warning = PdfObjectParserWarning::DepthExceeded;
        r.diagnosticMessage = QStringLiteral("Maximum nesting depth exceeded");
        return r;
    }

    auto arr = QSharedPointer<PdfArray>::create();
    bool missingEnd = true;
    PdfObjectParserWarning warning = PdfObjectParserWarning::None;

    while (true) {
        ensureLookahead(1);
        auto t = peek(0);
        if (t.token.type == PdfTokenType::ArrayEnd) {
            take();
            missingEnd = false;
            break;
        }
        if (t.token.type == PdfTokenType::EndOfFile) break;

        auto elemResult = parseValue(depth + 1);
        if (!elemResult.object) {
            if (elemResult.warning == PdfObjectParserWarning::DepthExceeded) {
                return elemResult;
            }
            warning = elemResult.warning;
        } else {
            arr->elements.append(elemResult.object);
        }
    }

    if (missingEnd && warning == PdfObjectParserWarning::None) {
        warning = PdfObjectParserWarning::UnterminatedArray;
    }

    PdfObjectResult r;
    r.object = PdfObject::makeArray(arr, 0, 0);
    r.warning = warning;
    if (warning == PdfObjectParserWarning::UnterminatedArray) {
        r.diagnosticMessage = QStringLiteral("Unterminated array");
    }
    return r;
}

PdfObjectResult PdfObjectParser::parseDictionary(int depth) {
    if (depth > MAX_DEPTH) {
        PdfObjectResult r;
        r.warning = PdfObjectParserWarning::DepthExceeded;
        r.diagnosticMessage = QStringLiteral("Maximum nesting depth exceeded");
        return r;
    }

    auto dict = QSharedPointer<PdfDictionary>::create();
    bool missingEnd = true;
    PdfObjectParserWarning warning = PdfObjectParserWarning::None;

    while (true) {
        ensureLookahead(1);
        auto keyTok = peek(0);

        if (keyTok.token.type == PdfTokenType::DictEnd) {
            take();
            missingEnd = false;
            break;
        }
        if (keyTok.token.type == PdfTokenType::EndOfFile) break;

        if (keyTok.token.type != PdfTokenType::Name) {
            warning = PdfObjectParserWarning::UnexpectedToken;
            take();
            continue;
        }

        QByteArray key = keyTok.token.bytes;
        take();

        ensureLookahead(1);
        auto valTok = peek(0);
        if (valTok.token.type == PdfTokenType::DictEnd) {
            take();
            missingEnd = false;
            warning = PdfObjectParserWarning::UnterminatedDictEntry;
            break;
        }
        if (valTok.token.type == PdfTokenType::EndOfFile) {
            warning = PdfObjectParserWarning::UnterminatedDictEntry;
            break;
        }

        auto valResult = parseValue(depth + 1);
        if (!valResult.object) {
            if (valResult.warning == PdfObjectParserWarning::DepthExceeded) {
                return valResult;
            }
            warning = valResult.warning;
        } else {
            dict->entries.insert(key, valResult.object);
        }
    }

    if (missingEnd && warning == PdfObjectParserWarning::None) {
        warning = PdfObjectParserWarning::UnterminatedDict;
    }

    qsizetype streamOffsetCapture = -1;
    ensureLookahead(1);
    if (peek(0).token.type == PdfTokenType::StreamKeyword) {
        streamOffsetCapture = peek(0).token.offset;
        if (warning == PdfObjectParserWarning::None) warning = PdfObjectParserWarning::UnsupportedStreamObject;
    }

    PdfObjectResult r;
    r.object = PdfObject::makeDictionary(dict, 0, 0);
    r.warning = warning;
    r.streamKeywordOffset = streamOffsetCapture;
    if (warning == PdfObjectParserWarning::UnterminatedDict) {
        r.diagnosticMessage = QStringLiteral("Unterminated dictionary");
    }
    if (warning == PdfObjectParserWarning::UnsupportedStreamObject) {
        r.diagnosticMessage = QStringLiteral("Stream objects not supported in this version");
    }
    return r;
}

PdfIndirectResult PdfObjectParser::parseIndirectObject() {
    ensureLookahead(3);
    auto t0 = peek(0);
    auto t1 = peek(1);
    auto t2 = peek(2);

    if (t0.token.type != PdfTokenType::Integer ||
        t1.token.type != PdfTokenType::Integer ||
        t2.token.type != PdfTokenType::Keyword || t2.token.bytes != "obj") {
        PdfIndirectResult r;
        r.warning = PdfObjectParserWarning::UnexpectedToken;
        r.diagnosticMessage = QStringLiteral("Expected 'N G obj' pattern");
        return r;
    }

    int64_t objNum = t0.token.intValue;
    int64_t genNum = t1.token.intValue;

    if (!isValidObjectNumber(objNum) || !isValidObjectNumber(genNum)) {
        PdfIndirectResult r;
        r.warning = PdfObjectParserWarning::InvalidIndirectObjectHeader;
        r.diagnosticMessage = QStringLiteral("Object or generation number out of valid range");
        return r;
    }

    take(); take(); take();

    auto bodyResult = parseValue(0);
    if (!bodyResult.object) {
        PdfIndirectResult r;
        r.objNum = static_cast<int>(objNum);
        r.genNum = static_cast<int>(genNum);
        r.warning = bodyResult.warning;
        r.diagnosticMessage = bodyResult.diagnosticMessage;
        return r;
    }

    if (bodyResult.warning == PdfObjectParserWarning::UnsupportedStreamObject) {
        PdfIndirectResult r;
        r.objNum = static_cast<int>(objNum);
        r.genNum = static_cast<int>(genNum);
        r.object = bodyResult.object;
        r.warning = PdfObjectParserWarning::UnsupportedStreamObject;
        r.diagnosticMessage = bodyResult.diagnosticMessage;
        r.streamKeywordOffset = bodyResult.streamKeywordOffset;
        return r;
    }

    auto end = take();
    if (end.token.type != PdfTokenType::Keyword || end.token.bytes != "endobj") {
        PdfIndirectResult r;
        r.objNum = static_cast<int>(objNum);
        r.genNum = static_cast<int>(genNum);
        r.object = bodyResult.object;
        r.warning = PdfObjectParserWarning::MissingEndobj;
        r.diagnosticMessage = QStringLiteral("Missing endobj in indirect object %1 %2")
            .arg(objNum).arg(genNum);
        r.streamKeywordOffset = bodyResult.streamKeywordOffset;
        return r;
    }

    PdfIndirectResult r;
    r.objNum = static_cast<int>(objNum);
    r.genNum = static_cast<int>(genNum);
    r.object = bodyResult.object;
    r.warning = bodyResult.warning;
    r.streamKeywordOffset = bodyResult.streamKeywordOffset;
    if (bodyResult.warning != PdfObjectParserWarning::None) {
        r.diagnosticMessage = bodyResult.diagnosticMessage;
    }
    return r;
}
