#include "LatexMathConverter.h"

#include <QHash>
#include <QStringList>

#include <initializer_list>
#include <utility>

namespace mdbr::render {

namespace {

// Nested groups deeper than this are emitted as escaped source instead of being
// parsed, so hostile input like "{{{{..." can never exhaust the stack.
constexpr int kMaxDepth = 64;

const QString kThinSpace = QStringLiteral(u"\u2009");

[[nodiscard]] QString escapeHtml(QStringView text)
{
    QString out;
    out.reserve(text.size());
    for (const QChar c : text) {
        switch (c.unicode()) {
        case u'<': out += QStringLiteral("&lt;"); break;
        case u'>': out += QStringLiteral("&gt;"); break;
        case u'&': out += QStringLiteral("&amp;"); break;
        case u'"': out += QStringLiteral("&quot;"); break;
        default:   out += c;
        }
    }
    return out;
}

using SymbolList = std::initializer_list<std::pair<const char *, const char16_t *>>;

[[nodiscard]] QHash<QString, QString> buildTable(SymbolList entries)
{
    QHash<QString, QString> table;
    table.reserve(qsizetype(entries.size()));
    for (const auto &[name, value] : entries)
        table.insert(QString::fromLatin1(name), QString::fromUtf16(value));
    return table;
}

const QHash<QString, QString> &symbols()
{
    static const QHash<QString, QString> table = buildTable({
        // Greek, lower case
        {"alpha", u"\u03B1"}, {"beta", u"\u03B2"}, {"gamma", u"\u03B3"}, {"delta", u"\u03B4"},
        {"epsilon", u"\u03F5"}, {"varepsilon", u"\u03B5"}, {"zeta", u"\u03B6"}, {"eta", u"\u03B7"},
        {"theta", u"\u03B8"}, {"vartheta", u"\u03D1"}, {"iota", u"\u03B9"}, {"kappa", u"\u03BA"},
        {"lambda", u"\u03BB"}, {"mu", u"\u03BC"}, {"nu", u"\u03BD"}, {"xi", u"\u03BE"},
        {"pi", u"\u03C0"}, {"varpi", u"\u03D6"}, {"rho", u"\u03C1"}, {"varrho", u"\u03F1"},
        {"sigma", u"\u03C3"}, {"varsigma", u"\u03C2"}, {"tau", u"\u03C4"}, {"upsilon", u"\u03C5"},
        {"phi", u"\u03D5"}, {"varphi", u"\u03C6"}, {"chi", u"\u03C7"}, {"psi", u"\u03C8"},
        {"omega", u"\u03C9"},
        // Greek, upper case
        {"Gamma", u"\u0393"}, {"Delta", u"\u0394"}, {"Theta", u"\u0398"}, {"Lambda", u"\u039B"},
        {"Xi", u"\u039E"}, {"Pi", u"\u03A0"}, {"Sigma", u"\u03A3"}, {"Upsilon", u"\u03A5"},
        {"Phi", u"\u03A6"}, {"Psi", u"\u03A8"}, {"Omega", u"\u03A9"},
        // Big operators
        {"sum", u"\u2211"}, {"prod", u"\u220F"}, {"coprod", u"\u2210"}, {"int", u"\u222B"},
        {"iint", u"\u222C"}, {"iiint", u"\u222D"}, {"oint", u"\u222E"}, {"bigcup", u"\u22C3"},
        {"bigcap", u"\u22C2"},
        // Binary operators
        {"pm", u" \u00B1 "}, {"mp", u" \u2213 "}, {"times", u" \u00D7 "}, {"div", u" \u00F7 "},
        {"cdot", u" \u00B7 "}, {"ast", u"\u2217"}, {"star", u"\u22C6"}, {"circ", u"\u2218"},
        {"bullet", u"\u2219"}, {"oplus", u" \u2295 "}, {"otimes", u" \u2297 "}, {"cup", u" \u222A "},
        {"cap", u" \u2229 "}, {"setminus", u" \u2216 "}, {"wedge", u" \u2227 "}, {"vee", u" \u2228 "},
        {"land", u" \u2227 "}, {"lor", u" \u2228 "},
        // Relations
        {"le", u" \u2264 "}, {"leq", u" \u2264 "}, {"ge", u" \u2265 "}, {"geq", u" \u2265 "},
        {"neq", u" \u2260 "}, {"ne", u" \u2260 "}, {"approx", u" \u2248 "}, {"equiv", u" \u2261 "},
        {"sim", u" \u223C "}, {"simeq", u" \u2243 "}, {"cong", u" \u2245 "}, {"propto", u" \u221D "},
        {"ll", u" \u226A "}, {"gg", u" \u226B "}, {"in", u" \u2208 "}, {"notin", u" \u2209 "},
        {"ni", u" \u220B "}, {"subset", u" \u2282 "}, {"subseteq", u" \u2286 "}, {"supset", u" \u2283 "},
        {"supseteq", u" \u2287 "}, {"perp", u" \u22A5 "}, {"parallel", u" \u2225 "}, {"mid", u" \u2223 "},
        // Arrows
        {"to", u" \u2192 "}, {"rightarrow", u" \u2192 "}, {"leftarrow", u" \u2190 "}, {"gets", u" \u2190 "},
        {"leftrightarrow", u" \u2194 "}, {"Rightarrow", u" \u21D2 "}, {"Leftarrow", u" \u21D0 "},
        {"Leftrightarrow", u" \u21D4 "}, {"implies", u" \u27F9 "}, {"iff", u" \u27FA "},
        {"mapsto", u" \u21A6 "}, {"uparrow", u"\u2191"}, {"downarrow", u"\u2193"},
        {"longrightarrow", u" \u27F6 "}, {"longleftarrow", u" \u27F5 "},
        // Misc symbols
        {"infty", u"\u221E"}, {"partial", u"\u2202"}, {"nabla", u"\u2207"}, {"forall", u"\u2200"},
        {"exists", u"\u2203"}, {"nexists", u"\u2204"}, {"neg", u"\u00AC"}, {"lnot", u"\u00AC"},
        {"emptyset", u"\u2205"}, {"varnothing", u"\u2205"}, {"angle", u"\u2220"}, {"triangle", u"\u25B3"},
        {"degree", u"\u00B0"}, {"prime", u"\u2032"}, {"hbar", u"\u210F"}, {"ell", u"\u2113"},
        {"Re", u"\u211C"}, {"Im", u"\u2111"}, {"aleph", u"\u2135"}, {"wp", u"\u2118"},
        {"therefore", u" \u2234 "}, {"because", u" \u2235 "}, {"top", u"\u22A4"}, {"bot", u"\u22A5"},
        {"ldots", u"\u2026"}, {"dots", u"\u2026"}, {"cdots", u"\u22EF"}, {"vdots", u"\u22EE"},
        {"ddots", u"\u22F1"}, {"langle", u"\u27E8"}, {"rangle", u"\u27E9"}, {"lfloor", u"\u230A"},
        {"rfloor", u"\u230B"}, {"lceil", u"\u2308"}, {"rceil", u"\u2309"}, {"vert", u"|"},
        {"Vert", u"\u2016"}, {"lbrace", u"{"}, {"rbrace", u"}"}, {"backslash", u"\\"},
        // Spacing
        {"quad", u"\u2003"}, {"qquad", u"\u2003\u2003"},
    });
    return table;
}

const QHash<QString, QString> &doubleStruck()
{
    static const QHash<QString, QString> table = buildTable({
        {"R", u"\u211D"}, {"N", u"\u2115"}, {"Z", u"\u2124"}, {"Q", u"\u211A"},
        {"C", u"\u2102"}, {"P", u"\u2119"}, {"H", u"\u210D"},
    });
    return table;
}

bool isFunctionName(const QString &name)
{
    static const QStringList names = {
        "sin", "cos", "tan", "cot", "sec", "csc", "arcsin", "arccos", "arctan", "sinh", "cosh",
        "tanh", "log", "ln", "lg", "exp", "lim", "max", "min", "sup", "inf", "det", "gcd",
        "deg", "arg", "dim", "ker", "hom", "Pr", "mod",
    };
    return names.contains(name);
}

class Parser
{
public:
    explicit Parser(QStringView source) : m_src(source) {}

    QString parseAll()
    {
        QString out;
        while (!atEnd())
            out += parseAtom();
        return out;
    }

private:
    [[nodiscard]] bool atEnd() const { return m_pos >= m_src.size(); }
    [[nodiscard]] QChar peek() const { return atEnd() ? QChar() : m_src[m_pos]; }

    void skipSpaces()
    {
        while (!atEnd() && m_src[m_pos].isSpace())
            ++m_pos;
    }

    QString parseAtom()
    {
        const QChar c = m_src[m_pos];
        if (c.isSpace()) {
            ++m_pos; // math mode ignores source whitespace
            return {};
        }
        switch (c.unicode()) {
        case u'{': return parseGroup();
        case u'}': ++m_pos; return {}; // stray closer
        case u'^': ++m_pos; return QStringLiteral("<sup>") + parseArgument() + QStringLiteral("</sup>");
        case u'_': ++m_pos; return QStringLiteral("<sub>") + parseArgument() + QStringLiteral("</sub>");
        case u'\\': return parseCommand();
        case u'&': ++m_pos; return QStringLiteral("\u2003");          // alignment column
        case u'~': ++m_pos; return QStringLiteral("\u00A0");
        case u'=': case u'+': case u'<': case u'>':
            ++m_pos; return kThinSpace + escapeHtml(QStringView(&c, 1)) + kThinSpace;
        case u'-': ++m_pos; return kThinSpace + QStringLiteral("\u2212") + kThinSpace;
        case u'*': ++m_pos; return QStringLiteral("\u2217");
        case u'\'': ++m_pos; return QStringLiteral("\u2032");
        default:
            ++m_pos;
            return escapeHtml(QStringView(&c, 1));
        }
    }

    // One argument: a {group}, a \command, or a single character.
    QString parseArgument()
    {
        skipSpaces();
        if (atEnd())
            return {};
        if (peek() == u'{')
            return parseGroup();
        if (peek() == u'\\')
            return parseCommand();
        const QChar c = m_src[m_pos++];
        return escapeHtml(QStringView(&c, 1));
    }

    QString parseGroup()
    {
        ++m_pos; // '{'
        if (++m_depth > kMaxDepth) {
            // Too deep: show the rest literally instead of recursing further.
            QString rest = escapeHtml(m_src.sliced(m_pos));
            m_pos = m_src.size();
            --m_depth;
            return rest;
        }
        QString out;
        while (!atEnd() && peek() != u'}')
            out += parseAtom();
        if (!atEnd())
            ++m_pos; // '}'
        --m_depth;
        return out;
    }

    // Raw text of a {group} (for \text and friends), without interpretation.
    QString rawGroup()
    {
        skipSpaces();
        if (peek() != u'{')
            return parseArgument();
        ++m_pos;
        const qsizetype start = m_pos;
        int depth = 1;
        while (!atEnd()) {
            if (m_src[m_pos] == u'{')
                ++depth;
            else if (m_src[m_pos] == u'}' && --depth == 0)
                break;
            ++m_pos;
        }
        const QString text = escapeHtml(m_src.sliced(start, m_pos - start));
        if (!atEnd())
            ++m_pos;
        return text;
    }

    QString readName()
    {
        const qsizetype start = m_pos;
        while (!atEnd() && m_src[m_pos].isLetter())
            ++m_pos;
        return m_src.sliced(start, m_pos - start).toString();
    }

    QString parseCommand()
    {
        ++m_pos; // '\'
        if (atEnd())
            return QStringLiteral("\\");

        const QChar c = m_src[m_pos];
        if (!c.isLetter()) {
            ++m_pos;
            switch (c.unicode()) {
            case u',': return kThinSpace;
            case u':': case u';': case u'>': return QStringLiteral("\u2005");
            case u'!': return {};
            case u' ': return QStringLiteral(" ");
            case u'\\': return QStringLiteral("<br>");
            case u'|': return QStringLiteral("\u2016");
            default: return escapeHtml(QStringView(&c, 1)); // \{ \} \% \$ \# \& \_
            }
        }

        const QString name = readName();

        if (const auto it = symbols().constFind(name); it != symbols().constEnd())
            return *it;
        if (isFunctionName(name))
            return name + kThinSpace;

        if (name == QLatin1StringView("frac") || name == QLatin1StringView("dfrac") || name == QLatin1StringView("tfrac")) {
            const QString numerator = parseArgument();
            const QString denominator = parseArgument();
            return QStringLiteral("<sup>") + numerator + QStringLiteral("</sup>\u2044<sub>")
                 + denominator + QStringLiteral("</sub>");
        }
        if (name == QLatin1StringView("sqrt")) {
            QString index;
            skipSpaces();
            if (peek() == u'[') {
                ++m_pos;
                while (!atEnd() && peek() != u']')
                    index += parseAtom();
                if (!atEnd())
                    ++m_pos;
            }
            const QString radicand = parseArgument();
            return (index.isEmpty() ? QString() : QStringLiteral("<sup>") + index + QStringLiteral("</sup>"))
                 + QStringLiteral("\u221A<span style=\"text-decoration: overline;\">") + radicand
                 + QStringLiteral("</span>");
        }
        if (name == QLatin1StringView("text") || name == QLatin1StringView("textrm") || name == QLatin1StringView("mbox") || name == QLatin1StringView("operatorname"))
            return rawGroup();
        if (name == QLatin1StringView("mathrm"))
            return parseArgument();
        if (name == QLatin1StringView("mathbf") || name == QLatin1StringView("textbf") || name == QLatin1StringView("boldsymbol") || name == QLatin1StringView("bm"))
            return QStringLiteral("<b>") + parseArgument() + QStringLiteral("</b>");
        if (name == QLatin1StringView("mathit") || name == QLatin1StringView("textit") || name == QLatin1StringView("emph"))
            return QStringLiteral("<i>") + parseArgument() + QStringLiteral("</i>");
        if (name == QLatin1StringView("mathbb")) {
            const QString letters = rawGroup();
            QString out;
            for (const QChar l : letters)
                out += doubleStruck().value(QString(l), QString(l));
            return out;
        }
        if (name == QLatin1StringView("mathcal") || name == QLatin1StringView("mathsf") || name == QLatin1StringView("mathtt") || name == QLatin1StringView("displaystyle")
            || name == QLatin1StringView("textstyle"))
            return name.startsWith(QLatin1StringView("math")) ? parseArgument() : QString();
        if (name == QLatin1StringView("overline") || name == QLatin1StringView("bar"))
            return QStringLiteral("<span style=\"text-decoration: overline;\">") + parseArgument()
                 + QStringLiteral("</span>");
        if (name == QLatin1StringView("underline"))
            return QStringLiteral("<u>") + parseArgument() + QStringLiteral("</u>");
        if (name == QLatin1StringView("hat") || name == QLatin1StringView("widehat"))   return parseArgument() + QStringLiteral("\u0302");
        if (name == QLatin1StringView("tilde") || name == QLatin1StringView("widetilde")) return parseArgument() + QStringLiteral("\u0303");
        if (name == QLatin1StringView("vec"))                          return parseArgument() + QStringLiteral("\u20D7");
        if (name == QLatin1StringView("dot"))                          return parseArgument() + QStringLiteral("\u0307");
        if (name == QLatin1StringView("ddot"))                         return parseArgument() + QStringLiteral("\u0308");
        if (name == QLatin1StringView("left") || name == QLatin1StringView("right") || name.startsWith(QLatin1StringView("big")) || name.startsWith(QLatin1StringView("Big"))) {
            skipSpaces();
            if (peek() == u'.') {
                ++m_pos; // invisible delimiter
                return {};
            }
            return atEnd() ? QString() : parseArgument();
        }
        if (name == QLatin1StringView("begin") || name == QLatin1StringView("end")) {
            rawGroup(); // environment name: matrices, cases, aligned... are shown as rows
            return name == QLatin1StringView("end") ? QStringLiteral(" ") : QString();
        }

        return escapeHtml(QStringLiteral("\\") + name); // unknown: keep it visible
    }

    QStringView m_src;
    qsizetype m_pos = 0;
    int m_depth = 0;
};

} // namespace

QString latexToHtml(QStringView latex)
{
    return Parser(latex).parseAll();
}

} // namespace mdbr::render
