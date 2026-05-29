#pragma once

#include "PdfTokenizer.h"
#include "PdfObject.h"
#include <QString>
#include <QList>
#include <limits>

enum class PdfObjectParserWarning : uint8_t {
    None,
    DepthExceeded,
    UnexpectedToken,
    UnterminatedArray,
    UnterminatedDict,
    UnterminatedDictEntry,
    MissingEndobj,
    UnsupportedStreamObject,
    InvalidIndirectObjectHeader,
};

struct PdfObjectResult {
    PdfObjectPtr object;
    PdfObjectParserWarning warning = PdfObjectParserWarning::None;
    QString diagnosticMessage;
    qsizetype streamKeywordOffset = -1;

    bool hasWarning() const { return warning != PdfObjectParserWarning::None; }
};

struct PdfIndirectResult {
    int objNum = 0;
    int genNum = 0;
    PdfObjectPtr object;
    PdfObjectParserWarning warning = PdfObjectParserWarning::None;
    QString diagnosticMessage;
    qsizetype streamKeywordOffset = -1;
};

class PdfObjectParser {
public:
    static constexpr int MAX_DEPTH = 128;

    explicit PdfObjectParser(PdfTokenizer &tokenizer);

    PdfObjectResult parseObject();
    PdfIndirectResult parseIndirectObject();

private:
    PdfTokenizer &m_tokenizer;
    QList<PdfTokenResult> m_buffer;

    void ensureLookahead(int minSize);
    PdfTokenResult peek(int offset);
    PdfTokenResult take();

    static bool isValidObjectNumber(int64_t n);

    PdfObjectResult parseObject(int depth);
    PdfObjectResult parseValue(int depth);
    PdfObjectResult parseArray(int depth);
    PdfObjectResult parseDictionary(int depth);
};
