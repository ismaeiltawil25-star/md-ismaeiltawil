#pragma once

#include "PdfCatalog.h"
#include "PdfObjectStore.h"
#include "PdfPageTree.h"
#include <QString>
#include <QSet>

enum class PdfPageTreeResolverWarning : uint8_t {
    None,
    UnresolvedPageNode,
    InvalidPageNode,
    InvalidPageType,
    MissingKids,
    InvalidKids,
    InvalidKidReference,
    MissingCount,
    InvalidCount,
    CountMismatch,
    CycleDetected,
    DepthExceeded,
};

struct PdfPageTreeResult {
    PdfPageTree tree;
    PdfPageTreeResolverWarning warning = PdfPageTreeResolverWarning::None;
    QString diagnosticMessage;

    bool ok() const { return warning == PdfPageTreeResolverWarning::None; }
};

struct PdfRefKey {
    int objNum;
    int genNum;
    bool operator==(const PdfRefKey &o) const = default;
};

inline size_t qHash(const PdfRefKey &k, size_t seed = 0) {
    return qHash(static_cast<quint64>(k.objNum) << 32 | static_cast<quint32>(k.genNum), seed);
}

class PdfPageTreeResolver {
public:
    static constexpr int MAX_DEPTH = 128;

    PdfPageTreeResult resolve(const PdfCatalog &catalog,
                              const PdfObjectStore &store);
};
