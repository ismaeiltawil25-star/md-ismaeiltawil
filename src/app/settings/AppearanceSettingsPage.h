#pragma once

// AppearanceSettingsPage — theme mode (Follow Windows / Day / Night / Custom)
// and the 12 custom reader colours, with a live preview.
// The colour editor is enabled only in Custom mode, so it is always clear
// which colours are on screen.

#include "SettingsPage.h"
#include "app/appearance/ReaderAppearanceSettings.h"
#include "app/appearance/ReaderTheme.h"

#include <QList>

class QButtonGroup;
class QGroupBox;
class QLabel;
class QPushButton;
class ReaderAppearanceController;

namespace mdbr::settings {

class AppearanceSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    explicit AppearanceSettingsPage(ReaderAppearanceController *controller, QWidget *parent = nullptr);

    [[nodiscard]] QString title() const override { return tr("Appearance"); }
    void apply() override;
    void restoreDefaults() override;

    // Exposed for tests.
    [[nodiscard]] appearance::ThemeMode selectedMode() const;
    void selectMode(appearance::ThemeMode mode);
    [[nodiscard]] bool colorEditorEnabled() const;

private:
    struct ColorRole
    {
        QString label;
        QString ReaderAppearanceSettings::*member;
        QPushButton *button = nullptr;
    };

    void setCustomColors(const ReaderAppearanceSettings &colors);
    void pickColor(ColorRole &role);
    void updateState();

    ReaderAppearanceController *m_controller;
    ReaderAppearanceSettings m_custom;
    QButtonGroup *m_modeGroup = nullptr;
    QGroupBox *m_colorGroup = nullptr;
    QLabel *m_preview = nullptr;
    QList<ColorRole> m_roles;
};

} // namespace mdbr::settings
