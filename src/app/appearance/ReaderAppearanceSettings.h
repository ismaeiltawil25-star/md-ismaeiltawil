#ifndef READERAPPEARANCESETTINGS_H
#define READERAPPEARANCESETTINGS_H

#include <QString>

struct ReaderAppearanceSettings
{
    bool customEnabled = false;

    QString pageBackground;
    QString mainText;
    QString headingText;
    QString mutedText;
    QString linkText;

    QString inlineCodeText;
    QString inlineCodeBackground;

    QString codeBlockText;
    QString codeBlockBackground;

    QString blockquoteText;
    QString blockquoteBorder;

    QString horizontalRule;

    friend bool operator==(const ReaderAppearanceSettings &a,
                           const ReaderAppearanceSettings &b)
    {
        return a.customEnabled == b.customEnabled
            && a.pageBackground == b.pageBackground
            && a.mainText == b.mainText
            && a.headingText == b.headingText
            && a.mutedText == b.mutedText
            && a.linkText == b.linkText
            && a.inlineCodeText == b.inlineCodeText
            && a.inlineCodeBackground == b.inlineCodeBackground
            && a.codeBlockText == b.codeBlockText
            && a.codeBlockBackground == b.codeBlockBackground
            && a.blockquoteText == b.blockquoteText
            && a.blockquoteBorder == b.blockquoteBorder
            && a.horizontalRule == b.horizontalRule;
    }

    friend bool operator!=(const ReaderAppearanceSettings &a,
                           const ReaderAppearanceSettings &b)
    {
        return !(a == b);
    }
};

#endif
