#pragma once

// EmojiShortcodes — GitHub/Slack style ":name:" → Unicode emoji.
//
// The table lives in EmojiShortcodes.cpp. To add an emoji, append one line
// there (name without colons, value as a \U escape); EmojiShortcodesTest
// checks that every entry is unique and non-empty.

#include <QString>
#include <QStringView>

#include <optional>

namespace mdbr::render {

// Returns the emoji for a shortcode name (without colons), if known.
[[nodiscard]] std::optional<QString> emojiForShortcode(QStringView name);

// Number of known shortcodes (for tests and diagnostics).
[[nodiscard]] qsizetype emojiShortcodeCount();

} // namespace mdbr::render
