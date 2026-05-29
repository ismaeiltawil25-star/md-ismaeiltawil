#pragma once

#include "PdfObject.h"
#include <QList>

struct PdfPageTree {
    QList<PdfRef> pageRefs;
    int actualCount = 0;         // = pageRefs.size()
    int declaredCount = -1;      // /Count from root, -1 if absent/invalid
};
