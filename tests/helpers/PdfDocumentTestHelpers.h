#pragma once

#include "PdfDocumentLoader.h"
#include "PdfZlibTestHelpers.h"

namespace PdfTestHelpers {

inline PdfDocument loadMinimalPdfDocument(const QByteArray &content,
                                          bool flateEncode,
                                          const QByteArray &pageExtra,
                                          const QByteArray &parentExtra = {}) {
    QByteArray body = flateEncode ? zlibCompress(content) : content;
    QByteArray filter;
    if (flateEncode) filter = " /Filter /FlateDecode";
    QByteArray pdf;
    pdf += "%PDF-1.7\n1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf += "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1" + parentExtra + " >>\nendobj\n";
    pdf += "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R" + pageExtra + " >>\nendobj\n";
    pdf += "4 0 obj\n<< /Length " + QByteArray::number(body.size())
         + filter + " >>\nstream\n" + body + "\nendstream\nendobj\n";
    qsizetype xo = pdf.size(); pdf += "xref\n0 5\n0000000000 65535 f \n";
    auto c = [&](int n) { auto p = pdf.indexOf(QByteArray::number(n) + " 0 obj");
        return QString("%1").arg(p, 10, 10, QLatin1Char('0')).toUtf8() + " 00000 n\n"; };
    pdf += c(1) + c(2) + c(3) + c(4);
    pdf += "trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n" + QByteArray::number(xo) + "\n%%EOF\n";
    return PdfDocumentLoader().load(pdf).document;
}

} // namespace PdfTestHelpers
