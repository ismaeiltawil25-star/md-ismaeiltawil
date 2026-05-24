#include <QTest>
#include <QCoreApplication>
#include <QSettings>
#include "app/appearance/ReaderAppearanceRepository.h"
#include "app/appearance/ReaderAppearanceDefaults.h"

class ReaderAppearanceRepositoryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testMissingSettingsLoadDefaults();
    void testSaveThenLoadPreservesAllRoles();
    void testCustomEnabledIsPersisted();
    void testInvalidColorFallsBackToDefault();
    void testSchemaVersionKeyExistsAfterSave();
    void cleanupTestCase();
};

void ReaderAppearanceRepositoryTest::initTestCase()
{
    QCoreApplication::setOrganizationName("MDTest_ReaderAppearance");
    QCoreApplication::setApplicationName("ReaderAppearanceRepoTest");
    QSettings settings;
    settings.remove("readerAppearance");
}

void ReaderAppearanceRepositoryTest::cleanupTestCase()
{
    QSettings settings;
    settings.remove("readerAppearance");
}

void ReaderAppearanceRepositoryTest::testMissingSettingsLoadDefaults()
{
    QSettings settings;
    settings.remove("readerAppearance");
    settings.sync();

    auto loaded = ReaderAppearanceRepository::load();
    auto expected = ReaderAppearanceDefaults::nightPalette();

    QCOMPARE(loaded.customEnabled, expected.customEnabled);
    QCOMPARE(loaded.pageBackground, expected.pageBackground);
    QCOMPARE(loaded.mainText, expected.mainText);
    QCOMPARE(loaded.headingText, expected.headingText);
    QCOMPARE(loaded.mutedText, expected.mutedText);
    QCOMPARE(loaded.linkText, expected.linkText);
    QCOMPARE(loaded.inlineCodeText, expected.inlineCodeText);
    QCOMPARE(loaded.inlineCodeBackground, expected.inlineCodeBackground);
    QCOMPARE(loaded.codeBlockText, expected.codeBlockText);
    QCOMPARE(loaded.codeBlockBackground, expected.codeBlockBackground);
    QCOMPARE(loaded.blockquoteText, expected.blockquoteText);
    QCOMPARE(loaded.blockquoteBorder, expected.blockquoteBorder);
    QCOMPARE(loaded.horizontalRule, expected.horizontalRule);
}

void ReaderAppearanceRepositoryTest::testSaveThenLoadPreservesAllRoles()
{
    ReaderAppearanceSettings original;
    original.customEnabled = true;
    original.pageBackground    = QStringLiteral("#ffffff");
    original.mainText          = QStringLiteral("#111111");
    original.headingText       = QStringLiteral("#222222");
    original.mutedText         = QStringLiteral("#333333");
    original.linkText          = QStringLiteral("#444444");
    original.inlineCodeText    = QStringLiteral("#555555");
    original.inlineCodeBackground = QStringLiteral("#666666");
    original.codeBlockText     = QStringLiteral("#777777");
    original.codeBlockBackground  = QStringLiteral("#888888");
    original.blockquoteText    = QStringLiteral("#999999");
    original.blockquoteBorder  = QStringLiteral("#aaaaaa");
    original.horizontalRule    = QStringLiteral("#bbbbbb");

    QVERIFY(ReaderAppearanceRepository::save(original));

    auto loaded = ReaderAppearanceRepository::load();

    QCOMPARE(loaded.customEnabled, original.customEnabled);
    QCOMPARE(loaded.pageBackground, original.pageBackground);
    QCOMPARE(loaded.mainText, original.mainText);
    QCOMPARE(loaded.headingText, original.headingText);
    QCOMPARE(loaded.mutedText, original.mutedText);
    QCOMPARE(loaded.linkText, original.linkText);
    QCOMPARE(loaded.inlineCodeText, original.inlineCodeText);
    QCOMPARE(loaded.inlineCodeBackground, original.inlineCodeBackground);
    QCOMPARE(loaded.codeBlockText, original.codeBlockText);
    QCOMPARE(loaded.codeBlockBackground, original.codeBlockBackground);
    QCOMPARE(loaded.blockquoteText, original.blockquoteText);
    QCOMPARE(loaded.blockquoteBorder, original.blockquoteBorder);
    QCOMPARE(loaded.horizontalRule, original.horizontalRule);
}

void ReaderAppearanceRepositoryTest::testCustomEnabledIsPersisted()
{
    ReaderAppearanceSettings s;
    s.customEnabled = true;
    ReaderAppearanceRepository::save(s);

    auto loaded = ReaderAppearanceRepository::load();
    QVERIFY(loaded.customEnabled);

    s.customEnabled = false;
    ReaderAppearanceRepository::save(s);

    loaded = ReaderAppearanceRepository::load();
    QVERIFY(!loaded.customEnabled);
}

void ReaderAppearanceRepositoryTest::testInvalidColorFallsBackToDefault()
{
    QSettings settings;
    settings.beginGroup("readerAppearance");
    settings.setValue("schemaVersion", 1);
    settings.setValue("customEnabled", true);
    settings.setValue("colors/mainText", "not-a-hex");
    settings.setValue("colors/pageBackground", "#12");      // too short
    settings.setValue("colors/linkText", "#GGGGGG");         // invalid hex
    settings.endGroup();
    settings.sync();

    auto loaded = ReaderAppearanceRepository::load();
    auto def = ReaderAppearanceDefaults::nightPalette();

    QVERIFY(loaded.customEnabled);
    QCOMPARE(loaded.pageBackground, def.pageBackground);
    QCOMPARE(loaded.mainText, def.mainText);
    QCOMPARE(loaded.linkText, def.linkText);
}

void ReaderAppearanceRepositoryTest::testSchemaVersionKeyExistsAfterSave()
{
    ReaderAppearanceRepository::save(ReaderAppearanceDefaults::calmPalette());

    QSettings settings;
    int ver = settings.value("readerAppearance/schemaVersion", -1).toInt();
    QCOMPARE(ver, ReaderAppearanceRepository::SchemaVersion);
}

QTEST_MAIN(ReaderAppearanceRepositoryTest)
#include "ReaderAppearanceRepositoryTest.moc"
