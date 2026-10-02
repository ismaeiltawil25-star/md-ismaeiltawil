#include "PageLayoutDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace mdbr::layout {

PageLayoutDialog::PageLayoutDialog(const PageLayoutSettings &initial, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Page Layout"));

    m_pageViewCheck = new QCheckBox(QStringLiteral("Show documents as printed pages (page view)"), this);
    m_pageViewCheck->setChecked(initial.pageViewEnabled);

    auto *hint = new QLabel(QStringLiteral(
        "Text is split into pages exactly as it would print, with visible gaps between pages.\n"
        "Turn this off for one continuous column."), this);
    hint->setWordWrap(true);
    hint->setEnabled(false); // muted hint text

    m_paperCombo = new QComboBox(this);
    for (const PaperSize paper : {PaperSize::A4, PaperSize::A5, PaperSize::Letter, PaperSize::Legal})
        m_paperCombo->addItem(paperLabel(paper), static_cast<int>(paper));
    m_paperCombo->setCurrentIndex(m_paperCombo->findData(static_cast<int>(initial.paper)));

    m_marginsCombo = new QComboBox(this);
    for (const PageMargins margins : {PageMargins::Narrow, PageMargins::Moderate,
                                      PageMargins::Normal, PageMargins::Wide})
        m_marginsCombo->addItem(marginsLabel(margins), static_cast<int>(margins));
    m_marginsCombo->setCurrentIndex(m_marginsCombo->findData(static_cast<int>(initial.margins)));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Paper size:"), m_paperCombo);
    form->addRow(QStringLiteral("Margins:"), m_marginsCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_pageViewCheck);
    layout->addWidget(hint);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(m_pageViewCheck, &QCheckBox::toggled, this, &PageLayoutDialog::updateEnabledStates);
    updateEnabledStates();
}

PageLayoutSettings PageLayoutDialog::settings() const
{
    PageLayoutSettings s;
    s.pageViewEnabled = m_pageViewCheck->isChecked();
    s.paper = static_cast<PaperSize>(m_paperCombo->currentData().toInt());
    s.margins = static_cast<PageMargins>(m_marginsCombo->currentData().toInt());
    return s;
}

void PageLayoutDialog::updateEnabledStates()
{
    const bool enabled = m_pageViewCheck->isChecked();
    m_paperCombo->setEnabled(enabled);
    m_marginsCombo->setEnabled(enabled);
}

} // namespace mdbr::layout
