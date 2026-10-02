// Preferences window: sections, theme page behaviour, saving, restore defaults,
// and the main window's night toggle.

#include <QAction>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>

#include "app/MainWindow.h"
#include "app/appearance/ReaderAppearanceController.h"
#include "app/appearance/ReaderAppearanceDefaults.h"
#include "app/appearance/ReaderAppearanceRepository.h"
#include "app/layout/PageLayoutRepository.h"
#include "app/settings/AppearanceSettingsPage.h"
#include "app/settings/PageLayoutSettingsPage.h"
#include "app/settings/SettingsDialog.h"

using namespace mdbr::settings;
using mdbr::appearance::ThemeMode;

class SettingsDialogTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void sectionsAreListed();
    void colorEditorOnlyInCustomMode();
    void applyCommitsThemeMode();
    void cancelCommitsNothing();
    void pageLayoutApplyReachesCallback();
    void restoreDefaultsResetsVisibleSection();
    void nightToggleRecoloursTheReader();
    void screenshotsForReview();
};

void SettingsDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("MDTest_SettingsDialog");
    QCoreApplication::setApplicationName("SettingsDialogTest");
}

void SettingsDialogTest::init()
{
    ReaderAppearanceRepository::clear();
    mdbr::layout::PageLayoutRepository::clear();
}

void SettingsDialogTest::sectionsAreListed()
{
    ReaderAppearanceController controller;
    SettingsDialog dialog;
    dialog.addPage(new AppearanceSettingsPage(&controller));
    dialog.addPage(new PageLayoutSettingsPage({}, nullptr));
    QCOMPARE(dialog.pageCount(), 2);
    QCOMPARE(dialog.page(0)->title(), QString("Appearance"));
    QCOMPARE(dialog.page(1)->title(), QString("Page Layout"));
}

void SettingsDialogTest::colorEditorOnlyInCustomMode()
{
    ReaderAppearanceController controller;
    AppearanceSettingsPage page(&controller);
    for (const ThemeMode mode : {ThemeMode::System, ThemeMode::Day, ThemeMode::Night}) {
        page.selectMode(mode);
        QVERIFY(!page.colorEditorEnabled());
    }
    page.selectMode(ThemeMode::Custom);
    QVERIFY(page.colorEditorEnabled());
}

void SettingsDialogTest::applyCommitsThemeMode()
{
    ReaderAppearanceController controller;
    controller.setThemeMode(ThemeMode::Day);
    SettingsDialog dialog;
    auto *page = new AppearanceSettingsPage(&controller);
    dialog.addPage(page);

    page->selectMode(ThemeMode::Night);
    dialog.applyAll();
    QCOMPARE(controller.themeMode(), ThemeMode::Night);
    QCOMPARE(controller.effectiveSettings(), ReaderAppearanceDefaults::nightPalette());
    QCOMPARE(ReaderAppearanceRepository::loadThemeMode(), ThemeMode::Night); // saved
}

void SettingsDialogTest::cancelCommitsNothing()
{
    ReaderAppearanceController controller;
    controller.setThemeMode(ThemeMode::Day);
    {
        SettingsDialog dialog;
        auto *page = new AppearanceSettingsPage(&controller);
        dialog.addPage(page);
        page->selectMode(ThemeMode::Night);
        dialog.reject();
    }
    QCOMPARE(controller.themeMode(), ThemeMode::Day);
}

void SettingsDialogTest::pageLayoutApplyReachesCallback()
{
    mdbr::layout::PageLayoutSettings received;
    bool called = false;
    mdbr::layout::PageLayoutSettings start;
    start.pageViewEnabled = true;
    start.paper = mdbr::layout::PaperSize::Letter;

    PageLayoutSettingsPage page(start, [&](const mdbr::layout::PageLayoutSettings &s) {
        received = s;
        called = true;
    });
    QCOMPARE(page.settings(), start);
    page.apply();
    QVERIFY(called);
    QCOMPARE(received, start);
}

void SettingsDialogTest::restoreDefaultsResetsVisibleSection()
{
    mdbr::layout::PageLayoutSettings start;
    start.pageViewEnabled = true;
    start.margins = mdbr::layout::PageMargins::Wide;
    PageLayoutSettingsPage page(start, nullptr);
    page.restoreDefaults();
    QCOMPARE(page.settings(), mdbr::layout::PageLayoutSettings{});

    ReaderAppearanceController controller;
    controller.setThemeMode(ThemeMode::Custom);
    AppearanceSettingsPage appearance(&controller);
    appearance.restoreDefaults();
    QCOMPARE(appearance.selectedMode(), ThemeMode::System);
    QCOMPARE(controller.themeMode(), ThemeMode::Custom); // nothing committed yet
}

void SettingsDialogTest::nightToggleRecoloursTheReader()
{
    QTemporaryDir dir;
    QFile file(dir.filePath("doc.md"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("# Title\n\nText.\n");
    file.close();

    MainWindow window;
    window.openDocumentsFromPaths({file.fileName()});
    auto *page = qobject_cast<DocumentTabPage *>(window.findChild<QTabWidget *>()->currentWidget());
    QVERIFY(page);
    auto *night = window.findChild<QAction *>("nightModeAction");
    QVERIFY(night);

    const QString nightBg = ReaderAppearanceDefaults::nightPalette().pageBackground;
    const QString dayBg = ReaderAppearanceDefaults::dayPalette().pageBackground;

    night->setChecked(false);
    night->trigger(); // -> Night
    QVERIFY(night->isChecked());
    QVERIFY2(page->styleSheet().contains(nightBg, Qt::CaseInsensitive), qPrintable(page->styleSheet()));

    night->trigger(); // -> Day
    QVERIFY(!night->isChecked());
    QVERIFY2(page->styleSheet().contains(dayBg, Qt::CaseInsensitive), qPrintable(page->styleSheet()));
}

// Saves images of the Preferences window for visual review when
// MDBR_SAVE_SETTINGS_SHOTS names a folder; skipped otherwise.
void SettingsDialogTest::screenshotsForReview()
{
    const QString folder = qEnvironmentVariable("MDBR_SAVE_SETTINGS_SHOTS");
    if (folder.isEmpty())
        QSKIP("set MDBR_SAVE_SETTINGS_SHOTS to a folder to save screenshots");

    ReaderAppearanceController controller;
    controller.setThemeMode(ThemeMode::Night);
    SettingsDialog dialog;
    dialog.addPage(new AppearanceSettingsPage(&controller));
    dialog.addPage(new PageLayoutSettingsPage({}, nullptr));
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    dialog.grab().save(folder + "/preferences-appearance.png");
    dialog.showPage(1);
    QTest::qWait(50);
    dialog.grab().save(folder + "/preferences-page-layout.png");
}

QTEST_MAIN(SettingsDialogTest)
#include "SettingsDialogTest.moc"
