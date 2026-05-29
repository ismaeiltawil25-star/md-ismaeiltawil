#include <QtTest/QTest>
#include "PdfPathRecorder.h"

static PdfContentOperation makeOp(PdfContentOperationType t, const QByteArray &name,
    QList<PdfContentOperand> operands = {}, qsizetype off = 0, qsizetype len = 1) {
    PdfContentOperation op; op.type = t; op.operatorName = name; op.operands = operands;
    op.offset = off; op.length = len; return op;
}
static void setNum(PdfContentOperation &op, std::initializer_list<double> vals) {
    int i = 0; for (double v : vals) { if (i < 6) op.numeric[i++] = v; }
}

class PdfPathRecorderTest : public QObject {
    Q_OBJECT

private slots:
    void testRecordEmpty();
    void testRecordSimpleLine();
    void testRecordRectangle();
    void testRecordClosePath();
    void testRecordCubicBezier();
    void testRecordMultiplePaths();
    void testRecordEndPathN();
    void testRecordPaintWithoutPath();
    void testRecordSegmentWithoutMove();
    void testRecordCurveWithoutMove();
    void testRecordCloseWithoutSubpath();
    void testRecordImplicitCloseS();
    void testRecordImplicitCloseB();
    void testRecordImplicitCloseOnClosedPath();
    void testRecordPaintTimeCTM();
    void testRecordSegmentCTM();
    void testRecordQRestoresCTMForSegments();
    void testRecordQWithoutPushIgnored();
    void testRecordAllPaintModes();
    void testRecordSourceOffsets();
    void testRecordNonPathOpsIgnored();
    void testRecordEmptyN();
    void testRecordImplicitCloseCTM();
};

void PdfPathRecorderTest::testRecordEmpty() {
    PdfPathRecorder recorder;
    auto r = recorder.record({});
    QVERIFY(r.records.isEmpty());
    QVERIFY(!r.hasDiagnostics());
}

void PdfPathRecorderTest::testRecordSimpleLine() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100,100});
    PdfPathRecorder recorder;
    auto r = recorder.record({m, l, makeOp(PdfContentOperationType::Stroke, "S")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 1);
    QCOMPARE(r.records[0].paintMode, PdfPaintMode::Stroke);
    QCOMPARE(r.records[0].segments.size(), 2);
    QCOMPARE(r.records[0].segments[0].type, PdfPathSegmentType::MoveTo);
    QCOMPARE(r.records[0].segments[1].type, PdfPathSegmentType::LineTo);
    QCOMPARE(r.records[0].segments[1].x, 100.0);
    QCOMPARE(r.records[0].segments[1].y, 100.0);
}

void PdfPathRecorderTest::testRecordRectangle() {
    auto re = makeOp(PdfContentOperationType::Rectangle, "re"); setNum(re, {0,0,100,100});
    PdfPathRecorder recorder;
    auto r = recorder.record({re, makeOp(PdfContentOperationType::Fill, "f")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 1);
    QCOMPARE(r.records[0].paintMode, PdfPaintMode::Fill);
    QCOMPARE(r.records[0].segments.size(), 1);
    QCOMPARE(r.records[0].segments[0].type, PdfPathSegmentType::Rectangle);
    QCOMPARE(r.records[0].segments[0].w, 100.0);
}

void PdfPathRecorderTest::testRecordClosePath() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100,0});
    auto h = makeOp(PdfContentOperationType::ClosePath, "h");
    PdfPathRecorder recorder;
    auto r = recorder.record({m, l, h, makeOp(PdfContentOperationType::Stroke, "S")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records[0].segments.size(), 3);
    QCOMPARE(r.records[0].segments[2].type, PdfPathSegmentType::CloseSubpath);
}

void PdfPathRecorderTest::testRecordCubicBezier() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto c = makeOp(PdfContentOperationType::CurveTo, "c"); setNum(c, {10,20,30,40,50,60});
    PdfPathRecorder recorder;
    auto r = recorder.record({m, c, makeOp(PdfContentOperationType::Stroke, "S")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records[0].segments[1].type, PdfPathSegmentType::CubicBezier);
    QCOMPARE(r.records[0].segments[1].x1, 10.0);
    QCOMPARE(r.records[0].segments[1].y1, 20.0);
    QCOMPARE(r.records[0].segments[1].x3, 50.0);
    QCOMPARE(r.records[0].segments[1].y3, 60.0);
}

void PdfPathRecorderTest::testRecordMultiplePaths() {
    auto m1 = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m1, {0,0});
    auto l1 = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l1, {100,0});
    auto m2 = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m2, {50,50});
    auto re = makeOp(PdfContentOperationType::Rectangle, "re"); setNum(re, {0,0,10,10});
    PdfPathRecorder recorder;
    auto r = recorder.record({m1, l1, makeOp(PdfContentOperationType::Stroke, "S"),
                               m2, re, makeOp(PdfContentOperationType::Fill, "f")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 2);
    QCOMPARE(r.records[0].paintMode, PdfPaintMode::Stroke);
    QCOMPARE(r.records[1].paintMode, PdfPaintMode::Fill);
}

void PdfPathRecorderTest::testRecordEndPathN() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    PdfPathRecorder recorder;
    auto r = recorder.record({m, makeOp(PdfContentOperationType::EndPath, "n")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 0);
}

void PdfPathRecorderTest::testRecordPaintWithoutPath() {
    PdfPathRecorder recorder;
    auto r = recorder.record({makeOp(PdfContentOperationType::Stroke, "S", {}, 10)});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfPathDiagnostic::PaintWithoutPath);
    QCOMPARE(r.records.size(), 0);
}

void PdfPathRecorderTest::testRecordSegmentWithoutMove() {
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100,100});
    PdfPathRecorder recorder;
    auto r = recorder.record({l, makeOp(PdfContentOperationType::Stroke, "S")});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfPathDiagnostic::SegmentWithoutMove);
    QCOMPARE(r.records.size(), 1);
    QCOMPARE(r.records[0].segments.size(), 1);
    QCOMPARE(r.records[0].segments[0].type, PdfPathSegmentType::LineTo);
}

void PdfPathRecorderTest::testRecordCurveWithoutMove() {
    auto c = makeOp(PdfContentOperationType::CurveTo, "c"); setNum(c, {10,20,30,40,50,60});
    PdfPathRecorder recorder;
    auto r = recorder.record({c, makeOp(PdfContentOperationType::Stroke, "S")});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfPathDiagnostic::SegmentWithoutMove);
    QCOMPARE(r.records.size(), 1);
    QCOMPARE(r.records[0].segments[0].type, PdfPathSegmentType::CubicBezier);
}

void PdfPathRecorderTest::testRecordCloseWithoutSubpath() {
    PdfPathRecorder recorder;
    auto r = recorder.record({makeOp(PdfContentOperationType::ClosePath, "h", {}, 5)});
    QCOMPARE(r.diagnostics.size(), 1);
    QCOMPARE(r.diagnostics[0].diagnostic, PdfPathDiagnostic::CloseWithoutSubpath);
    QCOMPARE(r.records.size(), 0);
}

void PdfPathRecorderTest::testRecordImplicitCloseS() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100,0});
    auto s = makeOp(PdfContentOperationType::CloseStroke, "s", {}, 45);
    PdfPathRecorder recorder;
    auto r = recorder.record({m, l, s});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 1);
    QCOMPARE(r.records[0].paintMode, PdfPaintMode::CloseStroke);
    QCOMPARE(r.records[0].segments.size(), 3);
    QCOMPARE(r.records[0].segments[2].type, PdfPathSegmentType::CloseSubpath);
    QCOMPARE(r.records[0].segments[2].offset, qsizetype(45));
}

void PdfPathRecorderTest::testRecordImplicitCloseB() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100,0});
    PdfPathRecorder recorder;
    auto r = recorder.record({m, l, makeOp(PdfContentOperationType::CloseFillStroke, "b")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 1);
    QCOMPARE(r.records[0].paintMode, PdfPaintMode::CloseFillStroke);
    QCOMPARE(r.records[0].segments.size(), 3);
}

void PdfPathRecorderTest::testRecordImplicitCloseOnClosedPath() {
    auto re = makeOp(PdfContentOperationType::Rectangle, "re"); setNum(re, {0,0,10,10});
    PdfPathRecorder recorder;
    auto r = recorder.record({re, makeOp(PdfContentOperationType::CloseStroke, "s")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 1);
    QCOMPARE(r.records[0].segments.size(), 1);
    QCOMPARE(r.records[0].segments[0].type, PdfPathSegmentType::Rectangle);
}

void PdfPathRecorderTest::testRecordPaintTimeCTM() {
    auto cm = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(cm, {2,0,0,2,10,20});
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100,0});
    PdfPathRecorder recorder;
    auto r = recorder.record({cm, m, l, makeOp(PdfContentOperationType::Stroke, "S")});
    QVERIFY(r.records[0].ctmSnapshot.nearlyEquals(PdfMatrix::fromCm(2,0,0,2,10,20)));
}

void PdfPathRecorderTest::testRecordSegmentCTM() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto cm = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(cm, {2,0,0,2,0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100,0});
    PdfPathRecorder recorder;
    auto r = recorder.record({m, cm, l, makeOp(PdfContentOperationType::Stroke, "S")});
    QVERIFY(r.records[0].segments[0].ctmSnapshot.nearlyEquals(PdfMatrix::identity()));
    QVERIFY(r.records[0].segments[1].ctmSnapshot.nearlyEquals(PdfMatrix::fromCm(2,0,0,2,0,0)));
}

void PdfPathRecorderTest::testRecordQRestoresCTMForSegments() {
    auto cmA = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(cmA, {2,0,0,2,0,0});
    auto cmB = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(cmB, {3,0,0,3,0,0});
    auto m2 = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m2, {0,0});
    auto l1 = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l1, {100,0});
    PdfPathRecorder recorder;
    // q, cmA, m2, Q, cmB, l1, S
    auto r = recorder.record({makeOp(PdfContentOperationType::PushState, "q"),
                               cmA, m2, makeOp(PdfContentOperationType::PopState, "Q"),
                               cmB, l1, makeOp(PdfContentOperationType::Stroke, "S")});
    QVERIFY(!r.hasDiagnostics());
    QVERIFY(r.records[0].segments[0].ctmSnapshot.nearlyEquals(PdfMatrix::fromCm(2,0,0,2,0,0)));
    QVERIFY(r.records[0].segments[1].ctmSnapshot.nearlyEquals(PdfMatrix::fromCm(3,0,0,3,0,0)));
}

void PdfPathRecorderTest::testRecordQWithoutPushIgnored() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    PdfPathRecorder recorder;
    auto r = recorder.record({makeOp(PdfContentOperationType::PopState, "Q"), m,
                               makeOp(PdfContentOperationType::Stroke, "S")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 1);
    QVERIFY(r.finalCtm.nearlyEquals(PdfMatrix::identity()));
}

void PdfPathRecorderTest::testRecordAllPaintModes() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {10,0});
    auto re = makeOp(PdfContentOperationType::Rectangle, "re"); setNum(re, {0,0,10,10});
    PdfPathRecorder recorder;
    auto r = recorder.record({
        m, l, makeOp(PdfContentOperationType::Stroke, "S"),
        m, l, makeOp(PdfContentOperationType::CloseStroke, "s"),
        re, makeOp(PdfContentOperationType::Fill, "f"),
        re, makeOp(PdfContentOperationType::Fill, "F"),
        re, makeOp(PdfContentOperationType::FillEvenOdd, "f*"),
        re, makeOp(PdfContentOperationType::FillStroke, "B"),
        re, makeOp(PdfContentOperationType::FillStrokeEvenOdd, "B*"),
        m, l, makeOp(PdfContentOperationType::CloseFillStroke, "b"),
        m, l, makeOp(PdfContentOperationType::CloseFillStrokeEvenOdd, "b*"),
    });
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 9);
    QCOMPARE(r.records[0].paintMode, PdfPaintMode::Stroke);
    QCOMPARE(r.records[1].paintMode, PdfPaintMode::CloseStroke);
    QCOMPARE(r.records[2].paintMode, PdfPaintMode::Fill);
    QCOMPARE(r.records[3].paintMode, PdfPaintMode::Fill);
    QCOMPARE(r.records[4].paintMode, PdfPaintMode::FillEvenOdd);
    QCOMPARE(r.records[5].paintMode, PdfPaintMode::FillStroke);
    QCOMPARE(r.records[6].paintMode, PdfPaintMode::FillStrokeEvenOdd);
    QCOMPARE(r.records[7].paintMode, PdfPaintMode::CloseFillStroke);
    QCOMPARE(r.records[8].paintMode, PdfPaintMode::CloseFillStrokeEvenOdd);
}

void PdfPathRecorderTest::testRecordSourceOffsets() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m", {}, 10, 2); setNum(m, {0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l", {}, 20, 3); setNum(l, {100,0});
    auto s = makeOp(PdfContentOperationType::Stroke, "S", {}, 30, 2);
    PdfPathRecorder recorder;
    auto r = recorder.record({m, l, s});
    QCOMPARE(r.records[0].segments[0].offset, qsizetype(10));
    QCOMPARE(r.records[0].segments[1].offset, qsizetype(20));
    QCOMPARE(r.records[0].paintOffset, qsizetype(30));
}

void PdfPathRecorderTest::testRecordNonPathOpsIgnored() {
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    PdfPathRecorder recorder;
    auto r = recorder.record({makeOp(PdfContentOperationType::BeginText, "BT"),
                               m, makeOp(PdfContentOperationType::Stroke, "S"),
                               makeOp(PdfContentOperationType::EndText, "ET")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 1);
}

void PdfPathRecorderTest::testRecordEmptyN() {
    PdfPathRecorder recorder;
    auto r = recorder.record({makeOp(PdfContentOperationType::EndPath, "n")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records.size(), 0);
}

void PdfPathRecorderTest::testRecordImplicitCloseCTM() {
    auto cm = makeOp(PdfContentOperationType::ConcatMatrix, "cm"); setNum(cm, {3,0,0,3,0,0});
    auto m = makeOp(PdfContentOperationType::MoveTo, "m"); setNum(m, {0,0});
    auto l = makeOp(PdfContentOperationType::LineTo, "l"); setNum(l, {100,0});
    PdfPathRecorder recorder;
    auto r = recorder.record({m, cm, l, makeOp(PdfContentOperationType::CloseStroke, "s")});
    QVERIFY(!r.hasDiagnostics());
    QCOMPARE(r.records[0].segments.size(), 3);
    QVERIFY(r.records[0].segments[2].ctmSnapshot.nearlyEquals(PdfMatrix::fromCm(3,0,0,3,0,0)));
}

QTEST_MAIN(PdfPathRecorderTest)
#include "PdfPathRecorderTest.moc"
