#include "PdfPageTreeResolver.h"
#include <functional>

static bool isStructuralWarning(PdfPageTreeResolverWarning w) {
    return w >= PdfPageTreeResolverWarning::UnresolvedPageNode
        && w <= PdfPageTreeResolverWarning::DepthExceeded;
}

PdfPageTreeResult PdfPageTreeResolver::resolve(const PdfCatalog &catalog,
                                                 const PdfObjectStore &store) {
    PdfPageTreeResult result;

    if (catalog.pagesRef.objNum <= 0 || catalog.pagesRef.genNum < 0) {
        result.warning = PdfPageTreeResolverWarning::UnresolvedPageNode;
        result.diagnosticMessage = QStringLiteral("Invalid pages reference: %1 %2 R")
            .arg(catalog.pagesRef.objNum).arg(catalog.pagesRef.genNum);
        return result;
    }

    auto rootResolved = store.resolve(catalog.pagesRef);
    if (!rootResolved.found()) {
        result.warning = PdfPageTreeResolverWarning::UnresolvedPageNode;
        result.diagnosticMessage = QStringLiteral("Root pages object not found");
        return result;
    }

    auto &root = rootResolved.object;
    if (root->kind != PdfObjectKind::Dictionary) {
        result.warning = PdfPageTreeResolverWarning::InvalidPageNode;
        result.diagnosticMessage = QStringLiteral("Root pages object is not a dictionary");
        return result;
    }

    auto &rootDict = root->dictValue;

    // Root /Type check
    auto rootType = rootDict->get("Type");
    if (rootType && (rootType->kind != PdfObjectKind::Name || rootType->nameValue != "Pages")) {
        result.warning = PdfPageTreeResolverWarning::InvalidPageType;
        result.diagnosticMessage = QStringLiteral("Root object /Type is not Pages");
        return result;
    }

    // Root /Kids
    auto rootKids = rootDict->get("Kids");
    if (!rootKids) {
        result.warning = PdfPageTreeResolverWarning::MissingKids;
        result.diagnosticMessage = QStringLiteral("Root Pages node missing /Kids");
        return result;
    }
    if (rootKids->kind != PdfObjectKind::Array) {
        result.warning = PdfPageTreeResolverWarning::InvalidKids;
        result.diagnosticMessage = QStringLiteral("Root /Kids is not an array");
        return result;
    }

    // Read /Count internally
    bool hasCount = false;
    bool countValid = false;
    int declaredCount = -1;
    PdfPageTreeResolverWarning countWarning = PdfPageTreeResolverWarning::None;

    auto countObj = rootDict->get("Count");
    if (countObj) {
        hasCount = true;
        if (countObj->kind == PdfObjectKind::Integer) {
            int64_t cv = countObj->intValue;
            if (cv >= 0) {
                countValid = true;
                declaredCount = static_cast<int>(cv);
            } else {
                countWarning = PdfPageTreeResolverWarning::InvalidCount;
            }
        } else {
            countWarning = PdfPageTreeResolverWarning::InvalidCount;
        }
    } else {
        countWarning = PdfPageTreeResolverWarning::MissingCount;
    }

    result.tree.declaredCount = declaredCount;

    // Traverse
    QSet<PdfRefKey> activePath;
    bool hasStructuralError = false;

    std::function<void(const PdfRef &, int)> traverse;
    traverse = [&](const PdfRef &ref, int depth) {
        if (depth > MAX_DEPTH) {
            if (!hasStructuralError)
                result.warning = PdfPageTreeResolverWarning::DepthExceeded;
            hasStructuralError = true;
            return;
        }

        PdfRefKey key{ref.objNum, ref.genNum};
        if (activePath.contains(key)) {
            if (!hasStructuralError)
                result.warning = PdfPageTreeResolverWarning::CycleDetected;
            hasStructuralError = true;
            return;
        }

        auto resolved = store.resolve(ref);
        if (!resolved.found()) {
            if (!hasStructuralError)
                result.warning = PdfPageTreeResolverWarning::UnresolvedPageNode;
            hasStructuralError = true;
            return;
        }

        auto obj = resolved.object;
        if (obj->kind != PdfObjectKind::Dictionary) {
            if (!hasStructuralError)
                result.warning = PdfPageTreeResolverWarning::InvalidPageNode;
            hasStructuralError = true;
            return;
        }

        auto &dict = obj->dictValue;

        // Classify node
        auto type = dict->get("Type");
        bool isPages = false;
        bool isPage = false;

        if (type) {
            if (type->kind == PdfObjectKind::Name) {
                if (type->nameValue == "Pages") isPages = true;
                else if (type->nameValue == "Page") isPage = true;
                else {
                    if (!hasStructuralError)
                        result.warning = PdfPageTreeResolverWarning::InvalidPageType;
                    hasStructuralError = true;
                    return;
                }
            } else {
                if (!hasStructuralError)
                    result.warning = PdfPageTreeResolverWarning::InvalidPageType;
                hasStructuralError = true;
                return;
            }
        }

        // If /Type absent, use /Kids heuristic
        if (!isPages && !isPage) {
            auto kids = dict->get("Kids");
            if (kids) {
                if (kids->kind == PdfObjectKind::Array)
                    isPages = true;
                else {
                    if (!hasStructuralError)
                        result.warning = PdfPageTreeResolverWarning::InvalidKids;
                    hasStructuralError = true;
                    return;
                }
            } else {
                isPage = true;
            }
        }

        if (isPage) {
            result.tree.pageRefs.append(ref);
            result.tree.actualCount++;
            return;
        }

        // Pages node
        auto kidsObj = dict->get("Kids");
        if (!kidsObj) {
            if (!hasStructuralError)
                result.warning = PdfPageTreeResolverWarning::MissingKids;
            hasStructuralError = true;
            return;
        }
        if (kidsObj->kind != PdfObjectKind::Array) {
            if (!hasStructuralError)
                result.warning = PdfPageTreeResolverWarning::InvalidKids;
            hasStructuralError = true;
            return;
        }

        activePath.insert(key);
        for (auto &kid : kidsObj->arrayValue->elements) {
            if (kid->kind != PdfObjectKind::Reference) {
                if (!hasStructuralError)
                    result.warning = PdfPageTreeResolverWarning::InvalidKidReference;
                hasStructuralError = true;
                continue;
            }
            auto &kidRef = kid->refValue;
            if (kidRef.objNum <= 0 || kidRef.genNum < 0) {
                if (!hasStructuralError)
                    result.warning = PdfPageTreeResolverWarning::InvalidKidReference;
                hasStructuralError = true;
                continue;
            }
            traverse(kidRef, depth + 1);
        }
        activePath.remove(key);
    };

    // Iterate kids without inserting root into activePath for the first level
    // Root is not a kid of itself — just a starting point. But we DO insert
    // root for cycle checking of deeper references back to root.
    activePath.insert(PdfRefKey{catalog.pagesRef.objNum, catalog.pagesRef.genNum});
    for (auto &kid : rootKids->arrayValue->elements) {
        if (kid->kind != PdfObjectKind::Reference) {
            if (!hasStructuralError)
                result.warning = PdfPageTreeResolverWarning::InvalidKidReference;
            hasStructuralError = true;
            continue;
        }
        auto &kidRef = kid->refValue;
        if (kidRef.objNum <= 0 || kidRef.genNum < 0) {
            if (!hasStructuralError)
                result.warning = PdfPageTreeResolverWarning::InvalidKidReference;
            hasStructuralError = true;
            continue;
        }
        traverse(kidRef, 1);
    }
    activePath.remove(PdfRefKey{catalog.pagesRef.objNum, catalog.pagesRef.genNum});

    // Apply count diagnostics only if no structural error
    if (!hasStructuralError) {
        if (declaredCount >= 0 && declaredCount != result.tree.actualCount) {
            result.warning = PdfPageTreeResolverWarning::CountMismatch;
            result.diagnosticMessage = QStringLiteral("/Count %1 does not match actual page count %2")
                .arg(declaredCount).arg(result.tree.actualCount);
        } else if (countWarning != PdfPageTreeResolverWarning::None) {
            result.warning = countWarning;
            if (countWarning == PdfPageTreeResolverWarning::MissingCount)
                result.diagnosticMessage = QStringLiteral("Root Pages node missing /Count");
            else
                result.diagnosticMessage = QStringLiteral("Root Pages node /Count is invalid");
        }
    }

    return result;
}
