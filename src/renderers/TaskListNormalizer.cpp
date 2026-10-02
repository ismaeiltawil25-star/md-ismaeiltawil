#include "TaskListNormalizer.h"

#include <QRegularExpression>

QString TaskListNormalizer::normalizeTaskLists(const QString &html)
{
    static const QRegularExpression inputTagRx(QStringLiteral("<input\\b[^>]*>"));

    QString result;
    int lastEnd = 0;
    auto it = inputTagRx.globalMatch(html);
    while (it.hasNext()) {
        auto match = it.next();
        result += html.mid(lastEnd, match.capturedStart() - lastEnd);

        const QString tag = match.captured();
        const bool hasTypeCheckbox = tag.contains(QStringLiteral("type=\"checkbox\""));
        const bool hasTaskClass = tag.contains(QStringLiteral("class=\"task-list-item-checkbox\""));
        const bool hasDisabled = tag.contains(QStringLiteral("disabled"));

        if (hasTypeCheckbox && hasTaskClass && hasDisabled) {
            result += tag.contains(QStringLiteral("checked"))
                          ? QStringLiteral(u"☑ ")  // ☑ BALLOT BOX WITH CHECK
                          : QStringLiteral(u"☐ "); // ☐ BALLOT BOX
        } else {
            result += tag;
        }

        lastEnd = match.capturedEnd();
    }
    result += html.mid(lastEnd);
    return result;
}
