#pragma once

// PageLayoutSettingsPage — page view on/off, paper size and margins.
// The same paper and margins are used for printing.

#include "SettingsPage.h"
#include "app/layout/PageLayoutSettings.h"

#include <functional>

class QCheckBox;
class QComboBox;

namespace mdbr::settings {

class PageLayoutSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    using ApplyFn = std::function<void(const layout::PageLayoutSettings &)>;

    PageLayoutSettingsPage(const layout::PageLayoutSettings &current, ApplyFn onApply,
                           QWidget *parent = nullptr);

    [[nodiscard]] QString title() const override { return tr("Page Layout"); }
    void apply() override;
    void restoreDefaults() override;

    [[nodiscard]] layout::PageLayoutSettings settings() const;

private:
    void load(const layout::PageLayoutSettings &settings);

    ApplyFn m_onApply;
    QCheckBox *m_pageView = nullptr;
    QComboBox *m_paper = nullptr;
    QComboBox *m_margins = nullptr;
};

} // namespace mdbr::settings
