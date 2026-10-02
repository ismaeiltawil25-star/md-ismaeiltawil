// Theme modes: palette resolution, persistence/migration, and the controller's
// change notifications (what the toolbar toggle and View > Theme rely on).

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

#include "app/appearance/ReaderAppearanceController.h"
#include "app/appearance/ReaderAppearanceDefaults.h"
#include "app/appearance/ReaderAppearanceRepository.h"
#include "app/appearance/ReaderTheme.h"

using namespace mdbr::appearance;

namespace {

ReaderAppearanceSettings customColors()
{
    ReaderAppearanceSettings s = ReaderAppearanceDefaults::nightPalette();
    s.pageBackground = "#112233";
    s.mainText = "#ddeeff";
    return s;
}

} // namespace

class ReaderThemeTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    // resolvePalette
    void systemFollowsWindows();
    void fixedModesIgnoreWindows();
    void customUsesCustomColors();
    void darkDetection();
    void idsRoundTrip();
    void threeColorsDeriveTheRest();
    void contrastCheck();

    // persistence and migration
    void freshInstallFollowsWindows();
    void legacyNightColorsBecomeNight();
    void legacyDayColorsBecomeDay();
    void legacyOtherColorsBecomeCustom();
    void savedModeWins();

    // controller notifications
    void changingModeRepaintsWhenColorsChange();
    void windowsSwitchRepaintsOnlyInSystemMode();
    void modePersistsAcrossRestarts();
};

void ReaderThemeTest::initTestCase()
{
    QCoreApplication::setOrganizationName("MDTest_ReaderTheme");
    QCoreApplication::setApplicationName("ReaderThemeTest");
}

void ReaderThemeTest::init()
{
    ReaderAppearanceRepository::clear();
}

void ReaderThemeTest::systemFollowsWindows()
{
    QCOMPARE(resolvePalette(ThemeMode::System, customColors(), true), ReaderAppearanceDefaults::nightPalette());
    QCOMPARE(resolvePalette(ThemeMode::System, customColors(), false), ReaderAppearanceDefaults::dayPalette());
}

void ReaderThemeTest::fixedModesIgnoreWindows()
{
    for (const bool dark : {true, false}) {
        QCOMPARE(resolvePalette(ThemeMode::Night, customColors(), dark), ReaderAppearanceDefaults::nightPalette());
        QCOMPARE(resolvePalette(ThemeMode::Day, customColors(), dark), ReaderAppearanceDefaults::dayPalette());
    }
}

void ReaderThemeTest::customUsesCustomColors()
{
    QCOMPARE(resolvePalette(ThemeMode::Custom, customColors(), false), customColors());
}

void ReaderThemeTest::darkDetection()
{
    QVERIFY(isDarkPalette(ReaderAppearanceDefaults::nightPalette()));
    QVERIFY(!isDarkPalette(ReaderAppearanceDefaults::dayPalette()));
    QVERIFY(!isDarkPalette(ReaderAppearanceDefaults::calmPalette()));
}

void ReaderThemeTest::idsRoundTrip()
{
    for (const ThemeMode m : {ThemeMode::System, ThemeMode::Day, ThemeMode::Night, ThemeMode::Custom})
        QCOMPARE(themeModeFromId(themeModeId(m)), m);
    QCOMPARE(themeModeFromId("nonsense"), ThemeMode::System);
}

void ReaderThemeTest::threeColorsDeriveTheRest()
{
    const ReaderAppearanceSettings p = deriveCustomPalette("#1e1e2e", "#cdd6f4", "#89b4fa");
    QCOMPARE(p.pageBackground, QString("#1e1e2e"));
    QCOMPARE(p.mainText, QString("#cdd6f4"));
    QCOMPARE(p.linkText, QString("#89b4fa"));
    // Derived roles sit between background and text, so nothing is invisible.
    for (const QString &derived : {p.mutedText, p.blockquoteText, p.blockquoteBorder,
                                   p.horizontalRule, p.codeBlockBackground}) {
        QVERIFY(derived != p.pageBackground);
        QVERIFY(derived != p.mainText);
    }
    QVERIFY(colorContrast(p.mutedText, p.pageBackground) >= 3.0); // muted, still legible
}

void ReaderThemeTest::contrastCheck()
{
    QVERIFY(hasReadableText(ReaderAppearanceDefaults::nightPalette()));
    QVERIFY(hasReadableText(ReaderAppearanceDefaults::dayPalette()));
    QVERIFY(!hasReadableText(deriveCustomPalette("#777777", "#888888", "#0000ff")));
    QCOMPARE(qRound(colorContrast("#000000", "#ffffff")), 21);
}

void ReaderThemeTest::freshInstallFollowsWindows()
{
    QCOMPARE(ReaderAppearanceRepository::loadThemeMode(), ThemeMode::System);
}

void ReaderThemeTest::legacyNightColorsBecomeNight()
{
    ReaderAppearanceRepository::save(ReaderAppearanceDefaults::nightPalette()); // no themeMode key
    QCOMPARE(ReaderAppearanceRepository::loadThemeMode(), ThemeMode::Night);
}

void ReaderThemeTest::legacyDayColorsBecomeDay()
{
    ReaderAppearanceRepository::save(ReaderAppearanceDefaults::dayPalette());
    QCOMPARE(ReaderAppearanceRepository::loadThemeMode(), ThemeMode::Day);
}

void ReaderThemeTest::legacyOtherColorsBecomeCustom()
{
    ReaderAppearanceRepository::save(customColors());
    QCOMPARE(ReaderAppearanceRepository::loadThemeMode(), ThemeMode::Custom);
}

void ReaderThemeTest::savedModeWins()
{
    ReaderAppearanceRepository::save(ReaderAppearanceDefaults::nightPalette());
    ReaderAppearanceRepository::saveThemeMode(ThemeMode::Day);
    QCOMPARE(ReaderAppearanceRepository::loadThemeMode(), ThemeMode::Day);
}

void ReaderThemeTest::changingModeRepaintsWhenColorsChange()
{
    ReaderAppearanceController controller;
    controller.setSystemIsDark(true);
    controller.setThemeMode(ThemeMode::Night);
    QSignalSpy changed(&controller, &ReaderAppearanceController::appearanceChanged);
    QSignalSpy modeChanged(&controller, &ReaderAppearanceController::themeModeChanged);

    controller.setThemeMode(ThemeMode::Day);
    QCOMPARE(modeChanged.count(), 1);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(controller.effectiveSettings(), ReaderAppearanceDefaults::dayPalette());

    controller.setThemeMode(ThemeMode::Day); // no change, no signal
    QCOMPARE(modeChanged.count(), 1);

    // Night -> System while Windows is dark shows the same colours: mode changes,
    // but nothing needs repainting.
    controller.setThemeMode(ThemeMode::Night);
    changed.clear();
    controller.setThemeMode(ThemeMode::System);
    QCOMPARE(changed.count(), 0);
}

void ReaderThemeTest::windowsSwitchRepaintsOnlyInSystemMode()
{
    ReaderAppearanceController controller;
    controller.setThemeMode(ThemeMode::System);
    controller.setSystemIsDark(true);
    QSignalSpy changed(&controller, &ReaderAppearanceController::appearanceChanged);

    controller.setSystemIsDark(false);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(controller.effectiveSettings(), ReaderAppearanceDefaults::dayPalette());

    controller.setThemeMode(ThemeMode::Night);
    changed.clear();
    controller.setSystemIsDark(true);
    QCOMPARE(changed.count(), 0); // Night ignores Windows
}

void ReaderThemeTest::modePersistsAcrossRestarts()
{
    {
        ReaderAppearanceController controller;
        controller.setThemeMode(ThemeMode::Custom);
    }
    ReaderAppearanceController restarted;
    QCOMPARE(restarted.themeMode(), ThemeMode::Custom);
}

QTEST_GUILESS_MAIN(ReaderThemeTest)
#include "ReaderThemeTest.moc"
