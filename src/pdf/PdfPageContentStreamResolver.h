#pragma once

#include "PdfPageTree.h"
#include "PdfObjectStore.h"
#include "PdfRawStreamStore.h"
#include "PdfStreamDecoder.h"
#include <QByteArray>
#include <QString>
#include <cstdint>

enum class PdfPageContentStreamResolverWarning : uint8_t {
    None,
    InvalidPageIndex,
    PageNotFound,
    PageNotDictionary,
    InvalidContentsType,
    UnresolvedContentReference,
    InvalidContentsEntry,
    ContentNotStream,
    RawStreamNotFound,
    DecodeFailure,
};

struct PdfPageContentResult {
    QByteArray contentBytes;
    int pageIndex = -1;
    PdfRef pageRef;
    PdfPageContentStreamResolverWarning warning = PdfPageContentStreamResolverWarning::None;
    PdfStreamDecoderWarning decoderWarning = PdfStreamDecoderWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfPageContentStreamResolverWarning::None; }
};

class PdfPageContentStreamResolver {
public:
    PdfPageContentStreamResolver(const PdfObjectStore &store,
                                  const PdfRawStreamStore &streamStore,
                                  PdfStreamDecoder &decoder);

    PdfPageContentResult resolve(const PdfPageTree &tree, int pageIndex);

private:
    const PdfObjectStore &m_store;
    const PdfRawStreamStore &m_streamStore;
    PdfStreamDecoder &m_decoder;

    PdfPageContentResult fail(int pageIndex, PdfRef pageRef,
                               PdfPageContentStreamResolverWarning w,
                               const QString &msg) const;
};
