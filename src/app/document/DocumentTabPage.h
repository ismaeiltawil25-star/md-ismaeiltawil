#ifndef DOCUMENTTABPAGE_H
#define DOCUMENTTABPAGE_H

#include <QTextBrowser>
#include "DocumentTabState.h"

class DocumentTabPage : public QTextBrowser
{
    Q_OBJECT

public:
    explicit DocumentTabPage(QWidget *parent = nullptr);

    const DocumentTabState &tabState() const { return m_state; }
    void setTabState(const DocumentTabState &state) { m_state = state; }

    QString filePath() const { return m_state.filePath; }
    void setFilePath(const QString &path) { m_state.filePath = path; }

    int zoomSteps() const { return m_state.zoomSteps; }
    void setZoomSteps(int steps) { m_state.zoomSteps = steps; }

    ContentMode contentMode() const { return m_state.contentMode; }
    void setContentMode(ContentMode mode) { m_state.contentMode = mode; }

    QString originalHtml() const { return m_state.originalHtml; }
    void setOriginalHtml(const QString &html) { m_state.originalHtml = html; }

    QString originalPlainText() const { return m_state.originalPlainText; }
    void setOriginalPlainText(const QString &text) { m_state.originalPlainText = text; }

private:
    DocumentTabState m_state;
};

#endif
