#ifndef READERAPPEARANCEDEFAULTS_H
#define READERAPPEARANCEDEFAULTS_H

#include "ReaderAppearanceSettings.h"

namespace ReaderAppearanceDefaults
{

inline ReaderAppearanceSettings calmPalette()
{
    ReaderAppearanceSettings s;
    s.pageBackground     = QStringLiteral("#ffffff");
    s.mainText           = QStringLiteral("#24292f");
    s.headingText        = QStringLiteral("#24292f");
    s.mutedText          = QStringLiteral("#57606a");
    s.linkText           = QStringLiteral("#0969da");
    s.inlineCodeText     = QStringLiteral("#24292f");
    s.inlineCodeBackground = QStringLiteral("#f6f8fa");
    s.codeBlockText      = QStringLiteral("#24292f");
    s.codeBlockBackground  = QStringLiteral("#f6f8fa");
    s.blockquoteText     = QStringLiteral("#57606a");
    s.blockquoteBorder   = QStringLiteral("#d0d7de");
    s.horizontalRule     = QStringLiteral("#d0d7de");
    return s;
}

inline ReaderAppearanceSettings dayPalette()
{
    ReaderAppearanceSettings s;
    s.customEnabled        = true;
    s.pageBackground       = QStringLiteral("#FBFAF7");
    s.mainText             = QStringLiteral("#24292F");
    s.headingText          = QStringLiteral("#1F2933");
    s.mutedText            = QStringLiteral("#6B7280");
    s.linkText             = QStringLiteral("#2563EB");
    s.inlineCodeText       = QStringLiteral("#1F2933");
    s.inlineCodeBackground = QStringLiteral("#EEF2F7");
    s.codeBlockText        = QStringLiteral("#1F2933");
    s.codeBlockBackground  = QStringLiteral("#F3F5F7");
    s.blockquoteText       = QStringLiteral("#64748B");
    s.blockquoteBorder     = QStringLiteral("#CBD5E1");
    s.horizontalRule       = QStringLiteral("#D8DEE4");
    return s;
}

inline ReaderAppearanceSettings nightPalette()
{
    ReaderAppearanceSettings s;
    s.customEnabled        = true;
    s.pageBackground       = QStringLiteral("#0F141A");
    s.mainText             = QStringLiteral("#D6DEE8");
    s.headingText          = QStringLiteral("#EEF3F8");
    s.mutedText            = QStringLiteral("#8B98A7");
    s.linkText             = QStringLiteral("#7AA2F7");
    s.inlineCodeText       = QStringLiteral("#E6EDF3");
    s.inlineCodeBackground = QStringLiteral("#1B2430");
    s.codeBlockText        = QStringLiteral("#DDE6F0");
    s.codeBlockBackground  = QStringLiteral("#151B23");
    s.blockquoteText       = QStringLiteral("#A3AFBF");
    s.blockquoteBorder     = QStringLiteral("#3A4656");
    s.horizontalRule       = QStringLiteral("#303A46");
    return s;
}

} // namespace ReaderAppearanceDefaults

#endif
