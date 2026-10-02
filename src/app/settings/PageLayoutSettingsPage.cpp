#include "PageLayoutSettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace mdbr::settings {

using namespace layout;

PageLayoutSettingsPage::PageLayoutSettingsPage(const PageLayoutSettings &current, ApplyFn onApply,
                                               QWidget *parent)
    : SettingsPage(parent)
    , m_onApply(std::move(onApply))
{
    m_pageView = new QCheckBox(tr("Show documents as printed pages (page view)"), this);

    auto *hint = new QLabel(tr("Text is split into pages as it will print, with visible gaps between "
                               "pages. Paper and margins below are also used for printing."), this);
    hint->setWordWrap(true);
    hint->setEnabled(false); // muted

    m_paper = new QComboBox(this);
    for (const PaperSize paper : {PaperSize::A4, PaperSize::A5, PaperSize::Letter, PaperSize::Legal})
        m_paper->addItem(paperLabel(paper), static_cast<int>(paper));

    m_margins = new QComboBox(this);
    for (const PageMargins margins : {PageMargins::Narrow, PageMargins::Moderate, PageMargins::Normal,
                                      PageMargins::Wide})
        m_margins->addItem(marginsLabel(margins), static_cast<int>(margins));

    auto *form = new QFormLayout;
    form->addRow(tr("Paper size:"), m_paper);
    form->addRow(tr("Margins:"), m_margins);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pageView);
    layout->addWidget(hint);
    layout->addLayout(form);
    layout->addStretch();

    load(current);
}

PageLayoutSettings PageLayoutSettingsPage::settings() const
{
    PageLayoutSettings s;
    s.pageViewEnabled = m_pageView->isChecked();
    s.paper = static_cast<PaperSize>(m_paper->currentData().toInt());
    s.margins = static_cast<PageMargins>(m_margins->currentData().toInt());
    return s;
}

void PageLayoutSettingsPage::load(const PageLayoutSettings &s)
{
    m_pageView->setChecked(s.pageViewEnabled);
    m_paper->setCurrentIndex(m_paper->findData(static_cast<int>(s.paper)));
    m_margins->setCurrentIndex(m_margins->findData(static_cast<int>(s.margins)));
}

void PageLayoutSettingsPage::apply()
{
    if (m_onApply)
        m_onApply(settings());
}

void PageLayoutSettingsPage::restoreDefaults()
{
    load(PageLayoutSettings{});
}

} // namespace mdbr::settings
