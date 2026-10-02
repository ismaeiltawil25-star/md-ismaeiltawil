#pragma once

// SettingsDialog — Settings > Preferences... (Ctrl+,).
//
// One window for all preferences: a section list on the left, the selected
// SettingsPage on the right, and OK / Cancel / Apply / Restore Defaults.
// Restore Defaults affects the visible section only; nothing is saved until
// OK or Apply.

#include <QDialog>
#include <QList>

class QListWidget;
class QStackedWidget;

namespace mdbr::settings {

class SettingsPage;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // Takes ownership. Sections appear in the order they are added.
    void addPage(SettingsPage *page);

    [[nodiscard]] int pageCount() const { return m_pages.size(); }
    [[nodiscard]] SettingsPage *page(int index) const { return m_pages.value(index); }
    void showPage(int index);

    // Commits every page (OK and Apply).
    void applyAll();

private:
    QListWidget *m_sections = nullptr;
    QStackedWidget *m_stack = nullptr;
    QList<SettingsPage *> m_pages;
};

} // namespace mdbr::settings
