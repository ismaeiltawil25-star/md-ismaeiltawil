#ifndef TASKLISTNORMALIZER_H
#define TASKLISTNORMALIZER_H

#include <QString>

class TaskListNormalizer
{
public:
    static QString normalizeTaskLists(const QString &html);
};

#endif
