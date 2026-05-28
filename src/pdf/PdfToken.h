#pragma once

#include <QByteArray>
#include <cstdint>

enum class PdfTokenType : uint8_t {
    EndOfFile,
    Integer,
    Real,
    Bool,
    Name,
    StringLiteral,
    HexString,
    ArrayStart,
    ArrayEnd,
    DictStart,
    DictEnd,
    StreamKeyword,
    EndStreamKeyword,
    Keyword,
    Invalid
};

struct PdfToken {
    PdfTokenType type = PdfTokenType::EndOfFile;
    qsizetype offset = 0;
    qsizetype length = 0;

    int64_t intValue = 0;
    double realValue = 0.0;
    bool boolValue = false;
    QByteArray bytes;

    static PdfToken makeEndOfFile(qsizetype off) {
        PdfToken t;
        t.type = PdfTokenType::EndOfFile;
        t.offset = off;
        t.length = 0;
        return t;
    }

    static PdfToken makeInteger(int64_t v, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = PdfTokenType::Integer;
        t.offset = off;
        t.length = len;
        t.intValue = v;
        return t;
    }

    static PdfToken makeReal(double v, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = PdfTokenType::Real;
        t.offset = off;
        t.length = len;
        t.realValue = v;
        return t;
    }

    static PdfToken makeBool(bool v, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = PdfTokenType::Bool;
        t.offset = off;
        t.length = len;
        t.boolValue = v;
        return t;
    }

    static PdfToken makeName(QByteArray v, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = PdfTokenType::Name;
        t.offset = off;
        t.length = len;
        t.bytes = v;
        return t;
    }

    static PdfToken makeString(QByteArray v, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = PdfTokenType::StringLiteral;
        t.offset = off;
        t.length = len;
        t.bytes = v;
        return t;
    }

    static PdfToken makeHexString(QByteArray v, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = PdfTokenType::HexString;
        t.offset = off;
        t.length = len;
        t.bytes = v;
        return t;
    }

    static PdfToken makeToken(PdfTokenType type, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = type;
        t.offset = off;
        t.length = len;
        return t;
    }

    static PdfToken makeKeyword(QByteArray v, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = PdfTokenType::Keyword;
        t.offset = off;
        t.length = len;
        t.bytes = v;
        return t;
    }

    static PdfToken makeInvalid(QByteArray v, qsizetype off, qsizetype len) {
        PdfToken t;
        t.type = PdfTokenType::Invalid;
        t.offset = off;
        t.length = len;
        t.bytes = v;
        return t;
    }
};
