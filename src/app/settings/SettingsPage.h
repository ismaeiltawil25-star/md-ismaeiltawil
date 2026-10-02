#pragma once

// SettingsPage — one section of Preferences (Settings > Preferences...).
//
// To add a section: subclass SettingsPage, implement the four functions, and
// add it in SettingsDialog's constructor. The dialog owns OK / Cancel / Apply /
// Restore Defaults; a page only edits its own widgets and commits on apply().

#include <QWidget>

namespace mdbr::settings {

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    using QWidget::QWidget;

    [[nodiscard]] virtual QString title() const = 0;

    // Commit the edited values (OK or Apply).
    virtual void apply() = 0;

    // Reset the widgets to the defaults; nothing is committed until apply().
    virtual void restoreDefaults() = 0;
};

} // namespace mdbr::settings
