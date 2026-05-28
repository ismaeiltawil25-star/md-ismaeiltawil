#include "PdfTokenizer.h"

PdfTokenizer::PdfTokenizer(PdfByteReader &reader)
    : m_reader(reader) {}

bool PdfTokenizer::isDelimiter(char c) {
    return c == '(' || c == ')' || c == '<' || c == '>' ||
           c == '[' || c == ']' || c == '{' || c == '}' ||
           c == '/' || c == '%';
}

bool PdfTokenizer::isPdfWhitespace(char c) {
    auto uc = static_cast<unsigned char>(c);
    return uc == 0x00 || uc == 0x09 || uc == 0x0A || uc == 0x0C || uc == 0x0D || uc == 0x20;
}

bool PdfTokenizer::isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int PdfTokenizer::hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return 0;
}

// ---------------------------------------------------------------------------
// Result factory helpers
// ---------------------------------------------------------------------------

PdfTokenResult PdfTokenizer::makeResult(PdfToken token) {
    PdfTokenResult r;
    r.token = token;
    return r;
}

PdfTokenResult PdfTokenizer::makeWarning(PdfToken token, PdfTokenizerWarning w, QString msg) {
    PdfTokenResult r;
    r.token = token;
    r.warning = w;
    r.diagnosticMessage = msg;
    return r;
}

PdfTokenResult PdfTokenizer::makeEndOfFile(qsizetype off) {
    return makeResult(PdfToken::makeEndOfFile(off));
}

PdfTokenResult PdfTokenizer::makeInteger(int64_t v, qsizetype off, qsizetype len) {
    return makeResult(PdfToken::makeInteger(v, off, len));
}

PdfTokenResult PdfTokenizer::makeReal(double v, qsizetype off, qsizetype len) {
    return makeResult(PdfToken::makeReal(v, off, len));
}

PdfTokenResult PdfTokenizer::makeBool(bool v, qsizetype off, qsizetype len) {
    return makeResult(PdfToken::makeBool(v, off, len));
}

PdfTokenResult PdfTokenizer::makeName(QByteArray v, qsizetype off, qsizetype len) {
    return makeResult(PdfToken::makeName(v, off, len));
}

PdfTokenResult PdfTokenizer::makeString(QByteArray v, qsizetype off, qsizetype len) {
    return makeResult(PdfToken::makeString(v, off, len));
}

PdfTokenResult PdfTokenizer::makeHexString(QByteArray v, qsizetype off, qsizetype len) {
    return makeResult(PdfToken::makeHexString(v, off, len));
}

PdfTokenResult PdfTokenizer::makeToken(PdfTokenType type, qsizetype off, qsizetype len) {
    return makeResult(PdfToken::makeToken(type, off, len));
}

PdfTokenResult PdfTokenizer::makeKeyword(QByteArray v, qsizetype off, qsizetype len) {
    return makeResult(PdfToken::makeKeyword(v, off, len));
}

PdfTokenResult PdfTokenizer::makeInvalid(QByteArray v, qsizetype off, qsizetype len,
                                          PdfTokenizerWarning w, QString msg) {
    auto token = PdfToken::makeInvalid(v, off, len);
    return makeWarning(token, w, msg);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

PdfTokenResult PdfTokenizer::nextToken() {
    if (m_peeked.has_value()) {
        auto result = *m_peeked;
        m_peeked.reset();
        return result;
    }
    return readToken();
}

PdfTokenResult PdfTokenizer::peekToken() {
    if (!m_peeked.has_value()) {
        m_peeked = readToken();
    }
    return *m_peeked;
}

void PdfTokenizer::consumeToken() {
    if (m_peeked.has_value()) {
        m_peeked.reset();
    } else {
        readToken();
    }
}

// ---------------------------------------------------------------------------
// Main dispatch
// ---------------------------------------------------------------------------

PdfTokenResult PdfTokenizer::readToken() {
    m_reader.skipWhitespaceAndComments();
    if (m_reader.eof()) {
        return makeEndOfFile(m_reader.tell());
    }

    qsizetype start = m_reader.tell();
    char c = static_cast<char>(*m_reader.peekByte());

    switch (static_cast<unsigned char>(c)) {
    case '(':
        return readString(start);
    case ')':
        m_reader.readByte();
        return makeInvalid(QByteArray(1, ')'), start, 1,
                           PdfTokenizerWarning::UnexpectedDelimiter,
                           QStringLiteral("Unexpected closing parenthesis"));
    case '<': {
        m_reader.readByte();
        if (!m_reader.eof() && *m_reader.peekByte() == '<') {
            m_reader.readByte();
            return makeToken(PdfTokenType::DictStart, start, 2);
        }
        return readHexString(start);
    }
    case '>': {
        m_reader.readByte();
        if (!m_reader.eof() && *m_reader.peekByte() == '>') {
            m_reader.readByte();
            return makeToken(PdfTokenType::DictEnd, start, 2);
        }
        return makeInvalid(QByteArray(1, '>'), start, 1,
                           PdfTokenizerWarning::UnmatchedRightAngle,
                           QStringLiteral("Unmatched right angle bracket '>'"));
    }
    case '[':
        m_reader.readByte();
        return makeToken(PdfTokenType::ArrayStart, start, 1);
    case ']':
        m_reader.readByte();
        return makeToken(PdfTokenType::ArrayEnd, start, 1);
    case '{':
        m_reader.readByte();
        return makeInvalid(QByteArray(1, '{'), start, 1,
                           PdfTokenizerWarning::UnexpectedDelimiter,
                           QStringLiteral("Unexpected delimiter '{'"));
    case '}':
        m_reader.readByte();
        return makeInvalid(QByteArray(1, '}'), start, 1,
                           PdfTokenizerWarning::UnexpectedDelimiter,
                           QStringLiteral("Unexpected delimiter '}'"));
    case '/':
        return readName(start);
    default:
        return readRegularRun(start);
    }
}

// ---------------------------------------------------------------------------
// Regular-character-run collector + classifier
// ---------------------------------------------------------------------------

PdfTokenResult PdfTokenizer::readRegularRun(qsizetype startOffset) {
    QByteArray run;
    while (!m_reader.eof()) {
        char c = static_cast<char>(*m_reader.peekByte());
        if (isDelimiter(c) || isPdfWhitespace(c)) break;
        run += static_cast<char>(*m_reader.readByte());
    }
    qsizetype span = m_reader.tell() - startOffset;
    return classifyRegularRun(run, startOffset, span);
}

static bool couldStartNumber(char c) {
    return c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9');
}

enum class NumberShape { NotNumber, Integer, Real, Invalid };

static NumberShape classifyNumberShape(const QByteArray &run) {
    if (run.isEmpty()) return NumberShape::NotNumber;

    qsizetype i = 0;
    if (run[i] == '+' || run[i] == '-') i++;

    if (i >= run.size()) return NumberShape::Invalid;

    bool hasDigitBefore = false;
    while (i < run.size() && run[i] >= '0' && run[i] <= '9') {
        hasDigitBefore = true;
        i++;
    }

    if (i >= run.size()) {
        return hasDigitBefore ? NumberShape::Integer : NumberShape::Invalid;
    }

    if (run[i] == '.') {
        i++;
        bool hasDigitAfter = false;
        while (i < run.size() && run[i] >= '0' && run[i] <= '9') {
            hasDigitAfter = true;
            i++;
        }
        if (i < run.size()) return NumberShape::Invalid;
        if (!hasDigitBefore && !hasDigitAfter) return NumberShape::Invalid;
        return NumberShape::Real;
    }

    if (run[i] >= '0' && run[i] <= '9') {
        // Should have been caught by the while loop above
        return NumberShape::Integer;
    }

    return NumberShape::Invalid;
}

PdfTokenResult PdfTokenizer::classifyRegularRun(const QByteArray &run, qsizetype offset, qsizetype length) {
    if (run.isEmpty()) {
        return makeEndOfFile(offset);
    }

    if (couldStartNumber(run[0])) {
        NumberShape shape = classifyNumberShape(run);
        if (shape == NumberShape::Integer) {
            bool ok = false;
            int64_t val = run.toLongLong(&ok);
            if (ok) {
                return makeInteger(val, offset, length);
            }
            return makeInvalid(run, offset, length,
                               PdfTokenizerWarning::IntegerOverflow,
                               QStringLiteral("Integer overflow"));
        }
        if (shape == NumberShape::Real) {
            bool ok = false;
            double val = run.toDouble(&ok);
            if (ok) {
                return makeReal(val, offset, length);
            }
            return makeInvalid(run, offset, length,
                               PdfTokenizerWarning::MalformedNumber,
                               QStringLiteral("Invalid real number"));
        }
        if (shape == NumberShape::Invalid) {
            return makeInvalid(run, offset, length,
                               PdfTokenizerWarning::MalformedNumber,
                               QStringLiteral("Invalid number"));
        }
    }

    if (run == "true")  return makeBool(true, offset, length);
    if (run == "false") return makeBool(false, offset, length);
    if (run == "stream")    return makeToken(PdfTokenType::StreamKeyword, offset, length);
    if (run == "endstream") return makeToken(PdfTokenType::EndStreamKeyword, offset, length);

    return makeKeyword(run, offset, length);
}

// ---------------------------------------------------------------------------
// Name parser
// ---------------------------------------------------------------------------

PdfTokenResult PdfTokenizer::readName(qsizetype startOffset) {
    m_reader.readByte();
    QByteArray decoded;
    PdfTokenizerWarning warning = PdfTokenizerWarning::None;

    while (!m_reader.eof()) {
        char c = static_cast<char>(*m_reader.peekByte());
        if (isDelimiter(c) || isPdfWhitespace(c)) break;
        m_reader.readByte();

        if (c == '#') {
            auto h1o = m_reader.peekByte();
            if (h1o.has_value() && isHexDigit(static_cast<char>(*h1o))) {
                char h1 = static_cast<char>(*m_reader.readByte());
                auto h2o = m_reader.peekByte();
                if (h2o.has_value() && isHexDigit(static_cast<char>(*h2o))) {
                    char h2 = static_cast<char>(*m_reader.readByte());
                    int hi = hexValue(h1);
                    int lo = hexValue(h2);
                    decoded += static_cast<char>((hi << 4) | lo);
                    continue;
                }
                decoded += '#';
                decoded += h1;
                warning = PdfTokenizerWarning::MalformedNameEscape;
                continue;
            }
            decoded += '#';
            warning = PdfTokenizerWarning::MalformedNameEscape;
            continue;
        }
        decoded += c;
    }

    qsizetype span = m_reader.tell() - startOffset;
    auto result = makeName(decoded, startOffset, span);
    if (warning != PdfTokenizerWarning::None) {
        result.warning = warning;
        result.diagnosticMessage = QStringLiteral("Malformed #XX escape in name");
    }
    return result;
}

// ---------------------------------------------------------------------------
// String literal parser
// ---------------------------------------------------------------------------

PdfTokenResult PdfTokenizer::readString(qsizetype startOffset) {
    m_reader.readByte();
    QByteArray decoded;
    int depth = 1;
    PdfTokenizerWarning warning = PdfTokenizerWarning::None;

    while (depth > 0 && !m_reader.eof()) {
        auto bo = m_reader.readByte();
        if (!bo.has_value()) break;
        char c = static_cast<char>(*bo);

        if (c == '\\') {
            if (m_reader.eof()) {
                decoded += '\\';
                break;
            }
            char esc = static_cast<char>(*m_reader.readByte());
            switch (esc) {
            case 'n':  decoded += '\n'; break;
            case 'r':  decoded += '\r'; break;
            case 't':  decoded += '\t'; break;
            case '\\': decoded += '\\'; break;
            case '(':  decoded += '(';  break;
            case ')':  decoded += ')';  break;
            case '\r':
                if (!m_reader.eof() && *m_reader.peekByte() == '\n')
                    m_reader.readByte();
                break;
            case '\n':
                break;
            default:
                if (esc >= '0' && esc <= '7') {
                    QByteArray octal;
                    octal += esc;
                    for (int i = 0; i < 2; ++i) {
                        if (!m_reader.eof()) {
                            char n = static_cast<char>(*m_reader.peekByte());
                            if (n >= '0' && n <= '7') {
                                octal += static_cast<char>(*m_reader.readByte());
                            } else break;
                        }
                    }
                    bool ok = false;
                    int code = octal.toInt(&ok, 8);
                    if (ok) decoded += static_cast<char>(code);
                } else {
                    decoded += esc;
                }
                break;
            }
        } else if (c == '(') {
            decoded += c;
            ++depth;
        } else if (c == ')') {
            --depth;
            if (depth > 0) decoded += c;
        } else {
            decoded += c;
        }
    }

    qsizetype span = m_reader.tell() - startOffset;
    if (depth > 0) {
        warning = PdfTokenizerWarning::UnterminatedString;
    }
    auto result = makeString(decoded, startOffset, span);
    if (warning != PdfTokenizerWarning::None) {
        result.warning = warning;
        result.diagnosticMessage = QStringLiteral("Unterminated string literal");
    }
    return result;
}

// ---------------------------------------------------------------------------
// Hex string parser
// ---------------------------------------------------------------------------

PdfTokenResult PdfTokenizer::readHexString(qsizetype startOffset) {
    QByteArray hexBuf;
    QByteArray sourceBytes;
    bool nonHexFound = false;
    bool foundClosing = false;
    char badChar = 0;

    while (!m_reader.eof()) {
        auto bo = m_reader.readByte();
        if (!bo.has_value()) break;
        char c = static_cast<char>(*bo);
        sourceBytes += c;

        if (c == '>') { foundClosing = true; break; }
        if (isPdfWhitespace(c)) continue;
        if (isHexDigit(c)) {
            hexBuf += c;
            continue;
        }
        nonHexFound = true;
        badChar = c;
        break;
    }

    if (nonHexFound) {
        QByteArray fullBytes;
        fullBytes.reserve(1 + sourceBytes.size());
        fullBytes += '<';
        fullBytes += sourceBytes;
        return makeInvalid(fullBytes, startOffset, m_reader.tell() - startOffset,
                           PdfTokenizerWarning::NonHexCharInHexString,
                           QStringLiteral("Non-hex character '%1' in hex string").arg(QLatin1Char(badChar)));
    }

    PdfTokenizerWarning warning = PdfTokenizerWarning::None;
    if (!foundClosing) {
        warning = PdfTokenizerWarning::UnterminatedHexString;
    }

    if (hexBuf.size() % 2 != 0) hexBuf += '0';

    QByteArray decoded;
    for (int i = 0; i < hexBuf.size(); i += 2) {
        bool ok1 = false, ok2 = false;
        int hi = QByteArray(1, hexBuf[i]).toInt(&ok1, 16);
        int lo = QByteArray(1, hexBuf[i + 1]).toInt(&ok2, 16);
        decoded += static_cast<char>((ok1 ? hi : 0) << 4 | (ok2 ? lo : 0));
    }

    qsizetype span = m_reader.tell() - startOffset;
    auto result = makeHexString(decoded, startOffset, span);
    if (warning != PdfTokenizerWarning::None) {
        result.warning = warning;
        result.diagnosticMessage = QStringLiteral("Unterminated hex string");
    }
    return result;
}
