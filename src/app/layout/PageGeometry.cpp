#include "PageGeometry.h"

namespace mdbr::layout {

PageGeometry computePageGeometry(const PageLayoutSettings &settings, qreal logicalDpi)
{
    const QSizeF mm = paperSizeMm(settings.paper);
    PageGeometry g;
    g.pageSize = QSizeF(mmToPx(mm.width(), logicalDpi), mmToPx(mm.height(), logicalDpi));
    g.margin = mmToPx(marginMm(settings.margins), logicalDpi);
    return g;
}

} // namespace mdbr::layout
