#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>

#include "app/document/DocumentTabPage.h"
#include "app/layout/PageLayoutSettings.h"

class QStackedWidget;
class QTabWidget;
class ReaderAppearanceController;

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
    void showReaderAppearanceDialog();
    void showPageLayoutDialog();
    void onAnchorClicked(const QUrl &url);

protected:
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
    QStringList m_recentFiles;
    QMenu *m_recentFilesMenu = nullptr;
};

#endif
