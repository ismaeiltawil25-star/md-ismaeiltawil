#include "AppearanceSettingsPage.h"

#include "app/appearance/ReaderAppearanceController.h"
#include "app/appearance/ReaderAppearanceDefaults.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace mdbr::settings {

using appearance::ThemeMode;

AppearanceSettingsPage::AppearanceSettingsPage(ReaderAppearanceController *controller, QWidget *parent)
    : SettingsPage(parent)
    , m_controller(controller)
    , m_custom(controller->settings())
{
    // --- Theme mode -----------------------------------------------------------
    auto *modeBox = new QGroupBox(tr("Theme"), this);
    auto *modeLayout = new QVBoxLayout(modeBox);
    m_modeGroup = new QButtonGroup(this);
    for (const ThemeMode mode : {ThemeMode::System, ThemeMode::Day, ThemeMode::Night, ThemeMode::Custom}) {
        QString text = appearance::themeModeLabel(mode);
        if (mode == ThemeMode::System)
            text += controller->systemIsDark() ? tr("  (Windows is dark now)") : tr("  (Windows is light now)");
        auto *radio = new QRadioButton(text, modeBox);
        m_modeGroup->addButton(radio, static_cast<int>(mode));
        modeLayout->addWidget(radio);
    }

    m_preview = new QLabel(tr("Preview: headings, text and <a href='#'>links</a>"), this);
    m_preview->setMinimumHeight(48);
    m_preview->setContentsMargins(12, 8, 12, 8);

    // --- Custom colours -------------------------------------------------------
    m_colorGroup = new QGroupBox(tr("Custom colors"), this);
    auto *form = new QFormLayout;
    m_roles = {
        {tr("Page background"),        &ReaderAppearanceSettings::pageBackground},
        {tr("Main text"),              &ReaderAppearanceSettings::mainText},
        {tr("Headings"),               &ReaderAppearanceSettings::headingText},
        {tr("Muted text"),             &ReaderAppearanceSettings::mutedText},
        {tr("Links"),                  &ReaderAppearanceSettings::linkText},
        {tr("Inline code text"),       &ReaderAppearanceSettings::inlineCodeText},
        {tr("Inline code background"), &ReaderAppearanceSettings::inlineCodeBackground},
        {tr("Code block text"),        &ReaderAppearanceSettings::codeBlockText},
        {tr("Code block background"),  &ReaderAppearanceSettings::codeBlockBackground},
        {tr("Quote text"),             &ReaderAppearanceSettings::blockquoteText},
        {tr("Quote bar"),              &ReaderAppearanceSettings::blockquoteBorder},
        {tr("Horizontal rule"),        &ReaderAppearanceSettings::horizontalRule},
    };
    for (ColorRole &role : m_roles) {
        role.button = new QPushButton(m_colorGroup);
        role.button->setFixedSize(56, 22);
        connect(role.button, &QPushButton::clicked, this, [this, &role] { pickColor(role); });
        form->addRow(role.label, role.button);
    }
    auto *startDay = new QPushButton(tr("Start from Day"), m_colorGroup);
    auto *startNight = new QPushButton(tr("Start from Night"), m_colorGroup);
    connect(startDay, &QPushButton::clicked, this,
            [this] { setCustomColors(ReaderAppearanceDefaults::dayPalette()); });
    connect(startNight, &QPushButton::clicked, this,
            [this] { setCustomColors(ReaderAppearanceDefaults::nightPalette()); });
    auto *presetRow = new QHBoxLayout;
    presetRow->addWidget(startDay);
    presetRow->addWidget(startNight);
    presetRow->addStretch();
    auto *colorLayout = new QVBoxLayout(m_colorGroup);
    colorLayout->addLayout(presetRow);
    colorLayout->addLayout(form);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(modeBox);
    layout->addWidget(m_preview);
    layout->addWidget(m_colorGroup);
    layout->addStretch();

    connect(m_modeGroup, &QButtonGroup::idClicked, this, [this] { updateState(); });
    selectMode(controller->themeMode());
    setCustomColors(m_custom);
}

ThemeMode AppearanceSettingsPage::selectedMode() const
{
    return static_cast<ThemeMode>(m_modeGroup->checkedId());
}

void AppearanceSettingsPage::selectMode(ThemeMode mode)
{
    if (auto *button = m_modeGroup->button(static_cast<int>(mode)))
        button->setChecked(true);
    updateState();
}

bool AppearanceSettingsPage::colorEditorEnabled() const
{
    return m_colorGroup->isEnabled();
}

void AppearanceSettingsPage::setCustomColors(const ReaderAppearanceSettings &colors)
{
    m_custom = colors;
    for (ColorRole &role : m_roles) {
        const QString hex = m_custom.*(role.member);
        role.button->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid #888;").arg(hex));
        role.button->setToolTip(hex);
    }
    updateState();
}

void AppearanceSettingsPage::pickColor(ColorRole &role)
{
    const QColor chosen = QColorDialog::getColor(QColor(m_custom.*(role.member)), this, role.label);
    if (!chosen.isValid())
        return;
    ReaderAppearanceSettings updated = m_custom;
    updated.*(role.member) = chosen.name();
    setCustomColors(updated);
}

void AppearanceSettingsPage::updateState()
{
    const ThemeMode mode = selectedMode();
    m_colorGroup->setEnabled(mode == ThemeMode::Custom);

    const ReaderAppearanceSettings shown =
        appearance::resolvePalette(mode, m_custom, m_controller->systemIsDark());
    m_preview->setStyleSheet(QStringLiteral("QLabel { background-color: %1; color: %2; border: 1px solid %3; }")
                                 .arg(shown.pageBackground, shown.mainText, shown.horizontalRule));
}

void AppearanceSettingsPage::apply()
{
    ReaderAppearanceSettings custom = m_custom;
    custom.customEnabled = selectedMode() == ThemeMode::Custom; // legacy flag kept consistent
    m_controller->applySettings(custom);
    m_controller->setThemeMode(selectedMode());
}

void AppearanceSettingsPage::restoreDefaults()
{
    selectMode(ThemeMode::System);
    setCustomColors(ReaderAppearanceDefaults::nightPalette());
}

} // namespace mdbr::settings
