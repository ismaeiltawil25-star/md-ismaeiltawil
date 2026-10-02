#pragma once

// HtmlTextRuns — the safe way for an extension to rewrite visible text.
//
// Walks rendered HTML once and hands every run of text that sits OUTSIDE tags
// to a callback, which returns replacement HTML. Text inside protected
// elements is passed through untouched:
//   <code>, <pre>        code must never be reinterpreted
//   <a>                  link text/targets stay as written
//   <x-equation>         math is handled by MathRenderer
//   <script>, <style>    never visible
//
// Text runs are still HTML-escaped (&amp;, &lt; ...). Callbacks must not
// unescape, and anything they insert must be valid HTML.
//
// Complexity: O(n) in the HTML length.

#include <QString>
#include <QStringView>

#include <functional>

namespace mdbr::render {

using TextRunTransform = std::function<QString(QStringView text)>;

[[nodiscard]] QString transformTextRuns(const QString &html, const TextRunTransform &transform);

} // namespace mdbr::render
