#include <QtTest/QTest>
#include "PdfByteReader.h"
#include "PdfTokenizer.h"
#include "PdfObjectParser.h"
#include <limits>

class PdfObjectParserTest : public QObject {
    Q_OBJECT

private slots:
    void testParseNull();
    void testParseBoolTrue();
    void testParseBoolFalse();
    void testParseInteger();
    void testParseReal();
    void testParseName();
    void testParseNameHexEscape();
    void testParseStringLiteral();
    void testParseHexString();

    void testParseArray();
    void testParseEmptyArray();
    void testParseMixedArray();
    void testParseNestedArray();
    void testParseArrayWithRef();
    void testParseArrayBogusToken();
    void testParseArrayInvalidRefNegative();
    void testParseArrayInvalidRefLarge();

    void testParseDictionary();
    void testParseEmptyDict();
    void testParseDictNameValue();
    void testParseNestedDict();
    void testParseDictBogusToken();

    void testParseIndirectObject();
    void testParseIndirectObjectDict();
    void testParseIndirectObjectRef();
    void testParseIndirectObjectNegativeObj();
    void testParseIndirectObjectNegativeGen();
    void testParseIndirectObjectObjTooLarge();
    void testParseIndirectObjectMissingEndobj();
    void testParseIndirectObjectStream();

    void testParseRefInArray();
    void testParseObjectDoesNotDetectRef();
    void testParseObjectDoesNotDetectIndirect();

    void testDepthExceededArray();
    void testDepthExceededDict();
    void testDepthExceededMixed();

    void testUnterminatedArray();
    void testUnterminatedDict();
    void testDictEntryMissingValue();
    void testDictEntryMissingValueEof();
    void testStreamKeywordWarning();
    void testOffsetAndLength();
    void testIndirectBodyWarningPropagated();
    void testIndirectMissingEndobjPriority();
};

void PdfObjectParserTest::testParseNull() {
    QByteArray data("null");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Null);
    QCOMPARE(result.warning, PdfObjectParserWarning::None);
}

void PdfObjectParserTest::testParseBoolTrue() {
    QByteArray data("true");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Bool);
    QVERIFY(result.object->boolValue);
}

void PdfObjectParserTest::testParseBoolFalse() {
    QByteArray data("false");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Bool);
    QVERIFY(!result.object->boolValue);
}

void PdfObjectParserTest::testParseInteger() {
    QByteArray data("42");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Integer);
    QCOMPARE(result.object->intValue, int64_t(42));
}

void PdfObjectParserTest::testParseReal() {
    QByteArray data("3.14");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Real);
    QCOMPARE(result.object->realValue, 3.14);
}

void PdfObjectParserTest::testParseName() {
    QByteArray data("/Type ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Name);
    QCOMPARE(result.object->nameValue, QByteArray("Type"));
}

void PdfObjectParserTest::testParseNameHexEscape() {
    QByteArray data("/Page#20One ");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Name);
    QCOMPARE(result.object->nameValue, QByteArray("Page One"));
}

void PdfObjectParserTest::testParseStringLiteral() {
    QByteArray data("(Hello)");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::String);
    QCOMPARE(result.object->stringValue, QByteArray("Hello"));
}

void PdfObjectParserTest::testParseHexString() {
    QByteArray data("<48656C6C6F>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::String);
    QCOMPARE(result.object->stringValue, QByteArray("Hello"));
}

void PdfObjectParserTest::testParseArray() {
    QByteArray data("[1 2 3]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.object->arrayValue->elements.size(), 3);
    QCOMPARE(result.object->arrayValue->elements[0]->kind, PdfObjectKind::Integer);
    QCOMPARE(result.object->arrayValue->elements[0]->intValue, int64_t(1));
}

void PdfObjectParserTest::testParseEmptyArray() {
    QByteArray data("[]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.object->arrayValue->elements.size(), 0);
}

void PdfObjectParserTest::testParseMixedArray() {
    QByteArray data("[true /X null 42]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.object->arrayValue->elements.size(), 4);
    QCOMPARE(result.object->arrayValue->elements[0]->kind, PdfObjectKind::Bool);
    QCOMPARE(result.object->arrayValue->elements[1]->kind, PdfObjectKind::Name);
    QCOMPARE(result.object->arrayValue->elements[2]->kind, PdfObjectKind::Null);
    QCOMPARE(result.object->arrayValue->elements[3]->kind, PdfObjectKind::Integer);
}

void PdfObjectParserTest::testParseNestedArray() {
    QByteArray data("[1 [2 [3]]]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.object->arrayValue->elements.size(), 2);
    auto inner1 = result.object->arrayValue->elements[1];
    QCOMPARE(inner1->kind, PdfObjectKind::Array);
    auto inner2 = inner1->arrayValue->elements[1];
    QCOMPARE(inner2->kind, PdfObjectKind::Array);
    QCOMPARE(inner2->arrayValue->elements[0]->intValue, int64_t(3));
}

void PdfObjectParserTest::testParseArrayWithRef() {
    QByteArray data("[10 0 R]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.object->arrayValue->elements.size(), 1);
    QCOMPARE(result.object->arrayValue->elements[0]->kind, PdfObjectKind::Reference);
    QCOMPARE(result.object->arrayValue->elements[0]->refValue.objNum, 10);
    QCOMPARE(result.object->arrayValue->elements[0]->refValue.genNum, 0);
}

void PdfObjectParserTest::testParseArrayBogusToken() {
    QByteArray data("[) 5]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.object->arrayValue->elements.size(), 1);
    QCOMPARE(result.object->arrayValue->elements[0]->kind, PdfObjectKind::Integer);
    QCOMPARE(result.object->arrayValue->elements[0]->intValue, int64_t(5));
    QCOMPARE(result.warning, PdfObjectParserWarning::UnexpectedToken);
}

void PdfObjectParserTest::testParseArrayInvalidRefNegative() {
    QByteArray data("[-1 0 R]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.warning, PdfObjectParserWarning::InvalidIndirectObjectHeader);
}

void PdfObjectParserTest::testParseArrayInvalidRefLarge() {
    QByteArray data("[99999999999 0 R]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.warning, PdfObjectParserWarning::InvalidIndirectObjectHeader);
}

void PdfObjectParserTest::testParseDictionary() {
    QByteArray data("<< /Type /Catalog /Pages 10 0 R >>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    auto dict = result.object->dictValue;
    QCOMPARE(dict->size(), 2);
    QCOMPARE(dict->get("Type")->kind, PdfObjectKind::Name);
    QCOMPARE(dict->get("Type")->nameValue, QByteArray("Catalog"));
    QCOMPARE(dict->get("Pages")->kind, PdfObjectKind::Reference);
    QCOMPARE(dict->get("Pages")->refValue.objNum, 10);
    QCOMPARE(dict->get("Pages")->refValue.genNum, 0);
}

void PdfObjectParserTest::testParseEmptyDict() {
    QByteArray data("<< >>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    QCOMPARE(result.object->dictValue->size(), 0);
}

void PdfObjectParserTest::testParseDictNameValue() {
    QByteArray data("<< /A /B >>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    auto dict = result.object->dictValue;
    QCOMPARE(dict->size(), 1);
    QVERIFY(dict->has("A"));
    QCOMPARE(dict->get("A")->kind, PdfObjectKind::Name);
    QCOMPARE(dict->get("A")->nameValue, QByteArray("B"));
}

void PdfObjectParserTest::testParseNestedDict() {
    QByteArray data("<< /A << /B 1 >> >>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    auto dict = result.object->dictValue;
    QCOMPARE(dict->size(), 1);
    auto inner = dict->get("A");
    QVERIFY(inner != nullptr);
    QCOMPARE(inner->kind, PdfObjectKind::Dictionary);
    QCOMPARE(inner->dictValue->size(), 1);
    QCOMPARE(inner->dictValue->get("B")->intValue, int64_t(1));
}

void PdfObjectParserTest::testParseDictBogusToken() {
    QByteArray data("<< ) 1 >>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    QCOMPARE(result.warning, PdfObjectParserWarning::UnexpectedToken);
}

void PdfObjectParserTest::testParseIndirectObject() {
    QByteArray data("12 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.objNum, 12);
    QCOMPARE(result.genNum, 0);
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Integer);
    QCOMPARE(result.object->intValue, int64_t(42));
    QCOMPARE(result.warning, PdfObjectParserWarning::None);
}

void PdfObjectParserTest::testParseIndirectObjectDict() {
    QByteArray data("5 0 obj << /Type /X >> endobj");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.objNum, 5);
    QCOMPARE(result.genNum, 0);
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    QCOMPARE(result.object->dictValue->size(), 1);
    QCOMPARE(result.object->dictValue->get("Type")->nameValue, QByteArray("X"));
}

void PdfObjectParserTest::testParseIndirectObjectRef() {
    QByteArray data("8 0 obj 10 0 R endobj");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.objNum, 8);
    QCOMPARE(result.genNum, 0);
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Reference);
    QCOMPARE(result.object->refValue.objNum, 10);
    QCOMPARE(result.object->refValue.genNum, 0);
}

void PdfObjectParserTest::testParseIndirectObjectNegativeObj() {
    QByteArray data("-1 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.warning, PdfObjectParserWarning::InvalidIndirectObjectHeader);
    QVERIFY(result.object == nullptr);
}

void PdfObjectParserTest::testParseIndirectObjectNegativeGen() {
    QByteArray data("1 -1 obj 42 endobj");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.warning, PdfObjectParserWarning::InvalidIndirectObjectHeader);
    QVERIFY(result.object == nullptr);
}

void PdfObjectParserTest::testParseIndirectObjectObjTooLarge() {
    QByteArray data("99999999999 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.warning, PdfObjectParserWarning::InvalidIndirectObjectHeader);
    QVERIFY(result.object == nullptr);
}

void PdfObjectParserTest::testParseIndirectObjectMissingEndobj() {
    QByteArray data("5 0 obj 42");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.objNum, 5);
    QCOMPARE(result.genNum, 0);
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Integer);
    QCOMPARE(result.object->intValue, int64_t(42));
    QCOMPARE(result.warning, PdfObjectParserWarning::MissingEndobj);
}

void PdfObjectParserTest::testParseIndirectObjectStream() {
    QByteArray data("1 0 obj << /Length 5 >> stream");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.objNum, 1);
    QCOMPARE(result.genNum, 0);
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    QCOMPARE(result.warning, PdfObjectParserWarning::UnsupportedStreamObject);
    QVERIFY(result.streamKeywordOffset > 0);
}

void PdfObjectParserTest::testParseRefInArray() {
    QByteArray data("[10 0 R]");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.object->arrayValue->elements[0]->kind, PdfObjectKind::Reference);
}

void PdfObjectParserTest::testParseObjectDoesNotDetectRef() {
    QByteArray data("10 0 R");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Integer);
    QCOMPARE(result.object->intValue, int64_t(10));
}

void PdfObjectParserTest::testParseObjectDoesNotDetectIndirect() {
    QByteArray data("5 0 obj 42 endobj");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Integer);
    QCOMPARE(result.object->intValue, int64_t(5));
}

void PdfObjectParserTest::testDepthExceededArray() {
    QByteArray data = QByteArray(150, '[') + QByteArray(150, ']');
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object == nullptr);
    QCOMPARE(result.warning, PdfObjectParserWarning::DepthExceeded);
}

void PdfObjectParserTest::testDepthExceededDict() {
    QByteArray data;
    for (int i = 0; i < 150; ++i) data += "<< /A ";
    data += QByteArray(150, '>');
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object == nullptr);
    QCOMPARE(result.warning, PdfObjectParserWarning::DepthExceeded);
}

void PdfObjectParserTest::testDepthExceededMixed() {
    QByteArray data;
    for (int i = 0; i < 65; ++i) data += "[";
    for (int i = 0; i < 65; ++i) data += "<< /A ";
    data += QByteArray(130, ']');
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object == nullptr);
    QCOMPARE(result.warning, PdfObjectParserWarning::DepthExceeded);
}

void PdfObjectParserTest::testUnterminatedArray() {
    QByteArray data("[1 2 3");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Array);
    QCOMPARE(result.object->arrayValue->elements.size(), 3);
    QCOMPARE(result.warning, PdfObjectParserWarning::UnterminatedArray);
}

void PdfObjectParserTest::testUnterminatedDict() {
    QByteArray data("<< /A 1");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    QCOMPARE(result.object->dictValue->size(), 1);
    QCOMPARE(result.warning, PdfObjectParserWarning::UnterminatedDict);
}

void PdfObjectParserTest::testDictEntryMissingValue() {
    QByteArray data("<< /A >>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    QCOMPARE(result.object->dictValue->size(), 0);
    QCOMPARE(result.warning, PdfObjectParserWarning::UnterminatedDictEntry);
}

void PdfObjectParserTest::testDictEntryMissingValueEof() {
    QByteArray data("<< /A");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    QCOMPARE(result.object->dictValue->size(), 0);
    QVERIFY(result.warning == PdfObjectParserWarning::UnterminatedDictEntry ||
           result.warning == PdfObjectParserWarning::UnterminatedDict);
}

void PdfObjectParserTest::testStreamKeywordWarning() {
    QByteArray data("<< /Length 5 >> stream");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->kind, PdfObjectKind::Dictionary);
    QCOMPARE(result.warning, PdfObjectParserWarning::UnsupportedStreamObject);
    QVERIFY(result.streamKeywordOffset > 0);
}

void PdfObjectParserTest::testOffsetAndLength() {
    QByteArray data("42");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseObject();
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.object->offset, qsizetype(0));
    QCOMPARE(result.object->length, qsizetype(2));
}

void PdfObjectParserTest::testIndirectBodyWarningPropagated() {
    QByteArray data("5 0 obj << /A >> endobj");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.objNum, 5);
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.warning, PdfObjectParserWarning::UnterminatedDictEntry);
}

void PdfObjectParserTest::testIndirectMissingEndobjPriority() {
    QByteArray data("5 0 obj << /A >>");
    PdfByteReader reader(data);
    PdfTokenizer tokenizer(reader);
    PdfObjectParser parser(tokenizer);

    auto result = parser.parseIndirectObject();
    QCOMPARE(result.objNum, 5);
    QVERIFY(result.object != nullptr);
    QCOMPARE(result.warning, PdfObjectParserWarning::MissingEndobj);
}

QTEST_MAIN(PdfObjectParserTest)
#include "PdfObjectParserTest.moc"
