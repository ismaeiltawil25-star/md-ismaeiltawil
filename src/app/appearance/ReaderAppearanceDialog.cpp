#include "ReaderAppearanceDialog.h"
#include "ReaderAppearanceDefaults.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ReaderAppearanceDialog::ReaderAppearanceDialog(
    const ReaderAppearanceSettings &initial, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Reader Appearance");
    setMinimumWidth(420);

    m_temp = initial;

    m_customCheck = new QCheckBox("Use custom reader colors");

    m_colors = {
        {"Page background",        &ReaderAppearanceSettings::pageBackground,       &m_btnPageBg},
        {"Main text",              &ReaderAppearanceSettings::mainText,             &m_btnMainText},
        {"Heading text",           &ReaderAppearanceSettings::headingText,          &m_btnHeadingText},
        {"Muted text",             &ReaderAppearanceSettings::mutedText,            &m_btnMutedText},
        {"Link text",              &ReaderAppearanceSettings::linkText,             &m_btnLinkText},
        {"Inline code text",       &ReaderAppearanceSettings::inlineCodeText,       &m_btnCodeText},
        {"Inline code background", &ReaderAppearanceSettings::inlineCodeBackground, &m_btnCodeBg},
        {"Code block text",        &ReaderAppearanceSettings::codeBlockText,        &m_btnBlockText},
        {"Code block background",  &ReaderAppearanceSettings::codeBlockBackground,  &m_btnBlockBg},
        {"Blockquote text",        &ReaderAppearanceSettings::blockquoteText,       &m_btnBlockquoteText},
        {"Blockquote border",      &ReaderAppearanceSettings::blockquoteBorder,     &m_btnBlockBorder},
        {"Horizontal rule",        &ReaderAppearanceSettings::horizontalRule,       &m_btnHr},
    };

    auto *dayBtn = new QPushButton("Day");
    auto *nightBtn = new QPushButton("Night");
    auto *presetLayout = new QHBoxLayout;
    presetLayout->addWidget(dayBtn);
    presetLayout->addWidget(nightBtn);
    presetLayout->addStretch();

    auto *form = new QFormLayout;
    for (auto &entry : m_colors) {
        *entry.button = createColorButton(m_temp.*(entry.member));
        auto *row = new QHBoxLayout;
        row->addWidget(*entry.button);
        row->addStretch();
        form->addRow(entry.label, row);
    }

    auto *restoreBtn = new QPushButton("Restore Defaults");

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_customCheck);
    mainLayout->addLayout(presetLayout);
    mainLayout->addLayout(form);
    mainLayout->addWidget(restoreBtn);
    mainLayout->addWidget(buttonBox);

    connect(m_customCheck, &QCheckBox::toggled,
            this, &ReaderAppearanceDialog::onCustomEnabledChanged);
    connect(dayBtn, &QPushButton::clicked,
            this, &ReaderAppearanceDialog::onDayPreset);
    connect(nightBtn, &QPushButton::clicked,
            this, &ReaderAppearanceDialog::onNightPreset);
    connect(restoreBtn, &QPushButton::clicked,
            this, &ReaderAppearanceDialog::onRestoreDefaults);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::clicked,
            this, [this, buttonBox](QAbstractButton *btn) {
        if (buttonBox->standardButton(btn) == QDialogButtonBox::Apply) {
            emit settingsApplied(m_temp);
        }
    });

    updateEnabledStates();
}

ReaderAppearanceSettings ReaderAppearanceDialog::settings() const
{
    return m_temp;
}

void ReaderAppearanceDialog::setSettings(const ReaderAppearanceSettings &settings)
{
    m_temp = settings;
    m_customCheck->setChecked(settings.customEnabled);
    for (auto &entry : m_colors)
        updateButtonColor(*entry.button, m_temp.*(entry.member));
    updateEnabledStates();
}

void ReaderAppearanceDialog::onRestoreDefaults()
{
    setSettings(ReaderAppearanceDefaults::nightPalette());
}

void ReaderAppearanceDialog::onDayPreset()
{
    setSettings(ReaderAppearanceDefaults::dayPalette());
}

void ReaderAppearanceDialog::onNightPreset()
{
    setSettings(ReaderAppearanceDefaults::nightPalette());
}

void ReaderAppearanceDialog::onCustomEnabledChanged()
{
    m_temp.customEnabled = m_customCheck->isChecked();
    updateEnabledStates();
}

void ReaderAppearanceDialog::onPickColor()
{
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn)
        return;

    QString current = btn->property("hexColor").toString();
    QColor chosen = QColorDialog::getColor(QColor(current), this, "Choose Color");
    if (!chosen.isValid())
        return;

    QString hex = chosen.name();
    updateButtonColor(btn, hex);

    for (auto &entry : m_colors) {
        if (*entry.button == btn) {
            m_temp.*(entry.member) = hex;
            break;
        }
    }
}

QPushButton *ReaderAppearanceDialog::createColorButton(const QString &color)
{
    auto *btn = new QPushButton;
    btn->setFixedSize(48, 24);
    updateButtonColor(btn, color);
    connect(btn, &QPushButton::clicked, this, &ReaderAppearanceDialog::onPickColor);
    return btn;
}

void ReaderAppearanceDialog::updateButtonColor(QPushButton *button, const QString &color)
{
    button->setStyleSheet(
        QStringLiteral("background-color: %1; border: 1px solid #888;").arg(color));
    button->setProperty("hexColor", color);
}

void ReaderAppearanceDialog::updateEnabledStates()
{
    bool enabled = m_customCheck->isChecked();
    for (auto &entry : m_colors)
        (*entry.button)->setEnabled(enabled);
}
