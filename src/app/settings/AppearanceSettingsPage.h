#pragma once

// AppearanceSettingsPage — Preferences > Appearance, designed to be simple:
//
//   1. Theme: four cards with a live preview (Follow Windows, Day, Night,
//      Custom). One click, nothing else to understand.
//   2. Custom only: three colours (background, text, links); the other nine
//      roles are derived automatically. A contrast warning appears if text
//      would be hard to read. "More colors" reveals all 12 roles for experts.
//
// Nothing is saved until apply() (OK / Apply in the Preferences window).

#include "SettingsPage.h"
#include "app/appearance/ReaderAppearanceSettings.h"
#include "app/appearance/ReaderTheme.h"

#include <QList>

class QButtonGroup;
class QLabel;
class QPushButton;
class QToolButton;
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
    [[nodiscard]] bool advancedColorsVisible() const;
    [[nodiscard]] bool contrastWarningVisible() const;
    [[nodiscard]] const ReaderAppearanceSettings &customColors() const { return m_custom; }
    void setBasicColors(const QString &background, const QString &text, const QString &link);

private:
    struct ColorRole
    {
        QString label;
        QString ReaderAppearanceSettings::*member;
        QPushButton *button = nullptr;
    };

    QPushButton *addColorButton(ColorRole &role, QWidget *parent);
    void setCustomColors(const ReaderAppearanceSettings &colors);
    void pickColor(ColorRole &role);
    void updateState();

    ReaderAppearanceController *m_controller;
    ReaderAppearanceSettings m_custom;
    QButtonGroup *m_modeGroup = nullptr;
    QToolButton *m_customCard = nullptr;
    QWidget *m_customSection = nullptr;
    QWidget *m_advancedSection = nullptr;
    QToolButton *m_moreColors = nullptr;
    QLabel *m_contrastWarning = nullptr;
    QLabel *m_preview = nullptr;
    QList<ColorRole> m_basicRoles;
    QList<ColorRole> m_advancedRoles;
};

} // namespace mdbr::settings
