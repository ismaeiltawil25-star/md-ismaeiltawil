#pragma once

#include "PdfDocument.h"
#include <QString>
#include <cstdint>

struct PdfPageBox {
    double llx = 0.0, lly = 0.0;
    double urx = 612.0, ury = 792.0;

    double width() const { return urx - llx; }
    double height() const { return ury - lly; }
    bool isValid() const { return width() > 0.0 && height() > 0.0; }
};

struct PdfPageGeometry {
    PdfPageBox mediaBox;
    PdfPageBox cropBox;
    PdfPageBox visibleBox;
    int rotation = 0;
    int pageIndex = -1;
    PdfRef pageRef;
};

enum class PdfPageGeometryWarning : uint8_t {
    None,
    InvalidPageIndex,
    PageNotFound,
    PageNotDictionary,
    MissingMediaBox,
    InvalidMediaBox,
    InvalidCropBox,
    InvalidRotate,
    InvalidParentPageNode,
    PageAttributeCycle,
    PageAttributeDepthExceeded,
};

struct PdfPageGeometryResult {
    PdfPageGeometry geometry;
    PdfPageGeometryWarning warning = PdfPageGeometryWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfPageGeometryWarning::None; }
};

class PdfPageGeometryResolver {
public:
    static constexpr int MAX_PARENT_DEPTH = 128;

    PdfPageGeometryResult resolve(const PdfDocument &document, int pageIndex);
};
