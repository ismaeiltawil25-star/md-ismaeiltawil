#include "PageLayoutSettings.h"

namespace mdbr::layout {

QSizeF paperSizeMm(PaperSize paper)
{
    switch (paper) {
    case PaperSize::A4:     return {210.0, 297.0};
    case PaperSize::A5:     return {148.0, 210.0};
    case PaperSize::Letter: return {215.9, 279.4};
    case PaperSize::Legal:  return {215.9, 355.6};
    }
    return {210.0, 297.0};
}

qreal marginMm(PageMargins margins)
{
    switch (margins) {
    case PageMargins::Narrow:   return 12.7;
    case PageMargins::Moderate: return 19.1;
    case PageMargins::Normal:   return 25.4;
    case PageMargins::Wide:     return 38.1;
    }
    return 25.4;
}

QString paperId(PaperSize paper)
{
    switch (paper) {
    case PaperSize::A4:     return QStringLiteral("A4");
    case PaperSize::A5:     return QStringLiteral("A5");
    case PaperSize::Letter: return QStringLiteral("Letter");
    case PaperSize::Legal:  return QStringLiteral("Legal");
    }
    return QStringLiteral("A4");
}

QString marginsId(PageMargins margins)
{
    switch (margins) {
    case PageMargins::Narrow:   return QStringLiteral("narrow");
    case PageMargins::Moderate: return QStringLiteral("moderate");
    case PageMargins::Normal:   return QStringLiteral("normal");
    case PageMargins::Wide:     return QStringLiteral("wide");
    }
    return QStringLiteral("normal");
}

PaperSize paperFromId(const QString &id, PaperSize fallback)
{
    for (const PaperSize p : {PaperSize::A4, PaperSize::A5, PaperSize::Letter, PaperSize::Legal}) {
        if (paperId(p).compare(id, Qt::CaseInsensitive) == 0)
            return p;
    }
    return fallback;
}

PageMargins marginsFromId(const QString &id, PageMargins fallback)
{
    for (const PageMargins m : {PageMargins::Narrow, PageMargins::Moderate,
                                PageMargins::Normal, PageMargins::Wide}) {
        if (marginsId(m).compare(id, Qt::CaseInsensitive) == 0)
            return m;
    }
    return fallback;
}

QString paperLabel(PaperSize paper)
{
    switch (paper) {
    case PaperSize::A4:     return QStringLiteral("A4 (210 \u00D7 297 mm)");
    case PaperSize::A5:     return QStringLiteral("A5 (148 \u00D7 210 mm)");
    case PaperSize::Letter: return QStringLiteral("Letter (8.5 \u00D7 11 in)");
    case PaperSize::Legal:  return QStringLiteral("Legal (8.5 \u00D7 14 in)");
    }
    return paperId(paper);
}

QString marginsLabel(PageMargins margins)
{
    switch (margins) {
    case PageMargins::Narrow:   return QStringLiteral("Narrow (1.27 cm)");
    case PageMargins::Moderate: return QStringLiteral("Moderate (1.91 cm)");
    case PageMargins::Normal:   return QStringLiteral("Normal (2.54 cm)");
    case PageMargins::Wide:     return QStringLiteral("Wide (3.81 cm)");
    }
    return marginsId(margins);
}

} // namespace mdbr::layout
