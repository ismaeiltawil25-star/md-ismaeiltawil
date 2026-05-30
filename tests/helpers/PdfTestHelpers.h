#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QTemporaryDir>
#include <QFile>

namespace PdfTestHelpers {

inline QByteArray buildMinimalPdfBytes(const QByteArray &content = {},
                                       const QByteArray &pageExtra = " /MediaBox [0 0 612 792]") {
    QByteArray pdf;
    pdf += "%PDF-1.7\n1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R" + pageExtra + " >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + QByteArray::number(content.size())
         + " >>\nstream\n" + content + "\nendstream\nendobj\n";
    qsizetype xo = pdf.size(); pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto c = [&](int n) { auto p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8() + " 00000 n\n"; };
    pdf += c(1) + c(2) + c(3) + c(4);
    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    return pdf;
}

inline int countNonBgPixels(const QImage &img) {
    int n = 0; QRgb bg = Qt::white;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixel(x, y) != bg) ++n;
    return n;
}

inline QString writeBytesFile(QTemporaryDir &dir,
                              const QString &fileName,
                              const QByteArray &bytes) {
    QString path = dir.filePath(fileName);
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(bytes);
    f.close();
    return path;
}

inline QString writePdfFile(QTemporaryDir &dir,
                            const QString &fileName,
                            const QByteArray &pdfBytes) {
    return writeBytesFile(dir, fileName, pdfBytes);
}

} // namespace PdfTestHelpers
