#pragma once

#include "PdfByteReader.h"
#include "PdfToken.h"
#include <QString>
#include <optional>

enum class PdfTokenizerWarning : uint8_t {
    None,
    NonHexCharInHexString,
    MalformedNameEscape,
    UnterminatedString,
    UnterminatedHexString,
    MalformedNumber,
    IntegerOverflow,
    UnmatchedRightAngle,
    UnexpectedDelimiter
};

struct PdfTokenResult {
    PdfToken token;
    PdfTokenizerWarning warning = PdfTokenizerWarning::None;
    QString diagnosticMessage;

    bool hasWarning() const {
        return warning != PdfTokenizerWarning::None;
    }
};

class PdfTokenizer {
public:
    explicit PdfTokenizer(PdfByteReader &reader);

    PdfTokenResult nextToken();
    PdfTokenResult peekToken();
    void consumeToken();

private:
    PdfByteReader &m_reader;
    std::optional<PdfTokenResult> m_peeked;

    static bool isDelimiter(char c);
    static bool isPdfWhitespace(char c);
    static bool isHexDigit(char c);
    static int hexValue(char c);
    static PdfTokenResult makeResult(PdfToken token);
    static PdfTokenResult makeWarning(PdfToken token, PdfTokenizerWarning w, QString msg);
    static PdfTokenResult makeEndOfFile(qsizetype off);
    static PdfTokenResult makeInteger(int64_t v, qsizetype off, qsizetype len);
    static PdfTokenResult makeReal(double v, qsizetype off, qsizetype len);
    static PdfTokenResult makeBool(bool v, qsizetype off, qsizetype len);
    static PdfTokenResult makeName(QByteArray v, qsizetype off, qsizetype len);
    static PdfTokenResult makeString(QByteArray v, qsizetype off, qsizetype len);
    static PdfTokenResult makeHexString(QByteArray v, qsizetype off, qsizetype len);
    static PdfTokenResult makeToken(PdfTokenType type, qsizetype off, qsizetype len);
    static PdfTokenResult makeKeyword(QByteArray v, qsizetype off, qsizetype len);
    static PdfTokenResult makeInvalid(QByteArray v, qsizetype off, qsizetype len,
                                       PdfTokenizerWarning w, QString msg);

    PdfTokenResult readToken();
    PdfTokenResult readRegularRun(qsizetype startOffset);
    PdfTokenResult classifyRegularRun(const QByteArray &run, qsizetype offset, qsizetype length);
    PdfTokenResult readName(qsizetype startOffset);
    PdfTokenResult readString(qsizetype startOffset);
    PdfTokenResult readHexString(qsizetype startOffset);
};
