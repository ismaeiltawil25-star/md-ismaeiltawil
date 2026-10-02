#pragma once

// HtmlEntities — the minimal escaping/unescaping the extension stages need.
// md4c escapes text as &amp; &lt; &gt; &quot;; numeric entities can come
// from the source document.

#include <QString>
#include <QStringView>

namespace mdbr::render {

// Decodes &amp; &lt; &gt; &quot; &#39; &apos; and numeric (&#169; &#xA9;) entities.
[[nodiscard]] QString decodeHtmlEntities(QStringView text);

// Escapes & < > " for use inside text or a double-quoted attribute.
[[nodiscard]] QString escapeHtmlAttribute(QStringView text);

} // namespace mdbr::render
