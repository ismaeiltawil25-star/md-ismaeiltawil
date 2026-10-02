#pragma once

// SyntaxLanguages — declarative rules for SyntaxHighlighter.

#include <QSet>
#include <QString>
#include <QStringList>

namespace mdbr::render {

struct LanguageRules
{
    QStringList names;          // language tag and aliases, lower case
    QSet<QString> keywords;
    QSet<QString> types;        // built-in types / literals coloured as types
    QStringList lineComments;   // e.g. "//", "#", "--"
    QString blockCommentOpen;   // e.g. "/*"
    QString blockCommentClose;  // e.g. "*/"
    QString stringQuotes;       // characters that open a string, e.g. "\"'`"
    bool caseInsensitive = false; // SQL
    bool hashPreprocessor = false; // C family: "#include ..." lines
    bool markup = false;          // HTML/XML: <tags> coloured as keywords
    bool dashInWords = false;     // PowerShell cmdlets: Get-ChildItem is one word
};

// Rules for a language tag (case-insensitive), or nullptr if unknown.
[[nodiscard]] const LanguageRules *findLanguage(const QString &language);

} // namespace mdbr::render
