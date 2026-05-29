#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <cstdint>

struct PdfContentOperand;

struct PdfContentDictionaryEntry {
    QByteArray key;
    QSharedPointer<PdfContentOperand> value;
    qsizetype offset = 0;
};

enum class PdfContentOperandType : uint8_t {
    Integer,
    Real,
    Boolean,
    Null,
    Name,
    String,
    Array,
    Dictionary,
};

struct PdfContentOperand {
    PdfContentOperandType type;
    qsizetype offset = 0;
    qsizetype length = 0;

    int64_t intValue = 0;
    double realValue = 0.0;
    bool boolValue = false;
    QByteArray bytes;

    QList<PdfContentOperand> arrayElements;
    QList<PdfContentDictionaryEntry> dictionaryEntries;
};

struct PdfContentOperator {
    QByteArray name;
    QList<PdfContentOperand> operands;
    qsizetype offset = 0;
    qsizetype length = 0;
};

enum class PdfContentStreamTokenizerWarning : uint8_t {
    None,
    UnterminatedString,
    MalformedNumber,
    UnterminatedArray,
    UnterminatedDictionary,
    DepthExceeded,
    UnexpectedToken,
    DanglingOperands,
    UnsupportedInlineImage,
    TokenizerError,
};

struct PdfContentStreamResult {
    QList<PdfContentOperator> operators;
    PdfContentStreamTokenizerWarning warning = PdfContentStreamTokenizerWarning::None;
    QString diagnosticMessage;
    QList<PdfContentOperand> danglingOperands;

    bool ok() const { return warning == PdfContentStreamTokenizerWarning::None; }
};

class PdfContentStreamTokenizer {
public:
    static constexpr int MAX_DEPTH = 64;

    PdfContentStreamResult parse(QByteArrayView contentStream);
};
