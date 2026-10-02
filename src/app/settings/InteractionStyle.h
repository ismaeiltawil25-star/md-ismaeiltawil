#pragma once

// InteractionStyle — clear hover / selected / checked states in light AND dark
// themes.
//
// Qt's native Windows style draws some states with very little contrast in
// dark mode (a checked toolbar button became pale blue with white text; menu
// hover was barely visible). This unit derives those colours from the current
// palette and enforces WCAG contrast:
//   * text on any highlighted background: at least 4.5 : 1
//   * hover / checked background against the window: at least 1.4 : 1, so the
//     state is visible at a glance
// The result is an application style sheet that only touches these states.

#include <QColor>
#include <QPalette>
#include <QString>

namespace mdbr::settings {

struct InteractionColors
{
    QColor window;        // normal background
    QColor text;          // normal text
    QColor hover;         // background under the mouse
    QColor checked;       // background of a checked/toggled button
    QColor checkedBorder; // outline of a checked button
    QColor selection;     // selected menu item / list row
    QColor selectionText; // text on `selection`
};

[[nodiscard]] InteractionColors interactionColors(const QPalette &palette);

[[nodiscard]] QString interactionStyleSheet(const QPalette &palette);

} // namespace mdbr::settings
