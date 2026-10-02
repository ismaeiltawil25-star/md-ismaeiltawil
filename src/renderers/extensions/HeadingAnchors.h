#pragma once

// HeadingAnchors — gives every heading a named anchor, so in-document links
// like [Install](#install) work (GitHub-compatible slugs).
//
// Slug rules (as on GitHub): lower-case; keep letters, digits, '-' and '_'
// (any script, so Arabic headings get Arabic slugs); spaces become '-';
// everything else is dropped; repeated slugs get -1, -2, ... suffixes.

#include <QString>
#include <QStringView>

namespace mdbr::render {

[[nodiscard]] QString slugify(QStringView headingText);

[[nodiscard]] QString addHeadingAnchors(const QString &html);

} // namespace mdbr::render
