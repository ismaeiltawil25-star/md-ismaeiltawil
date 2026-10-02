#pragma once

// FenceTracker — tells a line-by-line scanner whether it is inside a fenced
// code block, following CommonMark 0.31 §4.5:
//   * a fence is 3+ backticks or 3+ tildes, indented by at most 3 spaces;
//   * a backtick fence's info string must not contain a backtick;
//   * the closing fence uses the same character, is at least as long as the
//     opening fence, and is followed only by whitespace.
//
// Any text-level preprocessor that rewrites Markdown MUST use this before
// touching a line, so code samples are never modified.

#include <QChar>
#include <QStringView>

namespace mdbr::markdown {

class FenceTracker
{
public:
    // Feed the next line (without its line terminator).
    // Returns true when the line belongs to a fenced block: the opening fence,
    // any content line, or the closing fence.
    bool consume(QStringView line);

    [[nodiscard]] bool isInsideFence() const noexcept { return m_openLength > 0; }

private:
    QChar m_fenceChar;
    qsizetype m_openLength = 0;
};

} // namespace mdbr::markdown
