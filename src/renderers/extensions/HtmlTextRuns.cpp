#include "HtmlTextRuns.h"

#include <array>

namespace mdbr::render {

namespace {

constexpr std::array<QStringView, 6> kProtectedElements = {
    u"code", u"pre", u"a", u"x-equation", u"script", u"style",
};

[[nodiscard]] QStringView tagName(QStringView tag)
{
    // tag is "<name ...>" or "</name>"
    qsizetype start = 1;
    if (start < tag.size() && tag[start] == u'/')
        ++start;
    qsizetype end = start;
    while (end < tag.size() && tag[end] != u' ' && tag[end] != u'>' && tag[end] != u'/'
           && tag[end] != u'\n' && tag[end] != u'\t')
        ++end;
    return tag.sliced(start, end - start);
}

[[nodiscard]] int protectedIndex(QStringView name)
{
    for (int i = 0; i < int(kProtectedElements.size()); ++i)
        if (name.compare(kProtectedElements[i], Qt::CaseInsensitive) == 0)
            return i;
    return -1;
}

} // namespace

QString transformTextRuns(const QString &html, const TextRunTransform &transform)
{
    const QStringView view(html);
    QString out;
    out.reserve(html.size() + html.size() / 16);

    std::array<int, kProtectedElements.size()> depth{}; // open count per protected element
    int protectedDepth = 0;
    qsizetype pos = 0;

    const auto emitText = [&](QStringView text) {
        if (text.isEmpty())
            return;
        if (protectedDepth > 0)
            out += text;
        else
            out += transform(text);
    };

    while (pos < view.size()) {
        const qsizetype lt = view.indexOf(u'<', pos);
        if (lt < 0) {
            emitText(view.sliced(pos));
            break;
        }
        emitText(view.sliced(pos, lt - pos));

        const qsizetype gt = view.indexOf(u'>', lt);
        if (gt < 0) {
            out += view.sliced(lt); // malformed tail: copy verbatim
            break;
        }

        const QStringView tag = view.sliced(lt, gt - lt + 1);
        out += tag;
        pos = gt + 1;

        const bool isComment = tag.startsWith(u"<!");
        const bool isClosing = tag.size() > 1 && tag[1] == u'/';
        const bool isSelfClosing = tag.endsWith(u"/>");
        if (isComment || isSelfClosing)
            continue;

        const int index = protectedIndex(tagName(tag));
        if (index < 0)
            continue;
        if (isClosing) {
            if (depth[index] > 0) {
                --depth[index];
                --protectedDepth;
            }
        } else {
            ++depth[index];
            ++protectedDepth;
        }
    }

    return out;
}

} // namespace mdbr::render
