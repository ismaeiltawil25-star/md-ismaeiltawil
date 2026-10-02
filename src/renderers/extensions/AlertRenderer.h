#pragma once

// AlertRenderer — GitHub alerts (callouts):
//
//   > [!NOTE]
//   > Useful information.
//
// Types: NOTE, TIP, IMPORTANT, WARNING, CAUTION (case-insensitive).
// The marker is replaced by a coloured, bold title with an icon, and the
// blockquote is tagged with data-alert-color so wrapBlockquotes() colours the
// quote bar. Any other blockquote is left unchanged.
//
// Must run BEFORE HtmlElementWrapper::wrapBlockquotes().

#include <QString>

namespace mdbr::render {

[[nodiscard]] QString renderAlerts(const QString &html);

} // namespace mdbr::render
