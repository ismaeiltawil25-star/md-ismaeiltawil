#pragma once

// PrintController — the user-facing side of printing.
//
// Owns the "Print..." (Ctrl+P) and "Print Preview..." actions, shows Qt's
// print / preview dialogs, and delegates the actual work to DocumentPrinter.
// It does not know about tabs: MainWindow supplies the current content and
// page-layout settings through two callbacks, which keeps this class
// independent of the window structure.

#include "PrintableContent.h"
#include "app/layout/PageLayoutSettings.h"

#include <QColor>
#include <QObject>

#include <functional>

class QAction;
class QWidget;

namespace mdbr::print {

class PrintController : public QObject
{
    Q_OBJECT

public:
    using ContentProvider = std::function<PrintableContent()>;
    using SettingsProvider = std::function<layout::PageLayoutSettings()>;

    PrintController(QWidget *window, ContentProvider content, SettingsProvider settings);

    [[nodiscard]] QAction *printAction() const { return m_printAction; }
    [[nodiscard]] QAction *previewAction() const { return m_previewAction; }

public slots:
    // Enables the actions only when the current tab has something to print.
    void updateActions();
    // Redraw the toolbar icons in `color` (system light/dark switch).
    void refreshIcons(const QColor &color);
    void print();
    void preview();

signals:
    void statusMessage(const QString &message);

private:
    QWidget *m_window;
    ContentProvider m_content;
    SettingsProvider m_settings;
    QAction *m_printAction;
    QAction *m_previewAction;
};

} // namespace mdbr::print
