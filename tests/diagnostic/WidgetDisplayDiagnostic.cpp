#include <QtTest/QTest>
#include <QApplication>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QFile>
#include "app/MainWindow.h"
#include "widgets/PdfSinglePagePreviewWidget.h"

static int countNonBg(const QImage &img) {
    int n = 0; QRgb bg = Qt::white;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixel(x, y) != bg) ++n;
    return n;
}

static QByteArray buildPdf(const QByteArray &content,
    QByteArray pageExtra = " /MediaBox [0 0 612 792]") {
    QByteArray pdf;
    pdf += "%PDF-1.7\n1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R" + pageExtra + " >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + QByteArray::number(content.size())
        + " >>\nstream\n" + content + "\nendstream\nendobj\n";
    qsizetype xo = pdf.size(); pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto c = [&](int n){auto p=pdf.indexOf(QByteArray::number(n)+" 0 obj");
        return QString("%1").arg(p,10,10,QLatin1Char('0')).toUtf8()+" 00000 n\n";};
    pdf+=c(1)+c(2)+c(3)+c(4);
    pdf+="trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n"+QByteArray::number(xo)+"\n%%EOF\n";
    return pdf;
}

static QString createPdfFile(const QByteArray &content) {
    static QTemporaryDir dir;
    QString path = dir.filePath("test.pdf");
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(content);
    f.close();
    return path;
}

class WidgetDisplayDiagnostic : public QObject {
    Q_OBJECT
private slots:
    void testWidgetImageStoredCorrectly();
    void testWidgetSizeAfterSetImage();
    void testWidgetRenderOutputHasPixels();
};

void WidgetDisplayDiagnostic::testWidgetImageStoredCorrectly() {
    QString path = createPdfFile(buildPdf("BT /F1 12 Tf (Hello) Tj ET"));
    MainWindow w;
    w.show();
    w.openDocumentsFromPaths({path});

    auto *tabs = w.findChild<QTabWidget *>();
    QVERIFY(tabs != nullptr);
    QVERIFY2(tabs->count() >= 1, "A PDF tab should have been created");
    if (tabs->count() == 0) return;

    auto *sa = qobject_cast<QScrollArea *>(tabs->widget(0));
    QVERIFY(sa != nullptr);
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(sa->widget());
    QVERIFY(pw != nullptr);

    qDebug() << "=== Image stored in widget ===";
    qDebug() << "image.isNull:" << pw->image().isNull();
    qDebug() << "image size:" << pw->image().width() << "x" << pw->image().height();
    qDebug() << "countNonBg(image):" << countNonBg(pw->image());

    QVERIFY(!pw->image().isNull());
    QVERIFY(countNonBg(pw->image()) > 0);
}

void WidgetDisplayDiagnostic::testWidgetSizeAfterSetImage() {
    QString path = createPdfFile(buildPdf("BT /F1 12 Tf (Hello) Tj ET"));
    MainWindow w;
    w.show();
    w.openDocumentsFromPaths({path});

    auto *tabs = w.findChild<QTabWidget *>();
    QVERIFY(tabs != nullptr);
    QVERIFY2(tabs->count() >= 1, "A PDF tab should have been created");
    if (tabs->count() == 0) return;

    auto *sa = qobject_cast<QScrollArea *>(tabs->widget(0));
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(sa->widget());
    QVERIFY(!pw->image().isNull());

    qDebug() << "=== Size diagnostics ===";
    qDebug() << "sizeHint:" << pw->sizeHint();
    qDebug() << "actualSize:" << pw->size();
    qDebug() << "viewportSize:" << sa->viewport()->size();
    qDebug() << "widgetResizable:" << sa->widgetResizable();

    QVERIFY(pw->width() > 0);
    QVERIFY(pw->height() > 0);
}

void WidgetDisplayDiagnostic::testWidgetRenderOutputHasPixels() {
    QString path = createPdfFile(buildPdf("BT /F1 12 Tf (Hello) Tj ET"));
    MainWindow w;
    w.show();
    w.openDocumentsFromPaths({path});

    auto *tabs = w.findChild<QTabWidget *>();
    QVERIFY(tabs != nullptr);
    QVERIFY2(tabs->count() >= 1, "A PDF tab should have been created");
    if (tabs->count() == 0) return;

    auto *sa = qobject_cast<QScrollArea *>(tabs->widget(0));
    auto *pw = qobject_cast<PdfSinglePagePreviewWidget *>(sa->widget());

    qDebug() << "=== Widget dimensions ===";
    qDebug() << "widget->width():" << pw->width();
    qDebug() << "widget->height():" << pw->height();
    qDebug() << "widget->sizeHint():" << pw->sizeHint();
    qDebug() << "widget->minimumSizeHint():" << pw->minimumSizeHint();
    qDebug() << "scrollArea viewport:" << sa->viewport()->size();
    qDebug() << "widgetResizable:" << sa->widgetResizable();
    qDebug() << "image (stored):" << pw->image().width() << "x" << pw->image().height();

    // Render widget to offscreen image at its sizeHint
    QSize renderSize = pw->sizeHint().expandedTo(QSize(612, 792));
    pw->resize(renderSize);
    QApplication::processEvents();

    qDebug() << "=== After resize to sizeHint ===";
    qDebug() << "widget->width():" << pw->width();
    qDebug() << "widget->height():" << pw->height();

    QImage rendered(renderSize, QImage::Format_ARGB32);
    rendered.fill(Qt::white);
    pw->render(&rendered);

    int nonBg = countNonBg(rendered);
    qDebug() << "countNonBg(rendered widget):" << nonBg;

    QVERIFY(nonBg > 0);
}

QTEST_MAIN(WidgetDisplayDiagnostic)
#include "WidgetDisplayDiagnostic.moc"
