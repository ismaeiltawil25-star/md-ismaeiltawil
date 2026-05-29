#include <QtTest/QTest>
#include "PdfGraphicsStateTracker.h"

static PdfContentOperation makeOp(PdfContentOperationType t, const QByteArray &name,
    QList<PdfContentOperand> operands = {}, qsizetype off = 0) {
    PdfContentOperation op; op.type = t; op.operatorName = name; op.operands = operands; op.offset = off; return op;
}
static void setNum(PdfContentOperation &op, std::initializer_list<double> vals) {
    int i = 0; for (double v : vals) { if (i < 6) op.numeric[i++] = v; }
}
static void setStr(PdfContentOperation &op, const QByteArray &s) { op.nameOrString = s; }
static void setFS(PdfContentOperation &op, double sz) { op.fontSize = sz; }

static PdfContentOperand intOp(int64_t v) {
    PdfContentOperand op; op.type = PdfContentOperandType::Integer; op.intValue = v; return op;
}
static PdfContentOperand realOp(double v) {
    PdfContentOperand op; op.type = PdfContentOperandType::Real; op.realValue = v; return op;
}
static PdfContentOperand nameOp(QByteArray v) {
    PdfContentOperand op; op.type = PdfContentOperandType::Name; op.bytes = v; return op;
}
static PdfContentOperand strOp(QByteArray v) {
    PdfContentOperand op; op.type = PdfContentOperandType::String; op.bytes = v; return op;
}

class PdfGraphicsStateTrackerTest : public QObject {
    Q_OBJECT

private slots:
    void testExecuteEmpty();
    void testExecuteQPushPop();
    void testExecuteQUnderflow();
    void testExecuteCm();
    void testExecuteCmComposition();
    void testExecuteBTET();
    void testExecuteETWithoutBT();
    void testExecuteNestedBT();
    void testExecuteNestedBTDoesNotResetActiveTextState();
    void testExecuteTjOutsideBT();
    void testExecuteTdOutsideBT();
    void testExecuteFullTextBlock();
    void testExecuteTm();
    void testExecuteTStarUsesLeading();
    void testExecuteQuote();
    void testExecuteDoubleQuote();
    void testExecuteDoubleQuoteOutsideBTDoesNotSetSpacing();
    void testExecuteQRestoresFullStateIncludingFontAndTextMatrix();
    void testDiagnosticEntryHasContext();
    void testExecuteGenericCounted();
    void testExecuteUnknownCounted();
    void testExecutePathIgnored();
    void testExecuteQuoteAppliesNextLineAndShowsText();
};

void PdfGraphicsStateTrackerTest::testExecuteEmpty() {
    PdfGraphicsStateTracker tracker;
    auto r = tracker.execute({});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.state.ctm.a, 1.0);
    QCOMPARE(r.state.ctm.e, 0.0);
    QCOMPARE(r.state.ctm.f, 0.0);
    QVERIFY(!r.state.textObject);
    QCOMPARE(r.textShowCount, 0);
}

void PdfGraphicsStateTrackerTest::testExecuteQPushPop() {
    PdfGraphicsStateTracker tracker;
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::PushState, "q"),
        makeOp(PdfContentOperationType::PushState, "q"),
        makeOp(PdfContentOperationType::PopState, "Q"),
        makeOp(PdfContentOperationType::PopState, "Q"),
    };
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
}

void PdfGraphicsStateTrackerTest::testExecuteQUnderflow() {
    PdfGraphicsStateTracker tracker;
    auto r = tracker.execute({makeOp(PdfContentOperationType::PopState, "Q", {}, 50)});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfGraphicsStateDiagnostic::StateStackUnderflow);
    QCOMPARE(r.diagnostics[0].operatorName, QByteArray("Q"));
    QCOMPARE(r.diagnostics[0].offset, qsizetype(50));
    QVERIFY(r.state.ctm.nearlyEquals(PdfMatrix::identity()));
}

void PdfGraphicsStateTrackerTest::testExecuteCm() {
    PdfGraphicsStateTracker tracker;
    auto op = makeOp(PdfContentOperationType::ConcatMatrix, "cm");
    setNum(op, {2,0,0,2,10,20});
    auto r = tracker.execute({op});
    QVERIFY(!r.hasDiagnostics());
    QVERIFY(r.state.ctm.nearlyEquals(PdfMatrix::fromCm(2,0,0,2,10,20)));
}

void PdfGraphicsStateTrackerTest::testExecuteCmComposition() {
    PdfGraphicsStateTracker tracker;
    auto op1 = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(op1, {2,0,0,2,0,0});
    auto op2 = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(op2, {1,0,0,1,10,20});
    auto r = tracker.execute({op1, op2});
    QVERIFY(r.state.ctm.nearlyEquals(PdfMatrix::fromCm(2,0,0,2,10,20)));
}

void PdfGraphicsStateTrackerTest::testExecuteBTET() {
    PdfGraphicsStateTracker tracker;
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        makeOp(PdfContentOperationType::EndText, "ET"),
    };
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
    QVERIFY(!r.state.textObject);
}

void PdfGraphicsStateTrackerTest::testExecuteETWithoutBT() {
    PdfGraphicsStateTracker tracker;
    auto r = tracker.execute({makeOp(PdfContentOperationType::EndText, "ET", {}, 30)});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfGraphicsStateDiagnostic::ETWithoutBT);
    QVERIFY(!r.state.textObject);
}

void PdfGraphicsStateTrackerTest::testExecuteNestedBT() {
    PdfGraphicsStateTracker tracker;
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        makeOp(PdfContentOperationType::BeginText, "BT", {}, 20),
    };
    auto r = tracker.execute(ops);
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfGraphicsStateDiagnostic::NestedBT);
    QVERIFY(r.state.textObject);
}

void PdfGraphicsStateTrackerTest::testExecuteNestedBTDoesNotResetActiveTextState() {
    PdfGraphicsStateTracker tracker;
    auto td = makeOp(PdfContentOperationType::MoveTextPosition, "Td"); setNum(td, {72, 720});
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        td,
        makeOp(PdfContentOperationType::BeginText, "BT"),
    };
    auto r = tracker.execute(ops);
    QVERIFY(r.hasDiagnostics());
    QVERIFY(!r.state.textMatrix.nearlyEquals(PdfMatrix::identity()));
}

void PdfGraphicsStateTrackerTest::testExecuteTjOutsideBT() {
    PdfGraphicsStateTracker tracker;
    auto r = tracker.execute({makeOp(PdfContentOperationType::ShowText, "Tj", {}, 10)});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfGraphicsStateDiagnostic::TextShowingOutsideBT);
    QCOMPARE(r.textShowCount, 0);
}

void PdfGraphicsStateTrackerTest::testExecuteTdOutsideBT() {
    PdfGraphicsStateTracker tracker;
    auto op = makeOp(PdfContentOperationType::MoveTextPosition, "Td"); setNum(op, {100, 200});
    auto r = tracker.execute({op});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfGraphicsStateDiagnostic::TextPositionOutsideBT);
    QVERIFY(r.state.textMatrix.nearlyEquals(PdfMatrix::identity()));
}

void PdfGraphicsStateTrackerTest::testExecuteFullTextBlock() {
    PdfGraphicsStateTracker tracker;
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto td = makeOp(PdfContentOperationType::MoveTextPosition, "Td"); setNum(td, {72, 720});
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        tf,
        td,
        makeOp(PdfContentOperationType::ShowText, "Tj"),
        makeOp(PdfContentOperationType::EndText, "ET"),
    };
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.state.fontName, QByteArray("F1"));
    QCOMPARE(r.state.fontSize, 12.0);
    QCOMPARE(r.textShowCount, 1);
}

void PdfGraphicsStateTrackerTest::testExecuteTm() {
    PdfGraphicsStateTracker tracker;
    auto tm = makeOp(PdfContentOperationType::SetTextMatrix, "Tm"); setNum(tm, {1,0,0,1,72,720});
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        tm,
    };
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
    QVERIFY(r.state.textMatrix.nearlyEquals(PdfMatrix::fromCm(1,0,0,1,72,720)));
}

void PdfGraphicsStateTrackerTest::testExecuteTStarUsesLeading() {
    PdfGraphicsStateTracker tracker;
    auto tD = makeOp(PdfContentOperationType::MoveTextPositionSetLeading, "TD"); setNum(tD, {0, -15});
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        tD,
        makeOp(PdfContentOperationType::NextLine, "T*"),
    };
    auto r = tracker.execute(ops);
    QCOMPARE(r.state.leading, 15.0);
    QVERIFY(r.state.textMatrix.nearlyEquals(PdfMatrix::translation(0, -30)));
}

void PdfGraphicsStateTrackerTest::testExecuteQuote() {
    PdfGraphicsStateTracker tracker;
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        makeOp(PdfContentOperationType::NextLineShowText, "'"),
    };
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.textShowCount, 1);
}

void PdfGraphicsStateTrackerTest::testExecuteDoubleQuote() {
    PdfGraphicsStateTracker tracker;
    auto dq = makeOp(PdfContentOperationType::NextLineShowTextWithSpacing, "\""); setNum(dq, {10, 2});
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        dq,
    };
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.state.wordSpacing, 10.0);
    QCOMPARE(r.state.charSpacing, 2.0);
    QCOMPARE(r.textShowCount, 1);
}

void PdfGraphicsStateTrackerTest::testExecuteDoubleQuoteOutsideBTDoesNotSetSpacing() {
    PdfGraphicsStateTracker tracker;
    auto dq = makeOp(PdfContentOperationType::NextLineShowTextWithSpacing, "\""); setNum(dq, {10, 2});
    auto r = tracker.execute({dq});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfGraphicsStateDiagnostic::TextShowingOutsideBT);
    QCOMPARE(r.state.wordSpacing, 0.0);
    QCOMPARE(r.state.charSpacing, 0.0);
    QCOMPARE(r.textShowCount, 0);
}

void PdfGraphicsStateTrackerTest::testExecuteQRestoresFullStateIncludingFontAndTextMatrix() {
    PdfGraphicsStateTracker tracker;
    auto cm = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(cm, {2,0,0,2,0,0});
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto td = makeOp(PdfContentOperationType::MoveTextPosition, "Td"); setNum(td, {100, 200});
    QList<PdfContentOperation> ops = {
        cm,
        makeOp(PdfContentOperationType::PushState, "q"),
        makeOp(PdfContentOperationType::BeginText, "BT"),
        tf,
        td,
        makeOp(PdfContentOperationType::PopState, "Q"),
    };
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
    QVERIFY(r.state.ctm.nearlyEquals(PdfMatrix::fromCm(2,0,0,2,0,0)));
    QVERIFY(r.state.fontName.isEmpty());
    QCOMPARE(r.state.fontSize, 0.0);
    QVERIFY(r.state.textMatrix.nearlyEquals(PdfMatrix::identity()));
}

void PdfGraphicsStateTrackerTest::testDiagnosticEntryHasContext() {
    PdfGraphicsStateTracker tracker;
    auto r = tracker.execute({makeOp(PdfContentOperationType::EndText, "ET", {}, 42)});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfGraphicsStateDiagnostic::ETWithoutBT);
    QCOMPARE(r.diagnostics[0].operatorName, QByteArray("ET"));
    QCOMPARE(r.diagnostics[0].offset, qsizetype(42));
    QCOMPARE(r.diagnostics[0].operationIndex, 0);
}

void PdfGraphicsStateTrackerTest::testExecuteGenericCounted() {
    PdfGraphicsStateTracker tracker;
    auto r = tracker.execute({makeOp(PdfContentOperationType::Generic, "G")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.genericOpCount, 1);
    QVERIFY(r.state.ctm.nearlyEquals(PdfMatrix::identity()));
}

void PdfGraphicsStateTrackerTest::testExecuteUnknownCounted() {
    PdfGraphicsStateTracker tracker;
    auto r = tracker.execute({makeOp(PdfContentOperationType::Unknown, "XZ")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.unknownOpCount, 1);
    QVERIFY(r.state.ctm.nearlyEquals(PdfMatrix::identity()));
}

void PdfGraphicsStateTrackerTest::testExecutePathIgnored() {
    PdfGraphicsStateTracker tracker;
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0, 0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100, 100});
    QList<PdfContentOperation> ops = {m, l, makeOp(PdfContentOperationType::Stroke, "S")};
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.genericOpCount, 0);
    QCOMPARE(r.unknownOpCount, 0);
    QVERIFY(r.state.ctm.nearlyEquals(PdfMatrix::identity()));
}

void PdfGraphicsStateTrackerTest::testExecuteQuoteAppliesNextLineAndShowsText() {
    PdfGraphicsStateTracker tracker;
    auto tD = makeOp(PdfContentOperationType::MoveTextPositionSetLeading, "TD"); setNum(tD, {0, -20});
    QList<PdfContentOperation> ops = {
        makeOp(PdfContentOperationType::BeginText, "BT"),
        tD,
        makeOp(PdfContentOperationType::NextLineShowText, "'"),
    };
    auto r = tracker.execute(ops);
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.state.leading, 20.0);
    QCOMPARE(r.textShowCount, 1);
    QVERIFY(r.state.textMatrix.nearlyEquals(PdfMatrix::translation(0, -40)));
}

QTEST_MAIN(PdfGraphicsStateTrackerTest)
#include "PdfGraphicsStateTrackerTest.moc"
