# Third-party notices

MD Backpack Reader is MIT-licensed (see `LICENSE`). It uses the following
open-source components, which keep their own licenses.

## Qt 6 (The Qt Company Ltd. and contributors)

- License: GNU Lesser General Public License v3 (LGPL-3.0-only)
- Source: <https://download.qt.io/official_releases/qt/> and <https://code.qt.io>
- Used modules: Qt Core, Gui, Widgets, Network, PrintSupport

Qt is **dynamically linked**: the `Qt6*.dll` files shipped next to
`md-backpack-reader.exe` are unmodified Qt binaries. You may replace them with
your own compatible build of the same Qt version. The full LGPL-3.0 text is at
<https://www.gnu.org/licenses/lgpl-3.0.html>.

## md4c (Martin Mitáš and contributors)

- License: MIT
- Source: <https://github.com/mity/md4c> (release 0.5.2, built into the program)

## Compiler runtime

- MSVC builds ship Microsoft Visual C++ runtime DLLs under the Visual Studio
  redistribution terms.
- MinGW builds ship GCC runtime DLLs (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`,
  `libwinpthread-1.dll`) under the GCC Runtime Library Exception.
