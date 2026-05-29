#include "PdfPageGeometryResolver.h"

using W = PdfPageGeometryWarning;

static PdfObjectPtr resolveInherited(const PdfDictionary *dict, const QByteArray &key,
                                      const PdfObjectStore &store,
                                      QSet<QPair<int,int>> &visited,
                                      int depth, W &warn, QString &diag) {
    if (!dict) return nullptr;

    auto val = dict->get(key);
    if (val) return val;

    if (depth > PdfPageGeometryResolver::MAX_PARENT_DEPTH) {
        warn = W::PageAttributeDepthExceeded;
        diag = QStringLiteral("Parent chain depth exceeded for key '%1'").arg(QString::fromUtf8(key));
        return nullptr;
    }

    auto parentRef = dict->get("Parent");
    if (!parentRef || parentRef->kind != PdfObjectKind::Reference) return nullptr;

    PdfRef pr = parentRef->refValue;
    if (pr.objNum <= 0) return nullptr;

    QPair<int,int> pk{pr.objNum, pr.genNum};
    if (visited.contains(pk)) {
        warn = W::PageAttributeCycle;
        diag = QStringLiteral("Parent chain cycle detected for key '%1'").arg(QString::fromUtf8(key));
        return nullptr;
    }
    visited.insert(pk);

    auto resolved = store.resolve(pr);
    if (!resolved.found() || !resolved.object || resolved.object->kind != PdfObjectKind::Dictionary) {
        warn = W::InvalidParentPageNode;
        diag = QStringLiteral("Invalid parent page node %1 %2 R").arg(pr.objNum).arg(pr.genNum);
        return nullptr;
    }

    return resolveInherited(resolved.object->dictValue.data(), key, store, visited, depth + 1, warn, diag);
}

static bool parseBox(const PdfObject *boxObj, PdfPageBox &box) {
    if (!boxObj || boxObj->kind != PdfObjectKind::Array) return false;
    auto &arr = boxObj->arrayValue;
    if (!arr || arr->elements.size() != 4) return false;

    double vals[4];
    for (int i = 0; i < 4; ++i) {
        const auto &e = arr->elements[i];
        if (e->kind == PdfObjectKind::Integer) vals[i] = static_cast<double>(e->intValue);
        else if (e->kind == PdfObjectKind::Real) vals[i] = e->realValue;
        else return false;
    }

    if (vals[0] > vals[2]) std::swap(vals[0], vals[2]);
    if (vals[1] > vals[3]) std::swap(vals[1], vals[3]);

    box.llx = vals[0]; box.lly = vals[1];
    box.urx = vals[2]; box.ury = vals[3];
    return true;
}

static int parseRotate(const PdfObject *rotObj, bool &ok) {
    if (!rotObj) { ok = true; return 0; }
    if (rotObj->kind != PdfObjectKind::Integer) { ok = false; return 0; }
    int64_t v = rotObj->intValue;
    if (v == 0 || v == 90 || v == 180 || v == 270) { ok = true; return static_cast<int>(v); }
    ok = false; return 0;
}

PdfPageGeometryResult PdfPageGeometryResolver::resolve(const PdfDocument &document, int pageIndex) {
    PdfPageGeometryResult result;
    result.geometry.pageIndex = pageIndex;

    if (pageIndex < 0 || pageIndex >= document.pageTree.pageRefs.size()) {
        result.warning = W::InvalidPageIndex;
        result.diagnosticMessage = QStringLiteral("Page index %1 out of range").arg(pageIndex);
        return result;
    }

    PdfRef pageRef = document.pageTree.pageRefs[pageIndex];
    result.geometry.pageRef = pageRef;

    auto resolved = document.store.resolve(pageRef);
    if (!resolved.found() || !resolved.object) {
        result.warning = W::PageNotFound;
        result.diagnosticMessage = QStringLiteral("Page %1 %2 R not found").arg(pageRef.objNum).arg(pageRef.genNum);
        return result;
    }

    if (resolved.object->kind != PdfObjectKind::Dictionary || !resolved.object->dictValue) {
        result.warning = W::PageNotDictionary;
        result.diagnosticMessage = QStringLiteral("Page %1 %2 R is not a dictionary").arg(pageRef.objNum).arg(pageRef.genNum);
        return result;
    }

    auto pageDict = resolved.object->dictValue.data();

    // --- MediaBox ---
    W mediaWarn = W::None; QString mediaDiag;
    QSet<QPair<int,int>> visited;
    visited.insert({pageRef.objNum, pageRef.genNum});
    auto mbObj = resolveInherited(pageDict, "MediaBox", document.store, visited, 0, mediaWarn, mediaDiag);
    bool mbOk = false;
    PdfPageBox mediaBox;
    if (mbObj) mbOk = parseBox(mbObj.get(), mediaBox);
    if (!mbOk || !mediaBox.isValid()) {
        if (mediaWarn != W::None) result.warning = mediaWarn;
        else result.warning = mbObj ? W::InvalidMediaBox : W::MissingMediaBox;
        result.diagnosticMessage = mediaWarn != W::None ? mediaDiag :
            QStringLiteral("Page %1: using Letter fallback for MediaBox").arg(pageRef.objNum);
        mediaBox = PdfPageBox{0,0,612,792};
    }
    result.geometry.mediaBox = mediaBox;

    // --- CropBox ---
    QSet<QPair<int,int>> cv;
    cv.insert({pageRef.objNum, pageRef.genNum});
    W cropWarn = W::None; QString cropDiag;
    auto cbObj = resolveInherited(pageDict, "CropBox", document.store, cv, 0, cropWarn, cropDiag);
    bool cbOk = false;
    PdfPageBox cropBox;
    if (cbObj) cbOk = parseBox(cbObj.get(), cropBox);
    if (!cbOk) {
        cropBox = mediaBox;
        if (cbObj && result.warning == W::None)
            result.warning = W::InvalidCropBox;
    }
    result.geometry.cropBox = cropBox;

    // --- VisibleBox ---
    PdfPageBox vb;
    vb.llx = std::max(mediaBox.llx, cropBox.llx);
    vb.lly = std::max(mediaBox.lly, cropBox.lly);
    vb.urx = std::min(mediaBox.urx, cropBox.urx);
    vb.ury = std::min(mediaBox.ury, cropBox.ury);
    if (!vb.isValid()) {
        if (result.warning == W::None) result.warning = W::InvalidCropBox;
        vb = mediaBox;
    }
    result.geometry.visibleBox = vb;

    // --- Rotate ---
    QSet<QPair<int,int>> rv;
    rv.insert({pageRef.objNum, pageRef.genNum});
    W rotWarn = W::None; QString rotDiag;
    auto rotObj = resolveInherited(pageDict, "Rotate", document.store, rv, 0, rotWarn, rotDiag);
    bool rotOk = false;
    int rot = parseRotate(rotObj.get(), rotOk);
    if (!rotOk) {
        if (result.warning == W::None) result.warning = W::InvalidRotate;
        result.diagnosticMessage = QStringLiteral("Invalid /Rotate value on page %1").arg(pageRef.objNum);
    }
    result.geometry.rotation = rot;

    return result;
}
