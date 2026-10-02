#include "PrintController.h"

#include "DocumentPrinter.h"
#include "PrintIcons.h"

#include <QAction>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QWidget>

namespace mdbr::print {

PrintController::PrintController(QWidget *window, ContentProvider content, SettingsProvider settings)
    : QObject(window)
    , m_window(window)
    , m_content(std::move(content))
    , m_settings(std::move(settings))
{
    const QColor iconColor = window->palette().color(QPalette::ButtonText);

    m_printAction = new QAction(printIcon(iconColor), tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    m_printAction->setToolTip(tr("Print the current document (Ctrl+P)"));
    connect(m_printAction, &QAction::triggered, this, &PrintController::print);

    m_previewAction = new QAction(printPreviewIcon(iconColor), tr("Print Pre&view..."), this);
    m_previewAction->setToolTip(tr("Preview the printed pages"));
    connect(m_previewAction, &QAction::triggered, this, &PrintController::preview);

    updateActions();
}

void PrintController::updateActions()
{
    const bool printable = m_content().isPrintable();
    m_printAction->setEnabled(printable);
    m_previewAction->setEnabled(printable);
}

void PrintController::print()
{
    const PrintableContent content = m_content();
    if (!content.isPrintable())
        return;
    const layout::PageLayoutSettings settings = m_settings();

    QPrinter printer(QPrinter::HighResolution);
    DocumentPrinter::configurePrinter(printer, settings);
    printer.setDocName(content.title);

    QPrintDialog dialog(&printer, m_window);
    dialog.setWindowTitle(tr("Print %1").arg(content.title));
    const int pages = DocumentPrinter::pageCount(content, settings);
    if (pages > 0)
        dialog.setMinMax(1, pages);
    dialog.setOption(QAbstractPrintDialog::PrintPageRange, pages > 1);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const PrintResult result = DocumentPrinter::print(content, printer, settings);
    if (result.ok) {
        emit statusMessage(tr("Printed %1 page(s): %2").arg(result.pagesPrinted).arg(content.title));
    } else {
        QMessageBox::warning(m_window, tr("Print"), result.error);
        emit statusMessage(result.error);
    }
}

void PrintController::preview()
{
    const PrintableContent content = m_content();
    if (!content.isPrintable())
        return;
    const layout::PageLayoutSettings settings = m_settings();

    QPrinter printer(QPrinter::HighResolution);
    DocumentPrinter::configurePrinter(printer, settings);
    printer.setDocName(content.title);

    QPrintPreviewDialog dialog(&printer, m_window);
    dialog.setWindowTitle(tr("Print Preview - %1").arg(content.title));
    // Re-run on every change in the dialog (paper size, orientation...).
    connect(&dialog, &QPrintPreviewDialog::paintRequested, this, [&](QPrinter *target) {
        const PrintResult result = DocumentPrinter::print(content, *target, settings);
        if (!result.ok)
            emit statusMessage(result.error);
    });
    dialog.resize(m_window->size() * 0.9);
    dialog.exec();
}

} // namespace mdbr::print
