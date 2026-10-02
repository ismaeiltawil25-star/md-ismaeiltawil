#include "PdfContentStreamTokenizer.h"
#include "PdfByteReader.h"
#include "PdfTokenizer.h"

using W = PdfContentStreamTokenizerWarning;

static W mapTokenizerWarning(PdfTokenizerWarning w) {
    switch (w) {
    case PdfTokenizerWarning::None: return W::None;
    case PdfTokenizerWarning::UnterminatedString:
    case PdfTokenizerWarning::UnterminatedHexString: return W::UnterminatedString;
    case PdfTokenizerWarning::MalformedNumber:
    case PdfTokenizerWarning::IntegerOverflow: return W::MalformedNumber;
    case PdfTokenizerWarning::UnmatchedRightAngle:
    case PdfTokenizerWarning::UnexpectedDelimiter: return W::UnexpectedToken;
    case PdfTokenizerWarning::NonHexCharInHexString:
    case PdfTokenizerWarning::MalformedNameEscape: return W::None;
    }
    return W::TokenizerError;
}

static PdfContentOperand makeOperand(PdfContentOperandType t, qsizetype off, qsizetype len) {
    PdfContentOperand op; op.type = t; op.offset = off; op.length = len; return op;
}
static PdfContentOperand makeIntOperand(int64_t v, qsizetype off, qsizetype len) {
    auto op = makeOperand(PdfContentOperandType::Integer, off, len); op.intValue = v; return op;
}
static PdfContentOperand makeRealOperand(double v, qsizetype off, qsizetype len) {
    auto op = makeOperand(PdfContentOperandType::Real, off, len); op.realValue = v; return op;
}
static PdfContentOperand makeBoolOperand(bool v, qsizetype off, qsizetype len) {
    auto op = makeOperand(PdfContentOperandType::Boolean, off, len); op.boolValue = v; return op;
}
static PdfContentOperand makeNullOperand(qsizetype off, qsizetype len) {
    return makeOperand(PdfContentOperandType::Null, off, len);
}
static PdfContentOperand makeNameOperand(const QByteArray &v, qsizetype off, qsizetype len) {
    auto op = makeOperand(PdfContentOperandType::Name, off, len); op.bytes = v; return op;
}
static PdfContentOperand makeStringOperand(const QByteArray &v, qsizetype off, qsizetype len) {
    auto op = makeOperand(PdfContentOperandType::String, off, len); op.bytes = v; return op;
}

static PdfContentOperand accumulateToken(const PdfToken &tok, W &warn, QString &diag, int depth,
    auto &parseArrayFn, auto &parseDictFn) {
    if (tok.type == PdfTokenType::Bool) return makeBoolOperand(tok.boolValue, tok.offset, tok.length);
    if (tok.type == PdfTokenType::Integer) return makeIntOperand(tok.intValue, tok.offset, tok.length);
    if (tok.type == PdfTokenType::Real) return makeRealOperand(tok.realValue, tok.offset, tok.length);
    if (tok.type == PdfTokenType::Name) return makeNameOperand(tok.bytes, tok.offset, tok.length);
    if (tok.type == PdfTokenType::StringLiteral || tok.type == PdfTokenType::HexString)
        return makeStringOperand(tok.bytes, tok.offset, tok.length);
    if (tok.type == PdfTokenType::ArrayStart) {
        if (depth >= PdfContentStreamTokenizer::MAX_DEPTH) { warn = W::DepthExceeded; return {}; }
        return parseArrayFn(depth + 1, parseArrayFn, parseDictFn, tok.offset);
    }
    if (tok.type == PdfTokenType::DictStart) {
        if (depth >= PdfContentStreamTokenizer::MAX_DEPTH) { warn = W::DepthExceeded; return {}; }
        return parseDictFn(depth + 1, parseArrayFn, parseDictFn, tok.offset);
    }
    if (tok.type == PdfTokenType::Keyword && tok.bytes == "null")
        return makeNullOperand(tok.offset, tok.length);
    // keyword inside array/dict (non-null) or unexpected token type
    warn = W::UnexpectedToken;
    diag = QStringLiteral("Unexpected token in operand context");
    return {};
}

PdfContentStreamResult PdfContentStreamTokenizer::parse(QByteArrayView contentStream) {
    PdfContentStreamResult result;
    PdfByteReader reader(contentStream);
    PdfTokenizer tokenizer(reader);
    QList<PdfContentOperand> operandStack;

    auto emitOperator = [&](const QByteArray &name, qsizetype off, qsizetype len) {
        PdfContentOperator op;
        op.name = name; op.operands = operandStack; op.offset = off; op.length = len;
        result.operators.append(op);
        operandStack.clear();
    };

    auto checkWarn = [&](const PdfTokenResult &tr) -> bool {
        if (!tr.hasWarning()) return false;
        auto w = mapTokenizerWarning(tr.warning);
        if (w == W::None) return false;
        result.warning = w;
        result.diagnosticMessage = tr.diagnosticMessage;
        return true;
    };

    // Recursive parsing uses generic lambdas that receive each other by reference.

    auto parseDictInner = [&](int depth, auto &parseArr, auto &parseDict, qsizetype startOff) -> PdfContentOperand {
        PdfContentOperand dictOp = makeOperand(PdfContentOperandType::Dictionary, startOff, 0);
        for (;;) {
            auto keyTr = tokenizer.nextToken();
            if (keyTr.token.type == PdfTokenType::EndOfFile) {
                result.warning = W::UnterminatedDictionary;
                result.diagnosticMessage = QStringLiteral("Unterminated dictionary");
                return {};
            }
            if (keyTr.token.type == PdfTokenType::DictEnd) {
                dictOp.length = reader.tell() - startOff;
                return dictOp;
            }
            if (keyTr.token.type == PdfTokenType::StreamKeyword ||
                keyTr.token.type == PdfTokenType::EndStreamKeyword) {
                result.warning = W::TokenizerError; return {};
            }
            if (checkWarn(keyTr)) return {};
            if (keyTr.token.type != PdfTokenType::Name) {
                result.warning = W::UnexpectedToken;
                result.diagnosticMessage = QStringLiteral("Dictionary key must be a Name");
                return {};
            }
            QByteArray key = keyTr.token.bytes;

            auto valTr = tokenizer.nextToken();
            if (valTr.token.type == PdfTokenType::EndOfFile || valTr.token.type == PdfTokenType::DictEnd) {
                result.warning = W::UnterminatedDictionary;
                result.diagnosticMessage = QStringLiteral("Unterminated dictionary");
                return {};
            }
            if (checkWarn(valTr)) return {};

            W warn = W::None; QString diag;
            auto val = accumulateToken(valTr.token, warn, diag, depth, parseArr, parseDict);
            if (warn != W::None) { result.warning = warn; result.diagnosticMessage = diag; return {}; }

            PdfContentDictionaryEntry entry;
            entry.key = key; entry.value = QSharedPointer<PdfContentOperand>::create(val);
            entry.offset = valTr.token.offset;
            dictOp.dictionaryEntries.append(entry);
        }
    };

    auto parseArrayInner = [&](int depth, auto &parseArr, auto &parseDict, qsizetype startOff) -> PdfContentOperand {
        PdfContentOperand arrOp = makeOperand(PdfContentOperandType::Array, startOff, 0);
        for (;;) {
            auto tr = tokenizer.nextToken();
            if (tr.token.type == PdfTokenType::EndOfFile) {
                result.warning = W::UnterminatedArray;
                result.diagnosticMessage = QStringLiteral("Unterminated array");
                return {};
            }
            if (tr.token.type == PdfTokenType::ArrayEnd) {
                arrOp.length = reader.tell() - startOff;
                return arrOp;
            }
            if (tr.token.type == PdfTokenType::StreamKeyword ||
                tr.token.type == PdfTokenType::EndStreamKeyword) {
                result.warning = W::TokenizerError; return {};
            }
            if (checkWarn(tr)) return {};

            W warn = W::None; QString diag;
            auto elem = accumulateToken(tr.token, warn, diag, depth, parseArr, parseDict);
            if (warn != W::None) { result.warning = warn; result.diagnosticMessage = diag; return {}; }
            arrOp.arrayElements.append(elem);
        }
    };

    // Top-level loop
    for (;;) {
        auto tr = tokenizer.nextToken();

        if (tr.token.type == PdfTokenType::EndOfFile) break;

        if (tr.token.type == PdfTokenType::StreamKeyword ||
            tr.token.type == PdfTokenType::EndStreamKeyword) {
            result.warning = W::TokenizerError;
            result.diagnosticMessage = QStringLiteral("Stream/endstream keyword in content stream");
            return result;
        }

        if (checkWarn(tr)) return result;

        const auto &tok = tr.token;

        // Operands
        if (tok.type == PdfTokenType::Bool || tok.type == PdfTokenType::Integer ||
            tok.type == PdfTokenType::Real || tok.type == PdfTokenType::Name ||
            tok.type == PdfTokenType::StringLiteral || tok.type == PdfTokenType::HexString) {
            W warn = W::None; QString diag;
            auto op = accumulateToken(tok, warn, diag, 0, parseArrayInner, parseDictInner);
            if (warn != W::None) { result.warning = warn; result.diagnosticMessage = diag; return result; }
            operandStack.append(op);
        } else if (tok.type == PdfTokenType::ArrayStart) {
            auto arr = parseArrayInner(1, parseArrayInner, parseDictInner, tok.offset);
            if (arr.length == 0 && result.warning != W::None) return result;
            operandStack.append(arr);
        } else if (tok.type == PdfTokenType::DictStart) {
            auto dict = parseDictInner(1, parseArrayInner, parseDictInner, tok.offset);
            if (dict.length == 0 && result.warning != W::None) return result;
            operandStack.append(dict);
        } else if (tok.type == PdfTokenType::Keyword) {
            if (tok.bytes == "null") {
                operandStack.append(makeNullOperand(tok.offset, tok.length));
            } else if (tok.bytes == "BI") {
                result.warning = W::UnsupportedInlineImage;
                result.diagnosticMessage = QStringLiteral("Inline images are not supported");
                return result;
            } else {
                emitOperator(tok.bytes, tok.offset, tok.length);
            }
        }
    }

    if (!operandStack.isEmpty()) {
        result.warning = W::DanglingOperands;
        result.diagnosticMessage = QStringLiteral("%1 operand(s) without operator at end of content stream")
            .arg(operandStack.size());
        result.danglingOperands = operandStack;
    }

    return result;
}
