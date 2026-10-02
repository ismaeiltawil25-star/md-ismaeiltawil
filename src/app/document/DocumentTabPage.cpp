#include "DocumentTabPage.h"

#include <QAbstractTextDocumentLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QTextFrame>
#include <QtMath>

using namespace mdbr::layout;

namespace {
// Marks a root frame format that page view has modified, so the original
// (margins, body background) can be restored when page view is turned off.
constexpr int kPageViewFormatMarker = QTextFormat::UserProperty + 1;
} // namespace

DocumentTabPage::DocumentTabPage(QWidget *parent)
    : QTextBrowser(parent)
{
    // setHtml() / setPlainText() rebuild the root frame (margins, background),
    // so the page geometry is re-applied whenever the content changes.
    connect(this, &QTextEdit::textChanged, this, &DocumentTabPage::applyDocumentGeometry);

    // Incremental layout adds pages after the first paint; repaint the sheets.
    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::pageCountChanged,
            this, [this] {
                if (isPageViewEnabled())
                    viewport()->update();
            });
}

void DocumentTabPage::setPageView(const PageLayoutSettings &settings, const PageViewPalette &palette)
{
    const bool wasEnabled = m_pageLayout.pageViewEnabled;
    m_pageLayout = settings;
    m_palette = palette;
    m_geometry = settings.pageViewEnabled ? computePageGeometry(settings, logicalDpiY())
                                          : PageGeometry{};

    if (settings.pageViewEnabled) {
        // A fixed wrap width stops QTextEdit from resetting the document page
        // size (to "viewport width × unlimited height") on every resize.
        setLineWrapMode(QTextEdit::FixedPixelWidth);
        setLineWrapColumnOrWidth(qCeil(m_geometry.pageSize.width()));
    } else if (wasEnabled) {
        setLineWrapMode(QTextEdit::WidgetWidth);
    }

    applyDocumentGeometry();
    updateHorizontalCentering();
    viewport()->update();
}

int DocumentTabPage::pageCount() const
{
    return isPageViewEnabled() ? document()->pageCount() : 1;
}

void DocumentTabPage::applyDocumentGeometry()
{
    if (m_applyingGeometry)
        return; // changing the frame format emits textChanged again
    const QScopedValueRollback guard(m_applyingGeometry, true);

    QTextDocument *doc = document();
    QTextFrame *root = doc->rootFrame();
    QTextFrameFormat format = root->frameFormat();
    const bool isPageFormat = format.hasProperty(kPageViewFormatMarker);

    if (!isPageViewEnabled()) {
        if (isPageFormat)
            root->setFrameFormat(m_originalRootFormat);
        return;
    }

    if (!isPageFormat)
        m_originalRootFormat = format;

    const qreal margin = m_geometry.margin;
    const bool needsUpdate = !isPageFormat
        || !qFuzzyCompare(format.leftMargin(), margin)
        || !qFuzzyCompare(format.topMargin(), margin);
    if (needsUpdate) {
        format.setLeftMargin(margin);
        format.setRightMargin(margin);
        format.setTopMargin(margin);
        format.setBottomMargin(margin);
        format.setPadding(0);
        // The body background would cover the desk and the gaps between pages;
        // sheets are painted in the page colour instead (see paintPages()).
        format.clearBackground();
        format.setProperty(kPageViewFormatMarker, true);
        root->setFrameFormat(format);
    }

    if (doc->pageSize() != m_geometry.pageSize)
        doc->setPageSize(m_geometry.pageSize);
}

void DocumentTabPage::updateHorizontalCentering()
{
    int target = 0;
    if (isPageViewEnabled()) {
        const int available = viewport()->width() + 2 * m_centeringMargin;
        target = qMax(0, int((available - m_geometry.pageSize.width()) / 2));
    }
    if (target != m_centeringMargin) {
        m_centeringMargin = target;
        setViewportMargins(target, 0, target, 0);
    }
}

void DocumentTabPage::resizeEvent(QResizeEvent *event)
{
    // QTextEdit re-lays out the document on resize and resets its page size to
    // "wrap width × unlimited height", which would silently turn page view back
    // into one continuous column. Restore the page size right after.
    QTextBrowser::resizeEvent(event);
    applyDocumentGeometry();
    updateHorizontalCentering();
}

void DocumentTabPage::showEvent(QShowEvent *event)
{
    QTextBrowser::showEvent(event);
    applyDocumentGeometry();
    updateHorizontalCentering();

    // Content is set while the widget is still tiny and hidden; QTextEdit then
    // scrolls to "show" the cursor, which in page view sits below the top
    // margin. On first show, a document opened at its start begins at the top.
    if (!m_wasShown) {
        m_wasShown = true;
        if (textCursor().position() == 0)
            verticalScrollBar()->setValue(0);
    }
}

void DocumentTabPage::paintEvent(QPaintEvent *event)
{
    if (isPageViewEnabled()) {
        // Safety net: if anything else reset the page size, restore it on the
        // next event-loop turn (never change the layout while painting).
        if (document()->pageSize() != m_geometry.pageSize)
            QMetaObject::invokeMethod(this, &DocumentTabPage::applyDocumentGeometry, Qt::QueuedConnection);
        paintPages(event->rect());
    }
    QTextBrowser::paintEvent(event); // text is drawn on top of the sheets
}

void DocumentTabPage::paintPages(const QRect &dirtyRect)
{
    QPainter painter(viewport());
    painter.fillRect(dirtyRect, m_palette.desk);

    const qreal pageHeight = m_geometry.pageSize.height();
    if (pageHeight <= 0)
        return;

    const QPointF offset(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
    // Qt lays out long documents incrementally, so QTextDocument::pageCount()
    // can lag behind what is on screen. Also derive the page count from the
    // scrollable height, so sheets are never missing under visible text.
    const qreal contentHeight = qMax(document()->documentLayout()->documentSize().height(),
                                     qreal(verticalScrollBar()->maximum() + viewport()->height()));
    const int pagesFromHeight = qCeil(contentHeight / pageHeight);
    const int lastPage = qMax(pageCount(), pagesFromHeight) - 1;
    const int first = qBound(0, int((dirtyRect.top() - offset.y()) / pageHeight), lastPage);
    const int last = qBound(0, int((dirtyRect.bottom() - offset.y()) / pageHeight), lastPage);

    painter.setPen(QPen(m_palette.border, 1));
    for (int i = first; i <= last; ++i) {
        const QRectF sheet(offset.x(), offset.y() + m_geometry.sheetTop(i),
                           m_geometry.pageSize.width(), m_geometry.sheetHeight());
        painter.fillRect(sheet.translated(2, 3), m_palette.shadow);
        painter.fillRect(sheet, m_palette.page);
        painter.drawRect(sheet.adjusted(0, 0, -1, -1));
    }
}
