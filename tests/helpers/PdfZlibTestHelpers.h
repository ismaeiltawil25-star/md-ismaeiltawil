#pragma once

#include <QtZlib/zlib.h>
#include <QByteArray>

namespace PdfTestHelpers {

inline QByteArray zlibCompress(const QByteArray &input) {
    uLongf bound = compressBound(static_cast<uLongf>(input.size()));
    QByteArray compressed(static_cast<qsizetype>(bound), '\0');
    uLongf destLen = bound;
    int ret = compress2(reinterpret_cast<Bytef *>(compressed.data()), &destLen,
                        reinterpret_cast<const Bytef *>(input.constData()),
                        static_cast<uLongf>(input.size()), Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) return {};
    compressed.resize(static_cast<qsizetype>(destLen));
    return compressed;
}

} // namespace PdfTestHelpers
