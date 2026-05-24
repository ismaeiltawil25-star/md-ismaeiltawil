#include "TaskListNormalizer.h"

QString TaskListNormalizer::normalizeTaskLists(const QString &html)
{
    QString out = html;
    out.replace(QStringLiteral("<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled checked>"),
                QStringLiteral("☑ "));
    out.replace(QStringLiteral("<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled>"),
                QStringLiteral("☐ "));
    return out;
}
