#ifndef READERAPPEARANCEDIALOG_H
#define READERAPPEARANCEDIALOG_H

#include <QDialog>
#include "ReaderAppearanceSettings.h"

class QCheckBox;
class QPushButton;

class ReaderAppearanceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReaderAppearanceDialog(const ReaderAppearanceSettings &initial,
                                    QWidget *parent = nullptr);

    ReaderAppearanceSettings settings() const;
    void setSettings(const ReaderAppearanceSettings &settings);

signals:
    void settingsApplied(const ReaderAppearanceSettings &settings);

private slots:
    void onRestoreDefaults();
    void onCustomEnabledChanged();
    void onPickColor();
    void onDayPreset();
    void onNightPreset();

private:
    QPushButton *createColorButton(const QString &color);
    void updateButtonColor(QPushButton *button, const QString &color);
    void updateEnabledStates();

    ReaderAppearanceSettings m_temp;

    QCheckBox *m_customCheck = nullptr;
    QPushButton *m_btnPageBg = nullptr;
    QPushButton *m_btnMainText = nullptr;
    QPushButton *m_btnHeadingText = nullptr;
    QPushButton *m_btnMutedText = nullptr;
    QPushButton *m_btnLinkText = nullptr;
    QPushButton *m_btnCodeText = nullptr;
    QPushButton *m_btnCodeBg = nullptr;
    QPushButton *m_btnBlockText = nullptr;
    QPushButton *m_btnBlockBg = nullptr;
    QPushButton *m_btnBlockquoteText = nullptr;
    QPushButton *m_btnBlockBorder = nullptr;
    QPushButton *m_btnHr = nullptr;

    struct ColorEntry {
        QString label;
        QString ReaderAppearanceSettings::*member;
        QPushButton **button;
    };
    QList<ColorEntry> m_colors;
};

#endif
