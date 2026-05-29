#include <QtTest/QTest>
#include "PdfTextRunRecorder.h"

static PdfContentOperation makeOp(PdfContentOperationType t, const QByteArray &name,
    QList<PdfContentOperand> operands = {}, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperation op; op.type = t; op.operatorName = name; op.operands = operands;
    op.offset = off; op.length = len; return op;
}
static void setNum(PdfContentOperation &op, std::initializer_list<double> vals) {
    int i = 0; for (double v : vals) { if (i < 6) op.numeric[i++] = v; }
}
static void setStr(PdfContentOperation &op, const QByteArray &s) { op.nameOrString = s; }
static void setFS(PdfContentOperation &op, double sz) { op.fontSize = sz; }

static PdfContentOperand strOp(QByteArray v, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand o; o.type = PdfContentOperandType::String; o.bytes = v; o.offset = off; o.length = len; return o;
}
static PdfContentOperand intOp(int64_t v, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand o; o.type = PdfContentOperandType::Integer; o.intValue = v; o.offset = off; o.length = len; return o;
}
static PdfContentOperand nameOp(QByteArray v, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperand o; o.type = PdfContentOperandType::Name; o.bytes = v; o.offset = off; o.length = len; return o;
}
static PdfContentOperand arrOp(QList<PdfContentOperand> elems) {
    PdfContentOperand o; o.type = PdfContentOperandType::Array; o.arrayElements = elems; return o;
}

class PdfTextRunRecorderTest : public QObject { Q_OBJECT
private slots:
    void testRecordEmpty();
    void testRecordSimpleText();
    void testRecordTJ();
    void testRecordQuote();
    void testRecordDoubleQuote();
    void testRecordTStarUsesLeading();
    void testRecordTm();
    void testRecordCTMSnapshot();
    void testRecordQRestoresFullState();
    void testRecordTextShowOutsideBT();
    void testRecordTextPositionOutsideBT();
    void testRecordNestedBT();
    void testRecordETWithoutBT();
    void testRecordNoFontSelected();
    void testRecordNoFontSelectedNoMutate();
    void testRecordDblQuoteWithoutFont();
    void testRecordOutsideBTBeatsNoFont();
    void testRecordInvalidTJElement();
    void testRecordTJAllInvalid();
    void testRecordBTDoesNotResetFont();
    void testRecordRunSpacingSnapshot();
    void testRecordDoubleQuoteStoresSpacingOnRun();
    void testRecordQuoteStoresLeadingOnRun();
    void testRecordSourceOffsets();
    void testRecordDiagnosticEntryContext();
    void testRecordNonTextOpsIgnored();
};

void PdfTextRunRecorderTest::testRecordEmpty() {
    PdfTextRunRecorder r; auto res = r.record({});
    QVERIFY(res.runs.isEmpty()); QVERIFY(!res.hasDiagnostics());
    QVERIFY(res.finalState.ctm.nearlyEquals(PdfMatrix::identity()));
}

void PdfTextRunRecorderTest::testRecordSimpleText() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto td = makeOp(PdfContentOperationType::MoveTextPosition, "Td"); setNum(td, {72,720});
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "Hello");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,td,tj,
                            makeOp(PdfContentOperationType::EndText, "ET")});
    QVERIFY(!res.hasDiagnostics()); QCOMPARE(res.runs.size(), 1);
    QCOMPARE(res.runs[0].runType, PdfTextRunType::Tj);
    QCOMPARE(res.runs[0].textBytes, QByteArray("Hello"));
    QCOMPARE(res.runs[0].fontName, QByteArray("F1"));
    QCOMPARE(res.runs[0].fontSize, 12.0);
}

void PdfTextRunRecorderTest::testRecordTJ() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto arr = arrOp({strOp("H"), intOp(20), strOp("ello")});
    auto tjOp = makeOp(PdfContentOperationType::ShowTextWithOffsets, "TJ", {arr});
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,tjOp,
                            makeOp(PdfContentOperationType::EndText, "ET")});
    QVERIFY(!res.hasDiagnostics()); QCOMPARE(res.runs.size(), 1);
    QCOMPARE(res.runs[0].runType, PdfTextRunType::TJ);
    QCOMPARE(res.runs[0].tjElements.size(), 3);
    QVERIFY(res.runs[0].tjElements[0].isString);
    QCOMPARE(res.runs[0].tjElements[0].stringBytes, QByteArray("H"));
    QVERIFY(!res.runs[0].tjElements[1].isString);
    QCOMPARE(res.runs[0].tjElements[1].numericOffset, 20.0);
}

void PdfTextRunRecorderTest::testRecordQuote() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto td = makeOp(PdfContentOperationType::MoveTextPositionSetLeading, "TD"); setNum(td, {0,-15});
    auto q = makeOp(PdfContentOperationType::NextLineShowText, "'"); setStr(q, "Hi");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,td,q});
    QVERIFY(!res.hasDiagnostics()); QCOMPARE(res.runs.size(), 1);
    QCOMPARE(res.runs[0].runType, PdfTextRunType::Quote);
    QCOMPARE(res.runs[0].leading, 15.0);
}

void PdfTextRunRecorderTest::testRecordDoubleQuote() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto dq = makeOp(PdfContentOperationType::NextLineShowTextWithSpacing, "\""); setNum(dq, {10,2}); setStr(dq, "Hi");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,dq});
    QVERIFY(!res.hasDiagnostics()); QCOMPARE(res.runs.size(), 1);
    QCOMPARE(res.runs[0].runType, PdfTextRunType::DblQuote);
    QCOMPARE(res.runs[0].wordSpacing, 10.0);
    QCOMPARE(res.runs[0].charSpacing, 2.0);
}

void PdfTextRunRecorderTest::testRecordTStarUsesLeading() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto td = makeOp(PdfContentOperationType::MoveTextPositionSetLeading, "TD"); setNum(td, {0,-20});
    auto ts = makeOp(PdfContentOperationType::NextLine, "T*");
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,td,ts,tj});
    QVERIFY(!res.hasDiagnostics()); QCOMPARE(res.runs.size(), 1);
}

void PdfTextRunRecorderTest::testRecordTm() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto tm = makeOp(PdfContentOperationType::SetTextMatrix, "Tm"); setNum(tm, {1,0,0,1,72,720});
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,tm,tj});
    QVERIFY(res.runs[0].textMatrixSnapshot.nearlyEquals(PdfMatrix::fromCm(1,0,0,1,72,720)));
}

void PdfTextRunRecorderTest::testRecordCTMSnapshot() {
    auto cm = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(cm, {2,0,0,2,10,20});
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({cm, makeOp(PdfContentOperationType::BeginText, "BT"),tf,tj});
    QVERIFY(res.runs[0].ctmSnapshot.nearlyEquals(PdfMatrix::fromCm(2,0,0,2,10,20)));
}

void PdfTextRunRecorderTest::testRecordQRestoresFullState() {
    auto tfA = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tfA, "FontA"); setFS(tfA, 12);
    auto tfB = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tfB, "FontB"); setFS(tfB, 24);
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({tfA, makeOp(PdfContentOperationType::PushState, "q"),
                            tfB, makeOp(PdfContentOperationType::PopState, "Q"),
                            makeOp(PdfContentOperationType::BeginText, "BT"), tj});
    QCOMPARE(res.runs[0].fontName, QByteArray("FontA"));
    QCOMPARE(res.runs[0].fontSize, 12.0);
}

void PdfTextRunRecorderTest::testRecordTextShowOutsideBT() {
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj", {}, 10); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({tj});
    QCOMPARE(res.diagnostics.size(), 1);
    QCOMPARE(res.diagnostics[0].diagnostic, PdfTextRunDiagnostic::TextShowOutsideBT);
    QCOMPARE(res.runs.size(), 0);
}

void PdfTextRunRecorderTest::testRecordTextPositionOutsideBT() {
    auto td = makeOp(PdfContentOperationType::MoveTextPosition, "Td"); setNum(td, {100,200});
    PdfTextRunRecorder rec;
    auto res = rec.record({td});
    QCOMPARE(res.diagnostics.size(), 1);
    QCOMPARE(res.diagnostics[0].diagnostic, PdfTextRunDiagnostic::TextPositionOutsideBT);
    QVERIFY(res.finalState.textMatrix.nearlyEquals(PdfMatrix::identity()));
}

void PdfTextRunRecorderTest::testRecordNestedBT() {
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),
                            makeOp(PdfContentOperationType::BeginText, "BT", {}, 20)});
    QCOMPARE(res.diagnostics.size(), 1);
    QCOMPARE(res.diagnostics[0].diagnostic, PdfTextRunDiagnostic::NestedBT);
    QVERIFY(res.finalState.textObject);
}

void PdfTextRunRecorderTest::testRecordETWithoutBT() {
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::EndText, "ET", {}, 30)});
    QCOMPARE(res.diagnostics.size(), 1);
    QCOMPARE(res.diagnostics[0].diagnostic, PdfTextRunDiagnostic::ETWithoutBT);
}

void PdfTextRunRecorderTest::testRecordNoFontSelected() {
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"), tj});
    QCOMPARE(res.diagnostics.size(), 1);
    QCOMPARE(res.diagnostics[0].diagnostic, PdfTextRunDiagnostic::NoFontSelected);
    QCOMPARE(res.runs.size(), 0);
}

void PdfTextRunRecorderTest::testRecordNoFontSelectedNoMutate() {
    PdfTextRunRecorder rec;
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"), tj});
    QVERIFY(res.finalState.textMatrix.nearlyEquals(PdfMatrix::identity()));
    QVERIFY(res.finalState.ctm.nearlyEquals(PdfMatrix::identity()));
}

void PdfTextRunRecorderTest::testRecordDblQuoteWithoutFont() {
    auto dq = makeOp(PdfContentOperationType::NextLineShowTextWithSpacing, "\""); setNum(dq, {10,2}); setStr(dq, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"), dq});
    QCOMPARE(res.diagnostics.size(), 1);
    QCOMPARE(res.diagnostics[0].diagnostic, PdfTextRunDiagnostic::NoFontSelected);
    QCOMPARE(res.finalState.wordSpacing, 0.0);
    QCOMPARE(res.finalState.charSpacing, 0.0);
}

void PdfTextRunRecorderTest::testRecordOutsideBTBeatsNoFont() {
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({tj});
    QCOMPARE(res.diagnostics.size(), 1);
    QCOMPARE(res.diagnostics[0].diagnostic, PdfTextRunDiagnostic::TextShowOutsideBT);
}

void PdfTextRunRecorderTest::testRecordInvalidTJElement() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto arr = arrOp({nameOp("Bad"), strOp("A"), intOp(10)});
    auto tjOp = makeOp(PdfContentOperationType::ShowTextWithOffsets, "TJ", {arr});
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,tjOp});
    QCOMPARE(res.diagnostics.size(), 1);
    QCOMPARE(res.diagnostics[0].diagnostic, PdfTextRunDiagnostic::InvalidTJElement);
    QCOMPARE(res.runs.size(), 1);
    QCOMPARE(res.runs[0].tjElements.size(), 2);
}

void PdfTextRunRecorderTest::testRecordTJAllInvalid() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto arr = arrOp({nameOp("Bad1"), nameOp("Bad2")});
    auto tjOp = makeOp(PdfContentOperationType::ShowTextWithOffsets, "TJ", {arr});
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,tjOp});
    QCOMPARE(res.diagnostics.size(), 2);
    QCOMPARE(res.runs.size(), 0);
}

void PdfTextRunRecorderTest::testRecordBTDoesNotResetFont() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({tf, makeOp(PdfContentOperationType::BeginText, "BT"), tj});
    QVERIFY(!res.hasDiagnostics());
    QCOMPARE(res.runs[0].fontName, QByteArray("F1"));
}

void PdfTextRunRecorderTest::testRecordRunSpacingSnapshot() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto dq = makeOp(PdfContentOperationType::NextLineShowTextWithSpacing, "\""); setNum(dq, {10,2}); setStr(dq, "A");
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "B");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,dq,tj});
    QCOMPARE(res.runs.size(), 2);
    QCOMPARE(res.runs[0].wordSpacing, 10.0);
    QCOMPARE(res.runs[1].wordSpacing, 10.0);
}

void PdfTextRunRecorderTest::testRecordDoubleQuoteStoresSpacingOnRun() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto dq = makeOp(PdfContentOperationType::NextLineShowTextWithSpacing, "\""); setNum(dq, {10,2}); setStr(dq, "A");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,dq});
    QCOMPARE(res.runs[0].wordSpacing, 10.0);
    QCOMPARE(res.runs[0].charSpacing, 2.0);
}

void PdfTextRunRecorderTest::testRecordQuoteStoresLeadingOnRun() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto td = makeOp(PdfContentOperationType::MoveTextPositionSetLeading, "TD"); setNum(td, {0,-20});
    auto q = makeOp(PdfContentOperationType::NextLineShowText, "'"); setStr(q, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,td,q});
    QCOMPARE(res.runs[0].leading, 20.0);
}

void PdfTextRunRecorderTest::testRecordSourceOffsets() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj", {}, 50, 5); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::BeginText, "BT"),tf,tj});
    QCOMPARE(res.runs[0].offset, qsizetype(50));
    QCOMPARE(res.runs[0].length, qsizetype(5));
}

void PdfTextRunRecorderTest::testRecordDiagnosticEntryContext() {
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::EndText, "ET", {}, 42)});
    QCOMPARE(res.diagnostics[0].operatorName, QByteArray("ET"));
    QCOMPARE(res.diagnostics[0].offset, qsizetype(42));
    QCOMPARE(res.diagnostics[0].operationIndex, 0);
}

void PdfTextRunRecorderTest::testRecordNonTextOpsIgnored() {
    auto tf = makeOp(PdfContentOperationType::SetFont, "Tf"); setStr(tf, "F1"); setFS(tf, 12);
    auto tj = makeOp(PdfContentOperationType::ShowText, "Tj"); setStr(tj, "X");
    PdfTextRunRecorder rec;
    auto res = rec.record({makeOp(PdfContentOperationType::Stroke, "S"),
                            makeOp(PdfContentOperationType::MoveTo, "m"),
                            makeOp(PdfContentOperationType::BeginText, "BT"),tf,tj});
    QVERIFY(!res.hasDiagnostics());
    QCOMPARE(res.runs.size(), 1);
}

QTEST_MAIN(PdfTextRunRecorderTest)
#include "PdfTextRunRecorderTest.moc"
