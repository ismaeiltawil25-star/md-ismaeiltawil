#pragma once

// LatexMathConverter — turns the common subset of LaTeX math into HTML that
// QTextBrowser can show without a math engine:
//
//   \alpha \sum \le \to ...   → Unicode symbols (α ∑ ≤ →)
//   x^2  x_{i+1}              → <sup>2</sup>  <sub>i+1</sub>
//   \frac{a}{b}               → a⁄b with raised/lowered parts
//   \sqrt{x}  \sqrt[3]{x}     → √x̅  ³√x̅
//   \mathbb{R} \mathbf{v} \text{if}  → ℝ, bold, upright text
//   \left( \right)  \, \quad  → plain delimiters / spaces
//
// Unknown commands are kept visible as written ("\foo"), so content is never
// silently lost. The output is escaped HTML.
//
// This is a readable approximation, not typesetting. A full engine (e.g. the
// MIT-licensed MicroTeX) can replace it behind the same function signature.

#include <QString>
#include <QStringView>

namespace mdbr::render {

// `latex` is plain (unescaped) LaTeX source.
[[nodiscard]] QString latexToHtml(QStringView latex);

} // namespace mdbr::render
