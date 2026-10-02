// Source hygiene — rules for production code that the compiler cannot check.
//
// Rule: string and character literals in src/ are pure ASCII. Non-ASCII text
// must be written with escapes (u"\u2611", u"\U0001F604"). Raw characters
// depend on the compiler's source-encoding settings, and invisible ones
// (U+FEFF, U+200F, ...) can be silently deleted by editors.
// Comments and raw string literals R"(...)" are exempt.

#include <QDirIterator>
#include <QFile>
#include <QTest>

namespace {

// Returns "line:column" entries for non-ASCII characters inside literals.
QStringList findNonAsciiLiterals(const QString &source)
{
    QStringList hits;
    const qsizetype n = source.size();
    qsizetype i = 0;
    int line = 1;
    qsizetype lineStart = 0;

    const auto newline = [&](qsizetype at) { ++line; lineStart = at + 1; };

    while (i < n) {
        const QChar c = source[i];
        if (c == u'\n') { newline(i); ++i; continue; }

        if (source.mid(i, 2) == QStringLiteral("//")) {
            while (i < n && source[i] != u'\n') ++i;
            continue;
        }
        if (source.mid(i, 2) == QStringLiteral("/*")) {
            const qsizetype end = source.indexOf(QStringLiteral("*/"), i + 2);
            const qsizetype stop = end < 0 ? n : end + 2;
            for (qsizetype k = i; k < stop; ++k)
                if (source[k] == u'\n') newline(k);
            i = stop;
            continue;
        }
        if (c == u'R' && i + 1 < n && source[i + 1] == u'"') {
            const qsizetype open = source.indexOf(u'(', i + 2);
            const QString delimiter = source.mid(i + 2, open - i - 2);
            const qsizetype end = source.indexOf(u')' + delimiter + u'"', open);
            const qsizetype stop = end < 0 ? n : end + delimiter.size() + 2;
            for (qsizetype k = i; k < stop; ++k)
                if (source[k] == u'\n') newline(k);
            i = stop;
            continue;
        }
        if (c == u'"' || c == u'\'') {
            const QChar quote = c;
            ++i;
            while (i < n && source[i] != quote && source[i] != u'\n') {
                if (source[i] == u'\\') { i += 2; continue; }
                if (source[i].unicode() > 0x7F)
                    hits << QString("%1:%2").arg(line).arg(i - lineStart + 1);
                ++i;
            }
            ++i;
            continue;
        }
        ++i;
    }
    return hits;
}

} // namespace

class SourceHygieneTest : public QObject
{
    Q_OBJECT

private slots:
    void detectorFindsRawCharacters();
    void productionLiteralsAreAscii();
};

void SourceHygieneTest::detectorFindsRawCharacters()
{
    QCOMPARE(findNonAsciiLiterals(QStringLiteral(u"auto a = u\"\u2611\";")).size(), 1);
    QCOMPARE(findNonAsciiLiterals(QStringLiteral(u"auto a = u\"\\u2611\"; // \u2611")).size(), 0);
    QCOMPARE(findNonAsciiLiterals(QStringLiteral(u"auto r = R\"(\u2611)\";")).size(), 0);
}

void SourceHygieneTest::productionLiteralsAreAscii()
{
    QStringList violations;
    QDirIterator it(QStringLiteral(SOURCE_DIR "/src"), {"*.cpp", "*.h"}, QDir::Files,
                    QDirIterator::Subdirectories);
    int scanned = 0;
    while (it.hasNext()) {
        const QString path = it.next();
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QStringList hits = findNonAsciiLiterals(QString::fromUtf8(file.readAll()));
        for (const QString &hit : hits)
            violations << QString("%1:%2").arg(path.mid(QStringLiteral(SOURCE_DIR).size() + 1), hit);
        ++scanned;
    }
    QVERIFY(scanned > 50);
    QVERIFY2(violations.isEmpty(),
             qPrintable("Raw non-ASCII characters in string literals (use \\u escapes):\n  "
                        + violations.join("\n  ")));
}

QTEST_APPLESS_MAIN(SourceHygieneTest)
#include "SourceHygieneTest.moc"
