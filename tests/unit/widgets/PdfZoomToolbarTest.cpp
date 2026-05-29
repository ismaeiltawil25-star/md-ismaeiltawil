#include <QtTest/QTest>
#include <QSignalSpy>
#include "PdfZoomToolbar.h"

class PdfZoomToolbarTest : public QObject {
    Q_OBJECT
private slots:
    void testInitialResetButtonText();
    void testZoomInRequestedEmitted();
    void testZoomOutRequestedEmitted();
    void testZoomResetRequestedEmitted();
    void testSetZoomUpdatesButtonText();
    void testDisabledDisablesAllButtons();
    void testSetZoomSmallValue();
};

void PdfZoomToolbarTest::testInitialResetButtonText() {
    PdfZoomToolbar toolbar;
    auto *resetBtn = toolbar.findChild<QPushButton *>("pdfZoomResetButton");
    QVERIFY(resetBtn != nullptr);
    QCOMPARE(resetBtn->text(), QString("100 %"));
}

void PdfZoomToolbarTest::testZoomInRequestedEmitted() {
    PdfZoomToolbar toolbar;
    auto *btn = toolbar.findChild<QPushButton *>("pdfZoomInButton");
    QVERIFY(btn != nullptr);
    QSignalSpy spy(&toolbar, &PdfZoomToolbar::zoomInRequested);
    btn->click();
    QCOMPARE(spy.count(), 1);
}

void PdfZoomToolbarTest::testZoomOutRequestedEmitted() {
    PdfZoomToolbar toolbar;
    auto *btn = toolbar.findChild<QPushButton *>("pdfZoomOutButton");
    QVERIFY(btn != nullptr);
    QSignalSpy spy(&toolbar, &PdfZoomToolbar::zoomOutRequested);
    btn->click();
    QCOMPARE(spy.count(), 1);
}

void PdfZoomToolbarTest::testZoomResetRequestedEmitted() {
    PdfZoomToolbar toolbar;
    auto *btn = toolbar.findChild<QPushButton *>("pdfZoomResetButton");
    QVERIFY(btn != nullptr);
    QSignalSpy spy(&toolbar, &PdfZoomToolbar::zoomResetRequested);
    btn->click();
    QCOMPARE(spy.count(), 1);
}

void PdfZoomToolbarTest::testSetZoomUpdatesButtonText() {
    PdfZoomToolbar toolbar;
    toolbar.setZoom(1.5);
    auto *resetBtn = toolbar.findChild<QPushButton *>("pdfZoomResetButton");
    QVERIFY(resetBtn != nullptr);
    QCOMPARE(resetBtn->text(), QString("150 %"));
}

void PdfZoomToolbarTest::testDisabledDisablesAllButtons() {
    PdfZoomToolbar toolbar;
    toolbar.setEnabled(false);
    auto *inBtn  = toolbar.findChild<QPushButton *>("pdfZoomInButton");
    auto *outBtn = toolbar.findChild<QPushButton *>("pdfZoomOutButton");
    auto *rstBtn = toolbar.findChild<QPushButton *>("pdfZoomResetButton");
    QVERIFY(inBtn  && !inBtn->isEnabled());
    QVERIFY(outBtn && !outBtn->isEnabled());
    QVERIFY(rstBtn && !rstBtn->isEnabled());
}

void PdfZoomToolbarTest::testSetZoomSmallValue() {
    PdfZoomToolbar toolbar;
    toolbar.setZoom(0.25);
    auto *resetBtn = toolbar.findChild<QPushButton *>("pdfZoomResetButton");
    QVERIFY(resetBtn != nullptr);
    QCOMPARE(resetBtn->text(), QString("25 %"));
}

QTEST_MAIN(PdfZoomToolbarTest)
#include "PdfZoomToolbarTest.moc"
