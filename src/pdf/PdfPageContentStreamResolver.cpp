#include "PdfPageContentStreamResolver.h"

PdfPageContentStreamResolver::PdfPageContentStreamResolver(
    const PdfObjectStore &store,
    const PdfRawStreamStore &streamStore,
    PdfStreamDecoder &decoder)
    : m_store(store), m_streamStore(streamStore), m_decoder(decoder) {}

PdfPageContentResult PdfPageContentStreamResolver::fail(
    int pageIndex, PdfRef pageRef,
    PdfPageContentStreamResolverWarning w, const QString &msg) const {
    PdfPageContentResult r;
    r.pageIndex = pageIndex;
    r.pageRef = pageRef;
    r.warning = w;
    r.diagnosticMessage = msg;
    return r;
}

PdfPageContentResult PdfPageContentStreamResolver::resolve(
    const PdfPageTree &tree, int pageIndex) {

    if (pageIndex < 0 || pageIndex >= tree.pageRefs.size()) {
        return fail(pageIndex, PdfRef{},
                     PdfPageContentStreamResolverWarning::InvalidPageIndex,
                     QStringLiteral("Page index %1 out of range [0, %2)")
                         .arg(pageIndex).arg(tree.pageRefs.size()));
    }

    PdfRef pageRef = tree.pageRefs[pageIndex];

    auto pageResolved = m_store.resolve(pageRef);
    if (!pageResolved.found() || !pageResolved.object) {
        return fail(pageIndex, pageRef,
                     PdfPageContentStreamResolverWarning::PageNotFound,
                     QStringLiteral("Page %1 %2 not found in object store")
                         .arg(pageRef.objNum).arg(pageRef.genNum));
    }

    PdfObjectPtr pageObj = pageResolved.object;
    if (pageObj->kind != PdfObjectKind::Dictionary || !pageObj->dictValue) {
        return fail(pageIndex, pageRef,
                     PdfPageContentStreamResolverWarning::PageNotDictionary,
                     QStringLiteral("Page %1 %2 is not a dictionary")
                         .arg(pageRef.objNum).arg(pageRef.genNum));
    }

    auto contentsIt = pageObj->dictValue->entries.find(QByteArray("Contents"));
    if (contentsIt == pageObj->dictValue->entries.end()) {
        PdfPageContentResult r;
        r.pageIndex = pageIndex;
        r.pageRef = pageRef;
        return r;
    }

    PdfObjectPtr contents = contentsIt.value();

    if (contents->kind == PdfObjectKind::Reference) {
        PdfRef contentRef = contents->refValue;

        auto resolved = m_store.resolve(contentRef);
        if (!resolved.found() || !resolved.object) {
            return fail(pageIndex, pageRef,
                         PdfPageContentStreamResolverWarning::UnresolvedContentReference,
                         QStringLiteral("Content stream %1 %2 not found")
                             .arg(contentRef.objNum).arg(contentRef.genNum));
        }

        PdfObjectPtr contentObj = resolved.object;
        if (contentObj->kind != PdfObjectKind::Dictionary) {
            return fail(pageIndex, pageRef,
                         PdfPageContentStreamResolverWarning::ContentNotStream,
                         QStringLiteral("Content ref %1 %2 is not a dictionary")
                             .arg(contentRef.objNum).arg(contentRef.genNum));
        }

        const PdfRawStreamObject *rawStream = m_streamStore.get(contentRef);
        if (!rawStream) {
            return fail(pageIndex, pageRef,
                         PdfPageContentStreamResolverWarning::RawStreamNotFound,
                         QStringLiteral("Raw stream %1 %2 not in stream store")
                             .arg(contentRef.objNum).arg(contentRef.genNum));
        }

        auto decodeResult = m_decoder.decode(*rawStream);
        if (!decodeResult.ok()) {
            PdfPageContentResult r;
            r.pageIndex = pageIndex;
            r.pageRef = pageRef;
            r.warning = PdfPageContentStreamResolverWarning::DecodeFailure;
            r.decoderWarning = decodeResult.warning;
            r.diagnosticMessage = QStringLiteral("Decode failure for stream %1 %2: %3")
                .arg(contentRef.objNum).arg(contentRef.genNum)
                .arg(decodeResult.diagnosticMessage);
            return r;
        }

        PdfPageContentResult r;
        r.pageIndex = pageIndex;
        r.pageRef = pageRef;
        r.contentBytes = std::move(decodeResult.decodedBytes);
        return r;
    }

    if (contents->kind == PdfObjectKind::Array) {
        auto arr = contents->arrayValue;
        if (!arr) {
            return fail(pageIndex, pageRef,
                         PdfPageContentStreamResolverWarning::InvalidContentsType,
                         QStringLiteral("Page %1 %2 Contents array is null")
                             .arg(pageRef.objNum).arg(pageRef.genNum));
        }

        QByteArray result;
        for (int i = 0; i < arr->elements.size(); ++i) {
            const auto &elem = arr->elements[i];

            if (!elem || elem->kind != PdfObjectKind::Reference) {
                return fail(pageIndex, pageRef,
                             PdfPageContentStreamResolverWarning::InvalidContentsEntry,
                             QStringLiteral("Contents array element %1 is not a reference").arg(i));
            }

            PdfRef contentRef = elem->refValue;

            auto resolved = m_store.resolve(contentRef);
            if (!resolved.found() || !resolved.object) {
                return fail(pageIndex, pageRef,
                             PdfPageContentStreamResolverWarning::UnresolvedContentReference,
                             QStringLiteral("Content stream %1 %2 not found (array element %3)")
                                 .arg(contentRef.objNum).arg(contentRef.genNum).arg(i));
            }

            PdfObjectPtr contentObj = resolved.object;
            if (contentObj->kind != PdfObjectKind::Dictionary) {
                return fail(pageIndex, pageRef,
                             PdfPageContentStreamResolverWarning::ContentNotStream,
                             QStringLiteral("Content ref %1 %2 is not a dictionary (array element %3)")
                                 .arg(contentRef.objNum).arg(contentRef.genNum).arg(i));
            }

            const PdfRawStreamObject *rawStream = m_streamStore.get(contentRef);
            if (!rawStream) {
                return fail(pageIndex, pageRef,
                             PdfPageContentStreamResolverWarning::RawStreamNotFound,
                             QStringLiteral("Raw stream %1 %2 not in stream store (array element %3)")
                                 .arg(contentRef.objNum).arg(contentRef.genNum).arg(i));
            }

            auto decodeResult = m_decoder.decode(*rawStream);
            if (!decodeResult.ok()) {
                PdfPageContentResult r;
                r.pageIndex = pageIndex;
                r.pageRef = pageRef;
                r.warning = PdfPageContentStreamResolverWarning::DecodeFailure;
                r.decoderWarning = decodeResult.warning;
                r.diagnosticMessage = QStringLiteral("Decode failure for stream %1 %2 (array element %3): %4")
                    .arg(contentRef.objNum).arg(contentRef.genNum).arg(i)
                    .arg(decodeResult.diagnosticMessage);
                return r;
            }

            result.append(decodeResult.decodedBytes);
        }

        PdfPageContentResult r;
        r.pageIndex = pageIndex;
        r.pageRef = pageRef;
        r.contentBytes = std::move(result);
        return r;
    }

    return fail(pageIndex, pageRef,
                 PdfPageContentStreamResolverWarning::InvalidContentsType,
                 QStringLiteral("Page %1 %2 /Contents is not a reference or array")
                     .arg(pageRef.objNum).arg(pageRef.genNum));
}
