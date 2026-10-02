#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>

#include "app/document/DocumentTabPage.h"
#include "app/layout/PageLayoutSettings.h"

class QStackedWidget;
class QTabWidget;
class QToolBar;
class QMenu;
class ReaderAppearanceController;

namespace mdbr::print {
class PrintController;
struct PrintableContent;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openDocumentsFromPaths(const QStringList &paths);
    static bool isPdfFile(const QString &path);

private slots:
    void openFile();
    void closeTab(int index);
    void onCurrentTabChanged(int index);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void openRecentFile();
    void clearRecentFiles();
    void showPreferences();
    void onAnchorClicked(const QUrl &url);

protected:
    void changeEvent(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void openDocumentFromPath(const QString &path);
    void openPdfDocument(const QString &path);
    QString tabFilePath(QWidget *tab) const;
    void updateEmptyState();
    void updateStatusBarForTab(int index);
    void setStatusBarReady();

    void syncBrowserZoom(DocumentTabPage *tabPage) const;
    void applyGlobalZoomToAllBrowsers();
    void applyReaderAppearanceToBrowser(DocumentTabPage *tabPage) const;
    void applyReaderAppearanceToAllBrowsers();
    mdbr::print::PrintableContent currentPrintableContent() const;
    void applyPageLayout(const mdbr::layout::PageLayoutSettings &settings);
    void setupThemeControls(QMenu *viewMenu, QToolBar *toolBar);
    void applyInteractionStyle();
    void addToRecentFiles(const QString &absPath);
    void updateRecentFilesMenu();
    QStringList loadRecentFiles() const;
    void saveRecentFiles(const QStringList &files);

    static constexpr int MaxRecentFiles = 10;

    QStackedWidget *m_stack;
    QWidget *m_placeholderPage;
    QTabWidget *m_tabWidget;
    int m_zoomSteps = 0;
    ReaderAppearanceController *m_appearanceController = nullptr;
    mdbr::layout::PageLayoutSettings m_pageLayout;
    mdbr::print::PrintController *m_printController = nullptr;
    QAction *m_nightAction = nullptr;
    QStringList m_recentFiles;
    QMenu *m_recentFilesMenu = nullptr;
};

#endif
