#include "MainWindow.h"
#include "app/document/DocumentOpenPipeline.h"
#include "services/SupportedFileTypes.h"
#include "app/appearance/ReaderAppearanceController.h"
#include "app/appearance/ReaderAppearanceDialog.h"
#include "app/appearance/ReaderStylesheetBuilder.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMenuBar>
#include <QMimeData>
#include <QSettings>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QUrl>
#include <QDesktopServices>
#include <QVBoxLayout>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QScrollArea>

#include "PdfFilePreviewService.h"
#include "PdfSinglePagePreviewWidget.h"
#include "PdfSinglePagePreviewController.h"
#include "PdfZoomToolbar.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("MD Backpack Reader");
    resize(800, 600);

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_placeholderPage = new QWidget(this);
    auto *placeholderLayout = new QVBoxLayout(m_placeholderPage);
    auto *placeholderLabel = new QLabel("Open a Markdown file to get started.", m_placeholderPage);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLayout->addWidget(placeholderLabel);
    m_stack->addWidget(m_placeholderPage);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_stack->addWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    m_stack->setCurrentWidget(m_placeholderPage);

    auto *fileMenu = menuBar()->addMenu("&File");

    auto *openAction = fileMenu->addAction("&Open...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    fileMenu->addSeparator();

    m_recentFilesMenu = fileMenu->addMenu("Recent Files");
    m_recentFiles = loadRecentFiles();
    updateRecentFilesMenu();

    fileMenu->addSeparator();

    auto *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto *viewMenu = menuBar()->addMenu("&View");

    auto *zoomInAction = viewMenu->addAction("Zoom &In");
    zoomInAction->setShortcut(QKeySequence("Ctrl++"));
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);

    auto *zoomOutAction = viewMenu->addAction("Zoom &Out");
    zoomOutAction->setShortcut(QKeySequence("Ctrl+-"));
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);

    auto *resetZoomAction = viewMenu->addAction("&Reset Zoom");
    resetZoomAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(resetZoomAction, &QAction::triggered, this, &MainWindow::resetZoom);

    auto *settingsMenu = menuBar()->addMenu("&Settings");

    auto *readerAppearanceAction = settingsMenu->addAction("Reader &Appearance...");
    connect(readerAppearanceAction, &QAction::triggered,
            this, &MainWindow::showReaderAppearanceDialog);

    m_appearanceController = new ReaderAppearanceController(this);
    connect(m_appearanceController, &ReaderAppearanceController::appearanceChanged,
            this, &MainWindow::applyReaderAppearanceToAllBrowsers);

    statusBar()->showMessage("Ready");
}

void MainWindow::openFile()
{
    QStringList paths = QFileDialog::getOpenFileNames(
        this,
        "Open Markdown File(s)",
        QString(),
        mdbr::files::openDialogFilter());

    if (paths.isEmpty())
        return;

    for (const QString &path : paths)
        openDocumentFromPath(path);
}

static bool isSupportedFile(const QString &path)
{
    return mdbr::files::isSupported(path);
}

bool MainWindow::isPdfFile(const QString &path)
{
    return mdbr::files::isPdf(path);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return;
    const auto urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile() && isSupportedFile(url.toLocalFile())) {
            event->acceptProposedAction();
            return;
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;

    int opened = 0;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile())
            continue;
        QString path = url.toLocalFile();
        if (isSupportedFile(path)) {
            openDocumentFromPath(path);
            ++opened;
        }
    }

    if (opened == 0)
        statusBar()->showMessage("No supported files dropped");
}

void MainWindow::openDocumentsFromPaths(const QStringList &paths)
{
    int opened = 0;
    for (const QString &path : paths) {
        if (isSupportedFile(path)) {
            openDocumentFromPath(path);
            ++opened;
        }
    }
    if (opened == 0 && !paths.isEmpty())
        statusBar()->showMessage("No supported files opened");
}

QString MainWindow::tabFilePath(QWidget *tab) const
{
    if (auto *dtp = qobject_cast<DocumentTabPage *>(tab))
        return dtp->filePath();
    return tab->property("absoluteFilePath").toString();
}

void MainWindow::openDocumentFromPath(const QString &path)
{
    QString absPath = QFileInfo(path).absoluteFilePath();

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QWidget *tab = m_tabWidget->widget(i);
        QString tabPath = tabFilePath(tab);
        if (!tabPath.isEmpty() && QFileInfo(tabPath).absoluteFilePath() == absPath) {
            m_tabWidget->setCurrentIndex(i);
            statusBar()->showMessage(QString("Already open: %1").arg(path));
            addToRecentFiles(absPath);
            return;
        }
    }

    if (isPdfFile(path)) {
        openPdfDocument(path);
        return;
    }

    DocumentOpenPipeline pipeline;
    auto result = pipeline.open(absPath);

    if (!result.isValid()) {
        QMessageBox::warning(this, "Error",
            QString("Could not open:\n%1\n\n%2").arg(path, result.errorMessage));
        statusBar()->showMessage(result.errorMessage);
        return;
    }

    DocumentTabPage *tabPage = new DocumentTabPage(this);
    tabPage->setTabState(result.tabState);
    tabPage->setZoomSteps(0);
    tabPage->setOpenLinks(false);
    connect(tabPage, &QTextBrowser::anchorClicked,
            this, &MainWindow::onAnchorClicked);
    tabPage->document()->setBaseUrl(result.baseUrl);
    applyReaderAppearanceToBrowser(tabPage);

    if (result.tabState.contentMode == ContentMode::Html) {
        tabPage->setHtml(result.tabState.originalHtml);
        // The notice goes before the path so a long path cannot push it out of view.
        const QString notice = result.encodingNotice.isEmpty()
            ? QString() : QString(" [%1]").arg(result.encodingNotice);
        statusBar()->showMessage(QString("Opened%1: %2").arg(notice, path));
    } else {
        tabPage->setPlainText(result.tabState.originalPlainText);
        statusBar()->showMessage(QString("Rendered with fallback: %1").arg(path));
    }

    if (!result.renderWarning.isEmpty()) {
        QString detail = result.renderWarning;
        for (const auto &diag : result.diagnostics) {
            if (!diag.success) {
                detail += "\n\nStage: ";
                detail += renderStageName(diag.stage);
                detail += "\nError: ";
                detail += diag.warning;
                break;
            }
        }
        QMessageBox::warning(this, "Render Warning",
            QString("Failed to render:\n%1\n\n%2").arg(path, detail));
    }

    if (m_zoomSteps != 0)
        syncBrowserZoom(tabPage);

    int index = m_tabWidget->addTab(tabPage, result.fileName);
    m_tabWidget->setTabToolTip(index, path);
    m_tabWidget->setCurrentIndex(index);

    addToRecentFiles(absPath);

    updateEmptyState();
}

void MainWindow::syncBrowserZoom(DocumentTabPage *tabPage) const
{
    if (!tabPage)
        return;
    int current = tabPage->zoomSteps();
    int delta = m_zoomSteps - current;
    if (delta > 0)
        tabPage->zoomIn(delta);
    else if (delta < 0)
        tabPage->zoomOut(-delta);
    tabPage->setZoomSteps(m_zoomSteps);
}

void MainWindow::applyGlobalZoomToAllBrowsers()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *tabPage = qobject_cast<DocumentTabPage *>(m_tabWidget->widget(i));
        if (tabPage)
            syncBrowserZoom(tabPage);
    }
}

void MainWindow::zoomIn()
{
    if (m_zoomSteps < 20) {
        ++m_zoomSteps;
        applyGlobalZoomToAllBrowsers();
        statusBar()->showMessage(QString("Zoom: %1%").arg(100 + m_zoomSteps * 10));
    }
}

void MainWindow::zoomOut()
{
    if (m_zoomSteps > -5) {
        --m_zoomSteps;
        applyGlobalZoomToAllBrowsers();
        statusBar()->showMessage(QString("Zoom: %1%").arg(100 + m_zoomSteps * 10));
    }
}

void MainWindow::resetZoom()
{
    m_zoomSteps = 0;
    applyGlobalZoomToAllBrowsers();
    statusBar()->showMessage("Zoom reset");
}

void MainWindow::addToRecentFiles(const QString &absPath)
{
    m_recentFiles.removeAll(absPath);
    m_recentFiles.prepend(absPath);
    while (m_recentFiles.size() > MaxRecentFiles)
        m_recentFiles.removeLast();
    saveRecentFiles(m_recentFiles);
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
    m_recentFilesMenu->clear();
    if (m_recentFiles.isEmpty()) {
        auto *noItems = m_recentFilesMenu->addAction("No recent files");
        noItems->setEnabled(false);
        return;
    }
    m_recentFilesMenu->setEnabled(true);
    for (const QString &path : m_recentFiles) {
        auto *action = m_recentFilesMenu->addAction(path);
        action->setData(path);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    }
    m_recentFilesMenu->addSeparator();
    auto *clearAction = m_recentFilesMenu->addAction("Clear Recent Files");
    connect(clearAction, &QAction::triggered, this, &MainWindow::clearRecentFiles);
}

QStringList MainWindow::loadRecentFiles() const
{
    QSettings settings;
    return settings.value("recentFiles").toStringList();
}

void MainWindow::saveRecentFiles(const QStringList &files)
{
    QSettings settings;
    settings.setValue("recentFiles", files);
}

void MainWindow::openRecentFile()
{
    auto *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;

    const QString path = action->data().toString();
    const QFileInfo info(path);

    if (!info.exists() || !info.isFile()) {
        m_recentFiles.removeAll(path);
        saveRecentFiles(m_recentFiles);
        updateRecentFilesMenu();
        statusBar()->showMessage("Recent file no longer exists and was removed", 5000);
        return;
    }

    if (!isSupportedFile(path)) {
        m_recentFiles.removeAll(path);
        saveRecentFiles(m_recentFiles);
        updateRecentFilesMenu();
        statusBar()->showMessage("Unsupported file type was removed from Recent Files", 5000);
        return;
    }

    if (!path.isEmpty())
        openDocumentFromPath(path);
}

void MainWindow::clearRecentFiles()
{
    m_recentFiles.clear();
    saveRecentFiles(m_recentFiles);
    updateRecentFilesMenu();
}

void MainWindow::applyReaderAppearanceToBrowser(DocumentTabPage *tabPage) const
{
    if (!tabPage)
        return;
    const auto &settings = m_appearanceController->settings();
    tabPage->setStyleSheet(ReaderStylesheetBuilder::buildWidgetStylesheet(settings));
    tabPage->document()->setDefaultStyleSheet(
        ReaderStylesheetBuilder::buildDocumentStylesheet(settings));

    QString filePath = tabPage->filePath();
    if (!filePath.isEmpty())
        tabPage->document()->setBaseUrl(
            QUrl::fromLocalFile(QFileInfo(filePath).absolutePath() + '/'));

    if (tabPage->contentMode() == ContentMode::Html) {
        QString html = tabPage->originalHtml();
        if (html.isEmpty())
            return;
        auto *scrollBar = tabPage->verticalScrollBar();
        int scrollPos = scrollBar ? scrollBar->value() : 0;
        auto cursor = tabPage->textCursor();
        int cursorPos = cursor.position();

        tabPage->setHtml(html);

        if (scrollBar)
            scrollBar->setValue(scrollPos);
        cursor.setPosition(cursorPos);
        tabPage->setTextCursor(cursor);
    }
}

void MainWindow::applyReaderAppearanceToAllBrowsers()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *tabPage = qobject_cast<DocumentTabPage *>(m_tabWidget->widget(i));
        if (tabPage)
            applyReaderAppearanceToBrowser(tabPage);
    }
}

void MainWindow::showReaderAppearanceDialog()
{
    ReaderAppearanceDialog dialog(m_appearanceController->settings(), this);

    connect(&dialog, &ReaderAppearanceDialog::settingsApplied,
            m_appearanceController, &ReaderAppearanceController::applySettings);

    if (dialog.exec() == QDialog::Accepted)
        m_appearanceController->applySettings(dialog.settings());
}

void MainWindow::closeTab(int index)
{
    QWidget *page = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    delete page;

    if (m_tabWidget->count() > 0) {
        updateStatusBarForTab(m_tabWidget->currentIndex());
    } else {
        setStatusBarReady();
    }

    updateEmptyState();
}

void MainWindow::onCurrentTabChanged(int index)
{
    if (index >= 0) {
        updateStatusBarForTab(index);
    } else {
        setStatusBarReady();
    }
}

void MainWindow::updateStatusBarForTab(int index)
{
    QWidget *page = m_tabWidget->widget(index);
    if (!page)
        return;

    auto *tabPage = qobject_cast<DocumentTabPage *>(page);
    if (!tabPage) return;
    QString path = tabPage->filePath();
    if (!path.isEmpty()) {
        statusBar()->showMessage(QString("Opened: %1").arg(path));
    }
}

void MainWindow::setStatusBarReady()
{
    statusBar()->showMessage("Ready");
}

void MainWindow::onAnchorClicked(const QUrl &url)
{
    if (url.scheme() == QStringLiteral("http")
        || url.scheme() == QStringLiteral("https")
        || url.scheme() == QStringLiteral("mailto")) {
        if (!QDesktopServices::openUrl(url))
            statusBar()->showMessage(QString("Could not open: %1").arg(url.toString()));
    }
}

void MainWindow::openPdfDocument(const QString &path)
{
    QString absPath = QFileInfo(path).absoluteFilePath();

    PdfFilePreviewService service;
    auto result = service.renderFilePage(absPath);

    auto *container = new QWidget;
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(2);

    auto *toolbar = new PdfZoomToolbar;
    containerLayout->addWidget(toolbar);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(false);
    scrollArea->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(scrollArea);

    container->setProperty("absoluteFilePath", absPath);
    container->setProperty("contentType", "pdf");
    container->setProperty("recentEligible", result.ok());

    auto *previewWidget = new PdfSinglePagePreviewWidget;
    scrollArea->setWidget(previewWidget);

    auto *controller = new PdfSinglePagePreviewController(previewWidget, container);

    QObject::connect(toolbar, &PdfZoomToolbar::zoomInRequested,
                     controller, &PdfSinglePagePreviewController::zoomIn);
    QObject::connect(toolbar, &PdfZoomToolbar::zoomOutRequested,
                     controller, &PdfSinglePagePreviewController::zoomOut);
    QObject::connect(toolbar, &PdfZoomToolbar::zoomResetRequested,
                     controller, &PdfSinglePagePreviewController::zoomReset);
    QObject::connect(controller, &PdfSinglePagePreviewController::zoomChanged,
                     toolbar, &PdfZoomToolbar::setZoom);

    if (!result.image.isNull()) {
        previewWidget->setImage(result.image);
        previewWidget->resize(previewWidget->sizeHint());
    } else {
        previewWidget->setStatusText(result.diagnosticSummary);
        toolbar->setEnabled(false);
    }

    toolbar->setZoom(previewWidget->zoomFactor());

    int index = m_tabWidget->addTab(container, QFileInfo(path).fileName());
    m_tabWidget->setTabToolTip(index, absPath);
    m_tabWidget->setCurrentIndex(index);
    updateEmptyState();

    if (result.ok()) {
        statusBar()->showMessage(QString("Opened: %1").arg(path));
        addToRecentFiles(absPath);
    } else {
        statusBar()->showMessage(result.diagnosticSummary);
    }
}

void MainWindow::updateEmptyState()
{
    if (m_tabWidget->count() == 0) {
        m_stack->setCurrentWidget(m_placeholderPage);
    } else {
        m_stack->setCurrentWidget(m_tabWidget);
    }
}
