#pragma once

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QSharedPointer>
#include <cstdint>
#include <QtTypes>

enum class PdfObjectKind : uint8_t {
    Null,
    Bool,
    Integer,
    Real,
    Name,
    String,
    Array,
    Dictionary,
    Reference
};

struct PdfRef {
    int objNum = 0;
    int genNum = 0;
};

using PdfObjectPtr = QSharedPointer<class PdfObject>;

struct PdfArray {
    QList<PdfObjectPtr> elements;
};

struct PdfDictionary {
    QMap<QByteArray, PdfObjectPtr> entries;

    PdfObjectPtr get(const QByteArray &key) const {
        auto it = entries.constFind(key);
        return it != entries.constEnd() ? *it : PdfObjectPtr{};
    }

    bool has(const QByteArray &key) const {
        return entries.contains(key);
    }

    int size() const { return static_cast<int>(entries.size()); }
};

struct PdfObject {
    PdfObjectKind kind = PdfObjectKind::Null;
    qsizetype offset = 0;
    qsizetype length = 0;

    bool boolValue = false;
    int64_t intValue = 0;
    double realValue = 0.0;
    QByteArray nameValue;
    QByteArray stringValue;
    PdfRef refValue;
    QSharedPointer<PdfArray> arrayValue;
    QSharedPointer<PdfDictionary> dictValue;

    static PdfObjectPtr makeNull(qsizetype off, qsizetype len);
    static PdfObjectPtr makeBool(bool v, qsizetype off, qsizetype len);
    static PdfObjectPtr makeInteger(int64_t v, qsizetype off, qsizetype len);
    static PdfObjectPtr makeReal(double v, qsizetype off, qsizetype len);
    static PdfObjectPtr makeName(QByteArray v, qsizetype off, qsizetype len);
    static PdfObjectPtr makeString(QByteArray v, qsizetype off, qsizetype len);
    static PdfObjectPtr makeArray(QSharedPointer<PdfArray> arr, qsizetype off, qsizetype len);
    static PdfObjectPtr makeDictionary(QSharedPointer<PdfDictionary> dict, qsizetype off, qsizetype len);
    static PdfObjectPtr makeReference(PdfRef ref, qsizetype off, qsizetype len);
};

inline PdfObjectPtr PdfObject::makeNull(qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::Null;
    obj->offset = off;
    obj->length = len;
    return obj;
}

inline PdfObjectPtr PdfObject::makeBool(bool v, qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::Bool;
    obj->offset = off;
    obj->length = len;
    obj->boolValue = v;
    return obj;
}

inline PdfObjectPtr PdfObject::makeInteger(int64_t v, qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::Integer;
    obj->offset = off;
    obj->length = len;
    obj->intValue = v;
    return obj;
}

inline PdfObjectPtr PdfObject::makeReal(double v, qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::Real;
    obj->offset = off;
    obj->length = len;
    obj->realValue = v;
    return obj;
}

inline PdfObjectPtr PdfObject::makeName(QByteArray v, qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::Name;
    obj->offset = off;
    obj->length = len;
    obj->nameValue = v;
    return obj;
}

inline PdfObjectPtr PdfObject::makeString(QByteArray v, qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::String;
    obj->offset = off;
    obj->length = len;
    obj->stringValue = v;
    return obj;
}

inline PdfObjectPtr PdfObject::makeArray(QSharedPointer<PdfArray> arr, qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::Array;
    obj->offset = off;
    obj->length = len;
    obj->arrayValue = arr;
    return obj;
}

inline PdfObjectPtr PdfObject::makeDictionary(QSharedPointer<PdfDictionary> dict, qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::Dictionary;
    obj->offset = off;
    obj->length = len;
    obj->dictValue = dict;
    return obj;
}

inline PdfObjectPtr PdfObject::makeReference(PdfRef ref, qsizetype off, qsizetype len) {
    auto obj = QSharedPointer<PdfObject>::create();
    obj->kind = PdfObjectKind::Reference;
    obj->offset = off;
    obj->length = len;
    obj->refValue = ref;
    return obj;
}
