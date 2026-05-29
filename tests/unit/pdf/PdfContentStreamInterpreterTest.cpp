#include <QtTest/QTest>
#include "PdfContentStreamInterpreter.h"

static PdfContentOperand intOp(int64_t v, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand op; op.type = PdfContentOperandType::Integer; op.intValue = v; op.offset = off; op.length = len; return op;
}
static PdfContentOperand realOp(double v, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand op; op.type = PdfContentOperandType::Real; op.realValue = v; op.offset = off; op.length = len; return op;
}
static PdfContentOperand nameOp(const QByteArray &v, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand op; op.type = PdfContentOperandType::Name; op.bytes = v; op.offset = off; op.length = len; return op;
}
static PdfContentOperand strOp(const QByteArray &v, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand op; op.type = PdfContentOperandType::String; op.bytes = v; op.offset = off; op.length = len; return op;
}
static PdfContentOperand arrOp(QList<PdfContentOperand> elems, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand op; op.type = PdfContentOperandType::Array; op.arrayElements = elems; op.offset = off; op.length = len; return op;
}
static PdfContentOperand boolOp(bool v, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand op; op.type = PdfContentOperandType::Boolean; op.boolValue = v; op.offset = off; op.length = len; return op;
}
static PdfContentOperand nullOp(qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand op; op.type = PdfContentOperandType::Null; op.offset = off; op.length = len; return op;
}
static PdfContentOperator makeOp(const QByteArray &name, QList<PdfContentOperand> operands,
                                  qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperator op; op.name = name; op.operands = operands; op.offset = off; op.length = len; return op;
}

class PdfContentStreamInterpreterTest : public QObject {
    Q_OBJECT

private slots:
    void testInterpretEmpty();
    void testInterpretSimpleText();
    void testInterpretGraphicsPath();
    void testInterpretTJ();
    void testInterpretTJInvalidElement();
    void testInterpretUnknownOperator();
    void testInterpretInvalidOperandCount();
    void testInterpretInvalidOperandType();
    void testInterpretQUnderflow();
    void testInterpretStackBalance();
    void testInterpretUnbalancedQAtEOF();
    void testInterpretOperandsPreserved();
    void testInterpretColorGeneric();
    void testInterpretTm();
    void testInterpretTdAndTD();
    void testInterpretTStar();
    void testInterpretQuoteOperator();
    void testInterpretDoubleQuoteOperator();
    void testInterpretInvalidCountForZeroArgOp();
    void testInterpretInvalidNumericType();
    void testInterpretTfInvalidFirstOperand();
    void testInterpretCmInvalidCount();
    void testInterpretAllPathPainting();
    void testInterpretUnknownBeforeFatal();
};

void PdfContentStreamInterpreterTest::testInterpretEmpty() {
    PdfContentStreamInterpreter interp;
    auto r = interp.interpret({});
    QVERIFY(r.ok());
    QVERIFY(r.operations.isEmpty());
}

void PdfContentStreamInterpreterTest::testInterpretSimpleText() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("BT", {}),
        makeOp("Tf", {nameOp("F1"), intOp(12)}),
        makeOp("Td", {intOp(72), intOp(720)}),
        makeOp("Tj", {strOp("Hello")}),
        makeOp("ET", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations.size(), 5);
    QCOMPARE(r.operations[0].type, PdfContentOperationType::BeginText);
    QCOMPARE(r.operations[1].type, PdfContentOperationType::SetFont);
    QCOMPARE(r.operations[1].nameOrString, QByteArray("F1"));
    QCOMPARE(r.operations[1].fontSize, 12.0);
    QCOMPARE(r.operations[2].type, PdfContentOperationType::MoveTextPosition);
    QCOMPARE(r.operations[3].type, PdfContentOperationType::ShowText);
    QCOMPARE(r.operations[3].nameOrString, QByteArray("Hello"));
    QCOMPARE(r.operations[4].type, PdfContentOperationType::EndText);
}

void PdfContentStreamInterpreterTest::testInterpretGraphicsPath() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("q", {}),
        makeOp("cm", {intOp(1),intOp(0),intOp(0),intOp(1),intOp(0),intOp(0)}),
        makeOp("re", {intOp(0),intOp(0),intOp(100),intOp(100)}),
        makeOp("S", {}),
        makeOp("Q", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations.size(), 5);
    QCOMPARE(r.operations[0].type, PdfContentOperationType::PushState);
    QCOMPARE(r.operations[1].type, PdfContentOperationType::ConcatMatrix);
    QCOMPARE(r.operations[2].type, PdfContentOperationType::Rectangle);
    QCOMPARE(r.operations[2].numeric[0], 0.0);
    QCOMPARE(r.operations[2].numeric[1], 0.0);
    QCOMPARE(r.operations[2].numeric[2], 100.0);
    QCOMPARE(r.operations[2].numeric[3], 100.0);
    QCOMPARE(r.operations[3].type, PdfContentOperationType::Stroke);
    QCOMPARE(r.operations[4].type, PdfContentOperationType::PopState);
}

void PdfContentStreamInterpreterTest::testInterpretTJ() {
    PdfContentStreamInterpreter interp;
    auto arr = arrOp({strOp("H"), intOp(20), strOp("ello")});
    QList<PdfContentOperator> ops = {
        makeOp("BT", {}),
        makeOp("TJ", {arr}),
        makeOp("ET", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations.size(), 3);
    QCOMPARE(r.operations[1].type, PdfContentOperationType::ShowTextWithOffsets);
    QCOMPARE(r.operations[1].operands.size(), 1);
    QCOMPARE(r.operations[1].operands[0].type, PdfContentOperandType::Array);
}

void PdfContentStreamInterpreterTest::testInterpretTJInvalidElement() {
    PdfContentStreamInterpreter interp;
    auto arr = arrOp({nameOp("Bad")});
    QList<PdfContentOperator> ops = {
        makeOp("BT", {}),
        makeOp("TJ", {arr}),
        makeOp("ET", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::InvalidOperandType);
    QCOMPARE(r.operations.size(), 1);
    QCOMPARE(r.operations[0].type, PdfContentOperationType::BeginText);
}

void PdfContentStreamInterpreterTest::testInterpretUnknownOperator() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("XZ", {intOp(1)}),
        makeOp("BT", {}),
        makeOp("ET", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QVERIFY(r.hasUnknownOperators);
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::UnknownOperator);
    QCOMPARE(r.operations.size(), 3);
    QCOMPARE(r.operations[0].type, PdfContentOperationType::Unknown);
    QCOMPARE(r.operations[0].operatorName, QByteArray("XZ"));
    QCOMPARE(r.operations[0].operands.size(), 1);
    QCOMPARE(r.operations[1].type, PdfContentOperationType::BeginText);
    QCOMPARE(r.operations[2].type, PdfContentOperationType::EndText);
}

void PdfContentStreamInterpreterTest::testInterpretInvalidOperandCount() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("BT", {}),
        makeOp("Tf", {intOp(12)}),  // 1 operand, needs 2
        makeOp("ET", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::InvalidOperandCount);
    QCOMPARE(r.operations.size(), 1);
    QCOMPARE(r.operations[0].type, PdfContentOperationType::BeginText);
}

void PdfContentStreamInterpreterTest::testInterpretInvalidOperandType() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("BT", {}),
        makeOp("Tj", {intOp(42)}),  // integer where string expected
        makeOp("ET", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::InvalidOperandType);
    QCOMPARE(r.operations.size(), 1);
}

void PdfContentStreamInterpreterTest::testInterpretQUnderflow() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("Q", {})};
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::StateStackUnderflow);
    QCOMPARE(r.operations.size(), 0);
}

void PdfContentStreamInterpreterTest::testInterpretStackBalance() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("q", {}), makeOp("q", {}),
        makeOp("Q", {}), makeOp("Q", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::None);
    QCOMPARE(r.operations.size(), 4);
}

void PdfContentStreamInterpreterTest::testInterpretUnbalancedQAtEOF() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("q", {}), makeOp("q", {})};
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations.size(), 2);
}

void PdfContentStreamInterpreterTest::testInterpretOperandsPreserved() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("BT", {}),
        makeOp("Tf", {nameOp("F1"), intOp(12)}),
        makeOp("ET", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations[1].operands.size(), 2);
    QCOMPARE(r.operations[1].operands[0].type, PdfContentOperandType::Name);
    QCOMPARE(r.operations[1].operands[1].type, PdfContentOperandType::Integer);
}

void PdfContentStreamInterpreterTest::testInterpretColorGeneric() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("G", {realOp(0.5)})};
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations.size(), 1);
    QCOMPARE(r.operations[0].type, PdfContentOperationType::Generic);
    QCOMPARE(r.operations[0].operands.size(), 1);
}

void PdfContentStreamInterpreterTest::testInterpretTm() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("Tm", {intOp(1),intOp(0),intOp(0),intOp(1),intOp(72),intOp(720)})};
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations[0].type, PdfContentOperationType::SetTextMatrix);
    QCOMPARE(r.operations[0].numeric[0], 1.0);
    QCOMPARE(r.operations[0].numeric[4], 72.0);
    QCOMPARE(r.operations[0].numeric[5], 720.0);
}

void PdfContentStreamInterpreterTest::testInterpretTdAndTD() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("Td", {intOp(72), intOp(720)}),
        makeOp("TD", {intOp(10), intOp(20)}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations[0].type, PdfContentOperationType::MoveTextPosition);
    QCOMPARE(r.operations[0].numeric[0], 72.0);
    QCOMPARE(r.operations[0].numeric[1], 720.0);
    QCOMPARE(r.operations[1].type, PdfContentOperationType::MoveTextPositionSetLeading);
    QCOMPARE(r.operations[1].numeric[0], 10.0);
    QCOMPARE(r.operations[1].numeric[1], 20.0);
}

void PdfContentStreamInterpreterTest::testInterpretTStar() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("T*", {})};
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations[0].type, PdfContentOperationType::NextLine);
}

void PdfContentStreamInterpreterTest::testInterpretQuoteOperator() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("'", {strOp("Hello")})};
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations[0].type, PdfContentOperationType::NextLineShowText);
    QCOMPARE(r.operations[0].nameOrString, QByteArray("Hello"));
}

void PdfContentStreamInterpreterTest::testInterpretDoubleQuoteOperator() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("\"", {realOp(10.0), realOp(2.0), strOp("Hello")})};
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations[0].type, PdfContentOperationType::NextLineShowTextWithSpacing);
    QCOMPARE(r.operations[0].numeric[0], 10.0);
    QCOMPARE(r.operations[0].numeric[1], 2.0);
    QCOMPARE(r.operations[0].nameOrString, QByteArray("Hello"));
}

void PdfContentStreamInterpreterTest::testInterpretInvalidCountForZeroArgOp() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("q", {intOp(1)})};
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::InvalidOperandCount);
}

void PdfContentStreamInterpreterTest::testInterpretInvalidNumericType() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("m", {nameOp("X"), intOp(20)})};
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::InvalidOperandType);
}

void PdfContentStreamInterpreterTest::testInterpretTfInvalidFirstOperand() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("Tf", {intOp(12), intOp(10)})};
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::InvalidOperandType);
}

void PdfContentStreamInterpreterTest::testInterpretCmInvalidCount() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {makeOp("cm", {intOp(1),intOp(0),intOp(0),intOp(1),intOp(0)})};
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::InvalidOperandCount);
}

void PdfContentStreamInterpreterTest::testInterpretAllPathPainting() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("S", {}), makeOp("s", {}), makeOp("f", {}), makeOp("F", {}),
        makeOp("f*", {}),
        makeOp("B", {}), makeOp("B*", {}), makeOp("b", {}), makeOp("b*", {}),
        makeOp("n", {}),
    };
    auto r = interp.interpret(ops);
    QVERIFY(r.ok());
    QCOMPARE(r.operations.size(), 10);
    QCOMPARE(r.operations[0].type, PdfContentOperationType::Stroke);
    QCOMPARE(r.operations[1].type, PdfContentOperationType::CloseStroke);
    QCOMPARE(r.operations[2].type, PdfContentOperationType::Fill);
    QCOMPARE(r.operations[3].type, PdfContentOperationType::Fill);
    QCOMPARE(r.operations[4].type, PdfContentOperationType::FillEvenOdd);
    QCOMPARE(r.operations[5].type, PdfContentOperationType::FillStroke);
    QCOMPARE(r.operations[6].type, PdfContentOperationType::FillStrokeEvenOdd);
    QCOMPARE(r.operations[7].type, PdfContentOperationType::CloseFillStroke);
    QCOMPARE(r.operations[8].type, PdfContentOperationType::CloseFillStrokeEvenOdd);
    QCOMPARE(r.operations[9].type, PdfContentOperationType::EndPath);
}

void PdfContentStreamInterpreterTest::testInterpretUnknownBeforeFatal() {
    PdfContentStreamInterpreter interp;
    QList<PdfContentOperator> ops = {
        makeOp("XZ", {intOp(1)}),
        makeOp("Tf", {intOp(12)}),  // invalid count, fatal
    };
    auto r = interp.interpret(ops);
    QVERIFY(!r.ok());
    QCOMPARE(r.warning, PdfContentStreamInterpreterWarning::InvalidOperandCount);
    QVERIFY(r.hasUnknownOperators);
    QCOMPARE(r.operations.size(), 1);
    QCOMPARE(r.operations[0].type, PdfContentOperationType::Unknown);
}

QTEST_MAIN(PdfContentStreamInterpreterTest)
#include "PdfContentStreamInterpreterTest.moc"
