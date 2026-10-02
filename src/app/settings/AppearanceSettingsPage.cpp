#include "AppearanceSettingsPage.h"

#include "InteractionStyle.h"
#include "ThemeIcons.h"
#include "app/appearance/ReaderAppearanceController.h"
#include "app/appearance/ReaderAppearanceDefaults.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace mdbr::settings {

using appearance::ThemeMode;

namespace {

QLabel *sectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
    return label;
}

} // namespace

AppearanceSettingsPage::AppearanceSettingsPage(ReaderAppearanceController *controller, QWidget *parent)
    : SettingsPage(parent)
    , m_controller(controller)
    , m_custom(controller->settings())
{
    // --- 1. Theme cards ----------------------------------------------------------
    const ReaderAppearanceSettings day = ReaderAppearanceDefaults::dayPalette();
    const ReaderAppearanceSettings night = ReaderAppearanceDefaults::nightPalette();

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->setExclusive(true);
    auto *cards = new QHBoxLayout;
    cards->setSpacing(10);
    const auto addCard = [&](ThemeMode mode, const QIcon &icon, const QString &tooltip) {
        auto *card = new QToolButton(this);
        card->setCheckable(true);
        card->setText(appearance::themeModeLabel(mode));
        card->setIcon(icon);
        card->setIconSize(QSize(72, 48));
        card->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        card->setMinimumSize(110, 92);
        card->setToolTip(tooltip);
        card->setProperty("themeCard", true);
        m_modeGroup->addButton(card, static_cast<int>(mode));
        cards->addWidget(card);
        return card;
    };
    addCard(ThemeMode::System, themePreviewIcon(day, &night),
            controller->systemIsDark() ? tr("Matches Windows (dark now)") : tr("Matches Windows (light now)"));
    addCard(ThemeMode::Day, themePreviewIcon(day), tr("Light page, dark text"));
    addCard(ThemeMode::Night, themePreviewIcon(night), tr("Dark page, light text"));
    m_customCard = addCard(ThemeMode::Custom, themePreviewIcon(m_custom), tr("Your own colors"));
    cards->addStretch();

    m_preview = new QLabel(tr("The quick brown fox jumps over the lazy dog. <a href='#'>A link</a>"), this);
    m_preview->setWordWrap(true);
    m_preview->setContentsMargins(14, 10, 14, 10);

    // --- 2. Custom colours: three basics + optional advanced roles ----------------
    m_customSection = new QWidget(this);
    m_basicRoles = {
        {tr("Background"), &ReaderAppearanceSettings::pageBackground},
        {tr("Text"),       &ReaderAppearanceSettings::mainText},
        {tr("Links"),      &ReaderAppearanceSettings::linkText},
    };
    auto *basics = new QHBoxLayout;
    for (ColorRole &role : m_basicRoles) {
        basics->addWidget(new QLabel(role.label, m_customSection));
        basics->addWidget(addColorButton(role, m_customSection));
        basics->addSpacing(12);
    }
    basics->addStretch();

    m_contrastWarning = new QLabel(QStringLiteral(u"\u26A0 ")
                                       + tr("Text and background are too similar; text may be hard to read."),
                                   m_customSection);
    m_contrastWarning->setWordWrap(true);

    m_moreColors = new QToolButton(m_customSection);
    m_moreColors->setText(tr("More colors"));
    m_moreColors->setCheckable(true);
    m_moreColors->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_moreColors->setArrowType(Qt::RightArrow);
    m_moreColors->setAutoRaise(true);

    m_advancedSection = new QWidget(m_customSection);
    m_advancedRoles = {
        {tr("Headings"),               &ReaderAppearanceSettings::headingText},
        {tr("Muted text"),             &ReaderAppearanceSettings::mutedText},
        {tr("Inline code text"),       &ReaderAppearanceSettings::inlineCodeText},
        {tr("Inline code background"), &ReaderAppearanceSettings::inlineCodeBackground},
        {tr("Code block text"),        &ReaderAppearanceSettings::codeBlockText},
        {tr("Code block background"),  &ReaderAppearanceSettings::codeBlockBackground},
        {tr("Quote text"),             &ReaderAppearanceSettings::blockquoteText},
        {tr("Quote bar"),              &ReaderAppearanceSettings::blockquoteBorder},
        {tr("Horizontal rule"),        &ReaderAppearanceSettings::horizontalRule},
    };
    auto *grid = new QGridLayout(m_advancedSection);
    grid->setContentsMargins(18, 0, 0, 0);
    for (int i = 0; i < m_advancedRoles.size(); ++i) {
        ColorRole &role = m_advancedRoles[i];
        grid->addWidget(new QLabel(role.label, m_advancedSection), i / 2, (i % 2) * 2);
        grid->addWidget(addColorButton(role, m_advancedSection), i / 2, (i % 2) * 2 + 1);
    }
    m_advancedSection->setVisible(false);

    auto *customLayout = new QVBoxLayout(m_customSection);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->addWidget(sectionTitle(tr("Custom colors"), m_customSection));
    customLayout->addLayout(basics);
    customLayout->addWidget(m_contrastWarning);
    customLayout->addWidget(m_moreColors, 0, Qt::AlignLeft);
    customLayout->addWidget(m_advancedSection);

    // --- Page layout ----------------------------------------------------------------
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(sectionTitle(tr("Theme"), this));
    layout->addLayout(cards);
    layout->addWidget(m_preview);
    layout->addWidget(m_customSection);
    layout->addStretch();

    // Clear checked/hover states for the cards in light and dark themes.
    const InteractionColors colors = interactionColors(palette());
    setStyleSheet(QStringLiteral(
                      "QToolButton[themeCard=\"true\"] { border: 1px solid %1; border-radius: 8px; padding: 6px; }"
                      "QToolButton[themeCard=\"true\"]:hover { background-color: %2; }"
                      "QToolButton[themeCard=\"true\"]:checked { border: 2px solid %3; background-color: %4; color: %5; }")
                      .arg(colors.hover.name(), colors.hover.name(), colors.checkedBorder.name(),
                           colors.checked.name(), colors.text.name()));

    connect(m_modeGroup, &QButtonGroup::idClicked, this, [this] { updateState(); });
    connect(m_moreColors, &QToolButton::toggled, this, [this](bool shown) {
        m_advancedSection->setVisible(shown);
        m_moreColors->setArrowType(shown ? Qt::DownArrow : Qt::RightArrow);
    });

    selectMode(controller->themeMode());
    setCustomColors(m_custom);
}

QPushButton *AppearanceSettingsPage::addColorButton(ColorRole &role, QWidget *parent)
{
    role.button = new QPushButton(parent);
    role.button->setFixedSize(44, 22);
    role.button->setAccessibleName(role.label);
    connect(role.button, &QPushButton::clicked, this, [this, &role] { pickColor(role); });
    return role.button;
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
    return !m_customSection->isHidden();
}

bool AppearanceSettingsPage::advancedColorsVisible() const
{
    return !m_advancedSection->isHidden();
}

bool AppearanceSettingsPage::contrastWarningVisible() const
{
    return !m_contrastWarning->isHidden();
}

void AppearanceSettingsPage::setBasicColors(const QString &background, const QString &text, const QString &link)
{
    setCustomColors(appearance::deriveCustomPalette(background, text, link));
}

void AppearanceSettingsPage::setCustomColors(const ReaderAppearanceSettings &colors)
{
    m_custom = colors;
    for (QList<ColorRole> *roles : {&m_basicRoles, &m_advancedRoles}) {
        for (ColorRole &role : *roles) {
            const QString hex = m_custom.*(role.member);
            role.button->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(hex));
            role.button->setToolTip(hex);
        }
    }
    m_customCard->setIcon(themePreviewIcon(m_custom));
    updateState();
}

void AppearanceSettingsPage::pickColor(ColorRole &role)
{
    const QColor chosen = QColorDialog::getColor(QColor(m_custom.*(role.member)), this, role.label);
    if (!chosen.isValid())
        return;

    const bool isBasic = std::any_of(m_basicRoles.cbegin(), m_basicRoles.cend(),
                                     [&role](const ColorRole &r) { return r.member == role.member; });
    if (isBasic) {
        // Changing a basic colour re-derives the whole palette so it stays coherent.
        ReaderAppearanceSettings basics = m_custom;
        basics.*(role.member) = chosen.name();
        setBasicColors(basics.pageBackground, basics.mainText, basics.linkText);
    } else {
        ReaderAppearanceSettings updated = m_custom;
        updated.*(role.member) = chosen.name();
        setCustomColors(updated);
    }
}

void AppearanceSettingsPage::updateState()
{
    const ThemeMode mode = selectedMode();
    m_customSection->setVisible(mode == ThemeMode::Custom);
    m_contrastWarning->setVisible(!appearance::hasReadableText(m_custom));

    const ReaderAppearanceSettings shown =
        appearance::resolvePalette(mode, m_custom, m_controller->systemIsDark());
    m_preview->setStyleSheet(QStringLiteral("QLabel { background-color: %1; color: %2; border: 1px solid %3; "
                                            "border-radius: 6px; }")
                                 .arg(shown.pageBackground, shown.mainText, shown.horizontalRule));
    m_preview->setText(tr("The quick brown fox jumps over the lazy dog. ")
                       + QStringLiteral("<a href='#' style='color:%1'>").arg(shown.linkText) + tr("A link")
                       + QStringLiteral("</a>"));
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
