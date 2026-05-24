#include <QTest>
#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include "app/appearance/ReaderAppearanceController.h"
#include "app/appearance/ReaderAppearanceRepository.h"
#include "app/appearance/ReaderAppearanceDefaults.h"

class ReaderAppearanceControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void testStartsWithDefaultsWhenNoSettings();
    void testApplySettingsUpdatesAndEmits();
    void testApplySettingsPersists();
    void testApplySettingsNoEmitOnNoChange();
    void testRestoreDefaults();
    void testClearStoredSettings();
    void testInvalidPersistedSettingsLoadDefaults();
    void cleanupTestCase();
};

void ReaderAppearanceControllerTest::initTestCase()
{
    QCoreApplication::setOrganizationName("MDTest_ReaderAppearanceCtrl");
    QCoreApplication::setApplicationName("ReaderAppearanceControllerTest");
}

void ReaderAppearanceControllerTest::init()
{
    QSettings settings;
    settings.remove("readerAppearance");
    settings.sync();
}

void ReaderAppearanceControllerTest::testInvalidPersistedSettingsLoadDefaults()
{
    QSettings settings;
    settings.beginGroup("readerAppearance");
    settings.setValue("schemaVersion", 1);
    settings.setValue("customEnabled", false);
    settings.setValue("colors/mainText", "bad");
    settings.setValue("colors/pageBackground", "#12");
    settings.setValue("colors/linkText", "#GGGGGG");
    settings.endGroup();
    settings.sync();

    ReaderAppearanceController controller;
    auto s = controller.settings();
    auto night = ReaderAppearanceDefaults::nightPalette();
    QCOMPARE(s.customEnabled, false);
    QCOMPARE(s.pageBackground, night.pageBackground);
    QCOMPARE(s.mainText, night.mainText);
    QCOMPARE(s.linkText, night.linkText);
}

void ReaderAppearanceControllerTest::cleanupTestCase()
{
    QSettings settings;
    settings.remove("readerAppearance");
    settings.sync();
}

void ReaderAppearanceControllerTest::testStartsWithDefaultsWhenNoSettings()
{
    ReaderAppearanceController controller;
    auto expected = ReaderAppearanceDefaults::nightPalette();
    QCOMPARE(controller.settings(), expected);
}

void ReaderAppearanceControllerTest::testApplySettingsUpdatesAndEmits()
{
    ReaderAppearanceController controller;
    QSignalSpy spy(&controller, &ReaderAppearanceController::appearanceChanged);

    ReaderAppearanceSettings custom;
    custom.customEnabled = true;
    custom.pageBackground = QStringLiteral("#abcdef");
    custom.mainText = QStringLiteral("#123456");

    controller.applySettings(custom);

    QCOMPARE(spy.count(), 1);
    auto emitted = spy.at(0).at(0).value<ReaderAppearanceSettings>();
    QCOMPARE(emitted.pageBackground, QStringLiteral("#abcdef"));
    QCOMPARE(emitted.mainText, QStringLiteral("#123456"));
    QVERIFY(emitted.customEnabled);

    QCOMPARE(controller.settings().pageBackground, QStringLiteral("#abcdef"));
    QCOMPARE(controller.settings().mainText, QStringLiteral("#123456"));
}

void ReaderAppearanceControllerTest::testApplySettingsPersists()
{
    {
        ReaderAppearanceController controller;
        ReaderAppearanceSettings s;
        s.customEnabled = true;
        s.headingText = QStringLiteral("#aabbcc");
        controller.applySettings(s);
    }

    ReaderAppearanceController controller;
    QCOMPARE(controller.settings().customEnabled, true);
    QCOMPARE(controller.settings().headingText, QStringLiteral("#aabbcc"));
}

void ReaderAppearanceControllerTest::testApplySettingsNoEmitOnNoChange()
{
    ReaderAppearanceController controller;
    QSignalSpy spy(&controller, &ReaderAppearanceController::appearanceChanged);

    controller.applySettings(ReaderAppearanceDefaults::nightPalette());

    QCOMPARE(spy.count(), 0);
}

void ReaderAppearanceControllerTest::testRestoreDefaults()
{
    ReaderAppearanceController controller;
    ReaderAppearanceSettings custom;
    custom.customEnabled = true;
    custom.mainText = QStringLiteral("#111111");
    controller.applySettings(custom);

    QSignalSpy spy(&controller, &ReaderAppearanceController::appearanceChanged);

    controller.restoreDefaults();

    QCOMPARE(spy.count(), 1);
    auto expected = ReaderAppearanceDefaults::nightPalette();
    QCOMPARE(controller.settings(), expected);
}

void ReaderAppearanceControllerTest::testClearStoredSettings()
{
    ReaderAppearanceSettings custom;
    custom.customEnabled = true;
    custom.linkText = QStringLiteral("#ff00ff");
    ReaderAppearanceRepository::save(custom);

    ReaderAppearanceController controller;
    QSignalSpy spy(&controller, &ReaderAppearanceController::appearanceChanged);

    controller.clearStoredSettings();

    QCOMPARE(spy.count(), 1);
    auto expected = ReaderAppearanceDefaults::nightPalette();
    QCOMPARE(controller.settings(), expected);

    ReaderAppearanceController fresh;
    QCOMPARE(fresh.settings(), expected);
}

QTEST_MAIN(ReaderAppearanceControllerTest)
#include "ReaderAppearanceControllerTest.moc"
