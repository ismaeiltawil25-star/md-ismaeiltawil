#ifndef DOCUMENTTABPAGE_H
#define DOCUMENTTABPAGE_H

#include <QTextBrowser>
#include <QTextFormat>
#include "DocumentTabState.h"
#include "app/layout/PageGeometry.h"
#include "app/layout/PageLayoutSettings.h"
#include "app/layout/PageViewPalette.h"

// DocumentTabPage — the reader view for Markdown and text documents.
//
// Page view (optional, see mdbr::layout::PageLayoutSettings):
//   * the QTextDocument is given a real page size, so Qt's layout engine
//     paginates exactly as it does when printing (lines never cross a page
//     boundary and never enter the margins);
//   * paintEvent draws the "desk" and one sheet per page BEHIND the text;
//   * horizontal viewport margins centre the page in wide windows.
// The text itself is painted by QTextBrowser unchanged, so selection, links
// and find keep working with no coordinate translation.
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

    // Page view. The palette is derived from the reader appearance by the caller.
    void setPageView(const mdbr::layout::PageLayoutSettings &settings,
                     const mdbr::layout::PageViewPalette &palette);
    [[nodiscard]] bool isPageViewEnabled() const { return m_pageLayout.pageViewEnabled; }
    [[nodiscard]] const mdbr::layout::PageGeometry &pageGeometry() const { return m_geometry; }
    [[nodiscard]] int pageCount() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void applyDocumentGeometry();
    void updateHorizontalCentering();
    void paintPages(const QRect &dirtyRect);

    DocumentTabState m_state;
    mdbr::layout::PageLayoutSettings m_pageLayout;
    mdbr::layout::PageViewPalette m_palette;
    mdbr::layout::PageGeometry m_geometry;
    QTextFrameFormat m_originalRootFormat; // restored when page view is turned off
    int m_centeringMargin = 0;
    bool m_applyingGeometry = false;
    bool m_wasShown = false;
};

#endif
