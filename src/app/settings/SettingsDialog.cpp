#include "SettingsDialog.h"

#include "SettingsPage.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace mdbr::settings {

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    resize(680, 520);

    m_sections = new QListWidget(this);
    m_sections->setFixedWidth(160);
    m_stack = new QStackedWidget(this);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel
                                             | QDialogButtonBox::Apply | QDialogButtonBox::RestoreDefaults,
                                         this);
    buttons->button(QDialogButtonBox::RestoreDefaults)->setToolTip(tr("Reset this section to its defaults"));

    auto *body = new QHBoxLayout;
    body->addWidget(m_sections);
    body->addWidget(m_stack, 1);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(body, 1);
    layout->addWidget(buttons);

    connect(m_sections, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        applyAll();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons](QAbstractButton *button) {
        switch (buttons->standardButton(button)) {
        case QDialogButtonBox::Apply:
            applyAll();
            break;
        case QDialogButtonBox::RestoreDefaults:
            if (auto *current = qobject_cast<SettingsPage *>(m_stack->currentWidget()))
                current->restoreDefaults();
            break;
        default:
            break;
        }
    });
}

void SettingsDialog::addPage(SettingsPage *page)
{
    page->setParent(m_stack);
    m_pages.append(page);
    m_stack->addWidget(page);
    m_sections->addItem(page->title());
    if (m_sections->currentRow() < 0)
        m_sections->setCurrentRow(0);
}

void SettingsDialog::showPage(int index)
{
    m_sections->setCurrentRow(index);
}

void SettingsDialog::applyAll()
{
    for (SettingsPage *page : std::as_const(m_pages))
        page->apply();
}

} // namespace mdbr::settings
