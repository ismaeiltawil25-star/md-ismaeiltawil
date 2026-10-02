#include "EmojiShortcodes.h"

#include <QHash>

namespace mdbr::render {

namespace {

struct Entry
{
    const char *name;
    const char16_t *emoji;
};

// Most-used GitHub shortcodes. Names follow github.com/github/gemoji.
constexpr Entry kEntries[] = {
    // Faces
    {"smile", u"\U0001F604"}, {"smiley", u"\U0001F603"}, {"grinning", u"\U0001F600"},
    {"grin", u"\U0001F601"}, {"laughing", u"\U0001F606"}, {"joy", u"\U0001F602"},
    {"rofl", u"\U0001F923"}, {"wink", u"\U0001F609"}, {"blush", u"\U0001F60A"},
    {"innocent", u"\U0001F607"}, {"heart_eyes", u"\U0001F60D"}, {"kissing_heart", u"\U0001F618"},
    {"yum", u"\U0001F60B"}, {"sunglasses", u"\U0001F60E"}, {"smirk", u"\U0001F60F"},
    {"neutral_face", u"\U0001F610"}, {"expressionless", u"\U0001F611"}, {"unamused", u"\U0001F612"},
    {"sweat_smile", u"\U0001F605"}, {"sweat", u"\U0001F613"}, {"pensive", u"\U0001F614"},
    {"confused", u"\U0001F615"}, {"thinking", u"\U0001F914"}, {"relieved", u"\U0001F60C"},
    {"stuck_out_tongue", u"\U0001F61B"}, {"disappointed", u"\U0001F61E"}, {"worried", u"\U0001F61F"},
    {"angry", u"\U0001F620"}, {"rage", u"\U0001F621"}, {"cry", u"\U0001F622"},
    {"sob", u"\U0001F62D"}, {"scream", u"\U0001F631"}, {"astonished", u"\U0001F632"},
    {"flushed", u"\U0001F633"}, {"sleeping", u"\U0001F634"}, {"mask", u"\U0001F637"},
    {"slightly_smiling_face", u"\U0001F642"}, {"upside_down_face", u"\U0001F643"},
    {"roll_eyes", u"\U0001F644"}, {"zipper_mouth_face", u"\U0001F910"}, {"nerd_face", u"\U0001F913"},
    {"hugs", u"\U0001F917"}, {"partying_face", u"\U0001F973"}, {"star_struck", u"\U0001F929"},
    {"exploding_head", u"\U0001F92F"}, {"skull", u"\U0001F480"}, {"ghost", u"\U0001F47B"},
    {"robot", u"\U0001F916"}, {"see_no_evil", u"\U0001F648"},

    // Hands and people
    {"+1", u"\U0001F44D"}, {"thumbsup", u"\U0001F44D"}, {"-1", u"\U0001F44E"},
    {"thumbsdown", u"\U0001F44E"}, {"ok_hand", u"\U0001F44C"}, {"clap", u"\U0001F44F"},
    {"wave", u"\U0001F44B"}, {"raised_hands", u"\U0001F64C"}, {"pray", u"\U0001F64F"},
    {"muscle", u"\U0001F4AA"}, {"point_right", u"\U0001F449"}, {"point_left", u"\U0001F448"},
    {"point_up", u"\u261D\uFE0F"}, {"point_down", u"\U0001F447"}, {"v", u"\u270C\uFE0F"},
    {"handshake", u"\U0001F91D"}, {"eyes", u"\U0001F440"}, {"brain", u"\U0001F9E0"},

    // Hearts and symbols
    {"heart", u"\u2764\uFE0F"}, {"broken_heart", u"\U0001F494"}, {"sparkling_heart", u"\U0001F496"},
    {"blue_heart", u"\U0001F499"}, {"green_heart", u"\U0001F49A"}, {"yellow_heart", u"\U0001F49B"},
    {"purple_heart", u"\U0001F49C"}, {"orange_heart", u"\U0001F9E1"}, {"black_heart", u"\U0001F5A4"},
    {"100", u"\U0001F4AF"}, {"fire", u"\U0001F525"}, {"sparkles", u"\u2728"},
    {"star", u"\u2B50"}, {"star2", u"\U0001F31F"}, {"zap", u"\u26A1"},
    {"boom", u"\U0001F4A5"}, {"tada", u"\U0001F389"}, {"confetti_ball", u"\U0001F38A"},
    {"gift", u"\U0001F381"}, {"trophy", u"\U0001F3C6"}, {"medal_sports", u"\U0001F3C5"},
    {"crown", u"\U0001F451"}, {"gem", u"\U0001F48E"}, {"bell", u"\U0001F514"},

    // Status and marks
    {"white_check_mark", u"\u2705"}, {"heavy_check_mark", u"\u2714\uFE0F"},
    {"ballot_box_with_check", u"\u2611\uFE0F"}, {"x", u"\u274C"}, {"negative_squared_cross_mark", u"\u274E"},
    {"heavy_multiplication_x", u"\u2716\uFE0F"}, {"warning", u"\u26A0\uFE0F"},
    {"no_entry", u"\u26D4"}, {"no_entry_sign", u"\U0001F6AB"}, {"stop_sign", u"\U0001F6D1"},
    {"question", u"\u2753"}, {"grey_question", u"\u2754"}, {"exclamation", u"\u2757"},
    {"grey_exclamation", u"\u2755"}, {"bangbang", u"\u203C\uFE0F"}, {"information_source", u"\u2139\uFE0F"},
    {"red_circle", u"\U0001F534"}, {"large_blue_circle", u"\U0001F535"}, {"green_circle", u"\U0001F7E2"},
    {"yellow_circle", u"\U0001F7E1"}, {"white_circle", u"\u26AA"}, {"black_circle", u"\u26AB"},
    {"arrow_right", u"\u27A1\uFE0F"}, {"arrow_left", u"\u2B05\uFE0F"}, {"arrow_up", u"\u2B06\uFE0F"},
    {"arrow_down", u"\u2B07\uFE0F"}, {"arrows_counterclockwise", u"\U0001F504"},
    {"heavy_plus_sign", u"\u2795"}, {"heavy_minus_sign", u"\u2796"}, {"recycle", u"\u267B\uFE0F"},

    // Work and tech
    {"rocket", u"\U0001F680"}, {"bug", u"\U0001F41B"}, {"memo", u"\U0001F4DD"},
    {"pencil", u"\U0001F4DD"}, {"pencil2", u"\u270F\uFE0F"}, {"book", u"\U0001F4D6"},
    {"books", u"\U0001F4DA"}, {"bookmark", u"\U0001F516"}, {"link", u"\U0001F517"},
    {"paperclip", u"\U0001F4CE"}, {"pushpin", u"\U0001F4CC"}, {"round_pushpin", u"\U0001F4CD"},
    {"calendar", u"\U0001F4C6"}, {"date", u"\U0001F4C5"}, {"clock", u"\U0001F570\uFE0F"},
    {"hourglass", u"\u231B"}, {"alarm_clock", u"\u23F0"}, {"stopwatch", u"\u23F1\uFE0F"},
    {"bulb", u"\U0001F4A1"}, {"wrench", u"\U0001F527"}, {"hammer", u"\U0001F528"},
    {"hammer_and_wrench", u"\U0001F6E0\uFE0F"}, {"gear", u"\u2699\uFE0F"}, {"lock", u"\U0001F512"},
    {"unlock", u"\U0001F513"}, {"key", u"\U0001F511"}, {"mag", u"\U0001F50D"},
    {"computer", u"\U0001F4BB"}, {"desktop_computer", u"\U0001F5A5\uFE0F"}, {"keyboard", u"\u2328\uFE0F"},
    {"iphone", u"\U0001F4F1"}, {"floppy_disk", u"\U0001F4BE"}, {"cd", u"\U0001F4BF"},
    {"package", u"\U0001F4E6"}, {"inbox_tray", u"\U0001F4E5"}, {"outbox_tray", u"\U0001F4E4"},
    {"email", u"\U0001F4E7"}, {"envelope", u"\u2709\uFE0F"}, {"file_folder", u"\U0001F4C1"},
    {"open_file_folder", u"\U0001F4C2"}, {"page_facing_up", u"\U0001F4C4"}, {"clipboard", u"\U0001F4CB"},
    {"chart_with_upwards_trend", u"\U0001F4C8"}, {"chart_with_downwards_trend", u"\U0001F4C9"},
    {"bar_chart", u"\U0001F4CA"}, {"construction", u"\U0001F6A7"}, {"triangular_flag_on_post", u"\U0001F6A9"},
    {"checkered_flag", u"\U0001F3C1"}, {"art", u"\U0001F3A8"}, {"lipstick", u"\U0001F484"},
    {"globe_with_meridians", u"\U0001F310"}, {"earth_africa", u"\U0001F30D"}, {"house", u"\U0001F3E0"},
    {"moneybag", u"\U0001F4B0"}, {"dollar", u"\U0001F4B5"}, {"credit_card", u"\U0001F4B3"},
    {"speech_balloon", u"\U0001F4AC"}, {"thought_balloon", u"\U0001F4AD"}, {"loudspeaker", u"\U0001F4E2"},
    {"mega", u"\U0001F4E3"}, {"mute", u"\U0001F507"}, {"musical_note", u"\U0001F3B5"},
    {"camera", u"\U0001F4F7"}, {"movie_camera", u"\U0001F3A5"}, {"video_game", u"\U0001F3AE"},

    // Nature, weather, food
    {"sunny", u"\u2600\uFE0F"}, {"cloud", u"\u2601\uFE0F"}, {"umbrella", u"\u2614"},
    {"snowflake", u"\u2744\uFE0F"}, {"rainbow", u"\U0001F308"}, {"crescent_moon", u"\U0001F319"},
    {"seedling", u"\U0001F331"}, {"evergreen_tree", u"\U0001F332"}, {"deciduous_tree", u"\U0001F333"},
    {"cactus", u"\U0001F335"}, {"herb", u"\U0001F33F"}, {"four_leaf_clover", u"\U0001F340"},
    {"rose", u"\U0001F339"}, {"sunflower", u"\U0001F33B"}, {"cherry_blossom", u"\U0001F338"},
    {"apple", u"\U0001F34E"}, {"coffee", u"\u2615"}, {"tea", u"\U0001F375"},
    {"pizza", u"\U0001F355"}, {"cake", u"\U0001F370"}, {"beer", u"\U0001F37A"},
    {"cat", u"\U0001F431"}, {"dog", u"\U0001F436"}, {"snake", u"\U0001F40D"},
    {"bee", u"\U0001F41D"}, {"turtle", u"\U0001F422"}, {"penguin", u"\U0001F427"},
    {"unicorn", u"\U0001F984"}, {"zzz", u"\U0001F4A4"}, {"earth_americas", u"\U0001F30E"},
};

const QHash<QString, QString> &table()
{
    static const QHash<QString, QString> map = [] {
        QHash<QString, QString> m;
        m.reserve(std::size(kEntries));
        for (const Entry &e : kEntries)
            m.insert(QString::fromLatin1(e.name), QString::fromUtf16(e.emoji));
        return m;
    }();
    return map;
}

} // namespace

std::optional<QString> emojiForShortcode(QStringView name)
{
    const auto it = table().constFind(name.toString());
    if (it == table().constEnd())
        return std::nullopt;
    return *it;
}

qsizetype emojiShortcodeCount()
{
    return table().size();
}

} // namespace mdbr::render
