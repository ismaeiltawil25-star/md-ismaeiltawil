#include "MainWindow.h"
#include "services/DocumentLoader.h"
#include "services/MarkdownPreprocessor.h"
#include "renderers/MarkdownHtmlRenderer.h"
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
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QUrl>
#include <QDesktopServices>
#include <QVBoxLayout>

#include <QDragEnterEvent>
#include <QDropEvent>

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
        "Markdown / Text Files (*.md *.markdown *.txt);;All Files (*)");

    if (paths.isEmpty())
        return;

    for (const QString &path : paths)
        openDocumentFromPath(path);
}

static bool isSupportedFile(const QString &path)
{
    return path.endsWith(".md", Qt::CaseInsensitive)
        || path.endsWith(".markdown", Qt::CaseInsensitive)
        || path.endsWith(".txt", Qt::CaseInsensitive);
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

void MainWindow::openDocumentFromPath(const QString &path)
{
    QString absPath = QFileInfo(path).absoluteFilePath();

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QWidget *page = m_tabWidget->widget(i);
        QString tabPath = page->property("filePath").toString();
        if (!tabPath.isEmpty() && QFileInfo(tabPath).absoluteFilePath() == absPath) {
            m_tabWidget->setCurrentIndex(i);
            statusBar()->showMessage(QString("Already open: %1").arg(path));
            addToRecentFiles(absPath);
            return;
        }
    }

    auto loadResult = DocumentLoader::load(path);

    if (!loadResult.isValid()) {
        QMessageBox::warning(this, "Error",
            QString("Could not open:\n%1\n\n%2").arg(path, loadResult.errorMessage));
        statusBar()->showMessage(loadResult.errorMessage);
        return;
    }

    QString cleaned = MarkdownPreprocessor::stripYamlFrontMatter(loadResult.document.rawText);
    QString withDefinitions = MarkdownPreprocessor::convertDefinitionLists(cleaned);
    auto renderResult = MarkdownHtmlRenderer::render(withDefinitions);

    if (!renderResult.isValid()) {
        QMessageBox::warning(this, "Render Warning",
            QString("Failed to render:\n%1").arg(path));
    }

    QTextBrowser *browser = new QTextBrowser(this);
    browser->setProperty("filePath", absPath);
    browser->setProperty("zoomSteps", 0);
    browser->setOpenLinks(false);
    connect(browser, &QTextBrowser::anchorClicked,
            this, &MainWindow::onAnchorClicked);
    applyReaderAppearanceToBrowser(browser);

    QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(absPath).absolutePath() + '/');
    browser->document()->setBaseUrl(baseUrl);

    if (renderResult.isValid()) {
        browser->setProperty("contentMode", "html");
        browser->setProperty("originalHtml", renderResult.html);
        browser->setHtml(renderResult.html);
        statusBar()->showMessage(QString("Opened: %1").arg(path));
    } else {
        browser->setProperty("contentMode", "plain");
        browser->setProperty("originalPlainText", cleaned);
        browser->setPlainText(cleaned);
        statusBar()->showMessage(QString("Rendered with fallback: %1").arg(path));
    }

    if (m_zoomSteps != 0)
        syncBrowserZoom(browser);

    int index = m_tabWidget->addTab(browser, loadResult.document.fileName);
    m_tabWidget->setTabToolTip(index, path);
    m_tabWidget->setCurrentIndex(index);

    addToRecentFiles(absPath);

    updateEmptyState();
}

void MainWindow::syncBrowserZoom(QTextBrowser *browser) const
{
    if (!browser)
        return;
    int current = browser->property("zoomSteps").toInt();
    int delta = m_zoomSteps - current;
    if (delta > 0)
        browser->zoomIn(delta);
    else if (delta < 0)
        browser->zoomOut(-delta);
    browser->setProperty("zoomSteps", m_zoomSteps);
}

void MainWindow::applyGlobalZoomToAllBrowsers()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *browser = qobject_cast<QTextBrowser *>(m_tabWidget->widget(i));
        if (browser)
            syncBrowserZoom(browser);
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

void MainWindow::applyReaderAppearanceToBrowser(QTextBrowser *browser) const
{
    if (!browser)
        return;
    const auto &settings = m_appearanceController->settings();
    browser->setStyleSheet(ReaderStylesheetBuilder::buildWidgetStylesheet(settings));
    browser->document()->setDefaultStyleSheet(
        ReaderStylesheetBuilder::buildDocumentStylesheet(settings));

    QString filePath = browser->property("filePath").toString();
    if (!filePath.isEmpty())
        browser->document()->setBaseUrl(
            QUrl::fromLocalFile(QFileInfo(filePath).absolutePath() + '/'));

    QString mode = browser->property("contentMode").toString();
    if (mode == QStringLiteral("html")) {
        QString html = browser->property("originalHtml").toString();
        if (html.isEmpty())
            return;
        auto *scrollBar = browser->verticalScrollBar();
        int scrollPos = scrollBar ? scrollBar->value() : 0;
        auto cursor = browser->textCursor();
        int cursorPos = cursor.position();

        browser->setHtml(html);

        if (scrollBar)
            scrollBar->setValue(scrollPos);
        cursor.setPosition(cursorPos);
        browser->setTextCursor(cursor);
    }
}

void MainWindow::applyReaderAppearanceToAllBrowsers()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *browser = qobject_cast<QTextBrowser *>(m_tabWidget->widget(i));
        if (browser)
            applyReaderAppearanceToBrowser(browser);
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

    QString path = page->property("filePath").toString();
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

void MainWindow::updateEmptyState()
{
    if (m_tabWidget->count() == 0) {
        m_stack->setCurrentWidget(m_placeholderPage);
    } else {
        m_stack->setCurrentWidget(m_tabWidget);
    }
}
