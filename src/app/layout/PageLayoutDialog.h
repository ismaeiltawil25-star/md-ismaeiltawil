#pragma once

// PageLayoutDialog — Settings > Page Layout...
// Lets the user switch between continuous reading and page view, and choose
// the paper size and margins. Returns settings; persistence is the caller's job.

#include <QDialog>
#include "PageLayoutSettings.h"

class QCheckBox;
class QComboBox;

namespace mdbr::layout {

class PageLayoutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PageLayoutDialog(const PageLayoutSettings &initial, QWidget *parent = nullptr);

    [[nodiscard]] PageLayoutSettings settings() const;

private:
    void updateEnabledStates();

    QCheckBox *m_pageViewCheck = nullptr;
    QComboBox *m_paperCombo = nullptr;
    QComboBox *m_marginsCombo = nullptr;
};

} // namespace mdbr::layout
