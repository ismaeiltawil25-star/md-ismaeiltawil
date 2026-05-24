#ifndef DOCUMENTOPENRESULT_H
#define DOCUMENTOPENRESULT_H

#include <QString>
#include <QUrl>
#include "DocumentTabState.h"

struct DocumentOpenResult
{
    DocumentTabState tabState;
    QString fileName;
    QString errorMessage;
    QString renderWarning;
    QUrl baseUrl;

    bool isValid() const { return errorMessage.isEmpty(); }
};

#endif
