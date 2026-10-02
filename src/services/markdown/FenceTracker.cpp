#include "FenceTracker.h"

#include <optional>

namespace mdbr::markdown {

namespace {

constexpr qsizetype kMaxFenceIndent = 3;
constexpr qsizetype kMinFenceLength = 3;

struct FenceRun
{
    QChar character;
    qsizetype length = 0;
    QStringView rest; // text after the fence characters
};

// Returns the fence run at the start of `line`, or nullopt if the line is not a fence.
[[nodiscard]] std::optional<FenceRun> parseFence(QStringView line)
{
    qsizetype pos = 0;
    while (pos < line.size() && line[pos] == u' ')
        ++pos;
    if (pos > kMaxFenceIndent || pos >= line.size())
        return std::nullopt;

    const QChar c = line[pos];
    if (c != u'`' && c != u'~')
        return std::nullopt;

    const qsizetype start = pos;
    while (pos < line.size() && line[pos] == c)
        ++pos;

    const qsizetype length = pos - start;
    if (length < kMinFenceLength)
        return std::nullopt;

    return FenceRun{c, length, line.sliced(pos)};
}

[[nodiscard]] bool isBlank(QStringView text)
{
    return text.trimmed().isEmpty();
}

} // namespace

bool FenceTracker::consume(QStringView line)
{
    const auto fence = parseFence(line);

    if (!isInsideFence()) {
        if (!fence)
            return false;
        // An info string after a backtick fence may not contain backticks.
        if (fence->character == u'`' && fence->rest.contains(u'`'))
            return false;
        m_fenceChar = fence->character;
        m_openLength = fence->length;
        return true;
    }

    const bool closes = fence
        && fence->character == m_fenceChar
        && fence->length >= m_openLength
        && isBlank(fence->rest);
    if (closes)
        m_openLength = 0;
    return true;
}

} // namespace mdbr::markdown
