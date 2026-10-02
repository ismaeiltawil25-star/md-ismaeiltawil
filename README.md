# MD Backpack Reader

A lightweight Markdown reader for Windows built with Qt6 Widgets and C++20.
Renders `.md` files as HTML in tabs using the **md4c** parser (no WebEngine).

## MVP Capabilities

- Open Markdown and text files via File > Open, drag-and-drop, or command line:
  `.md` `.markdown` `.mdown` `.mdwn` `.mkd` `.mkdn` `.mkdown` `.mdtxt` `.mdtext`,
  dialects `.rmd` `.qmd` `.mdx` `.livemd` `.ronn`, and plain text `.txt` `.text`
  (the single list is in `src/services/SupportedFileTypes.cpp`)
- Automatic encoding detection: UTF-8, UTF-16/32 (with or without BOM), legacy
  Windows-1256 (Arabic) and Windows-1252; the status bar says when the encoding was guessed
- Render GitHub-flavored Markdown with md4c: tables, task lists, ~~strikethrough~~,
  automatic links for bare URLs
- Extended syntax: footnotes, GitHub alerts (`> [!NOTE]`), math (`$..$`, `$$..$$`),
  `==highlight==`, `^superscript^`, `:emoji:` shortcodes, `[[wiki links]]`,
  heading anchors for `#section` links, and syntax highlighting for 20 languages
- Printing and print preview for every document (Markdown, text, PDF) from the toolbar
  or Ctrl+P: same paper, margins and page breaks as page view, dark ink on white paper
  in any theme, page numbers, page ranges
- Clickable links: `#section` scrolls the document, and links to other Markdown files
  open them in a new tab
- Right-to-left layout for Arabic/Hebrew paragraphs, lists and quotes
- Optional page view (Preferences > Page Layout): documents are split into
  printed pages (A4, A5, Letter, Legal) with margins and gaps, like a word
  processor's print layout. Works in day, night and custom themes
- Linear-time rendering: a 1 MB document renders in well under a second, and
  deeply nested input cannot freeze the app
- Automatic YAML front-matter stripping before rendering
- Multi-tab interface with closeable tabs
- Duplicate-tab prevention (re-focuses existing tab)
- Global zoom (Ctrl++ / Ctrl+- / Ctrl+0)
- Recent Files menu (persisted via QSettings, max 10 entries)
- Large-file guard (10 MB limit before reading)
- Status bar with file path and zoom feedback
- Reader-friendly CSS styling
- Reader-mode fallback when md4c rendering fails
- **Themes**: Follow Windows (switches live with Windows light/dark), Day, Night or
  Custom colors. Toggle night/day from the toolbar moon button or Ctrl+Shift+N, or pick
  a mode in *View > Theme*
- **Preferences** (*Settings > Preferences...*, Ctrl+,): one window with the
  *Appearance* section (four theme cards with previews; custom mode needs only
  background, text and link colors, with *More colors* for all 12 roles) and the
  *Page Layout* section (page view, paper, margins, also used for printing)

## Download

Releases for Windows are published on the
[Releases page](https://github.com/ismaeiltawil25-star/md-ismaeiltawil/releases).
Each release is a portable ZIP: extract it anywhere and run
`md-backpack-reader.exe`. Nothing is installed or changed on the system, except
the reader's own settings in the registry (see *Uninstall*).

A clean Windows machine may also need the
[Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe).

### Code signing policy

Free code signing provided by SignPath.io, certificate by SignPath Foundation.
See the full [code signing policy](docs/CODE_SIGNING_POLICY.md) for the team
roles and the privacy policy.

### Uninstall

- **Portable ZIP:** delete the extracted folder. To also remove the reader's
  settings (recent files, colours, page layout), delete the registry key
  `HKEY_CURRENT_USER\Software\MD Backpack Reader`.
- **Installer:** use *Settings > Apps > Installed apps > MD Backpack Reader >
  Uninstall*, or the *Uninstall* shortcut in the Start menu.

### Privacy

The reader works offline. It does not collect or send any data. Web links
open in your browser only when you click them.

## License

MIT, see [LICENSE](LICENSE). Third-party components (Qt under LGPL-3.0, md4c
under MIT) are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Build Requirements

- Windows (MSVC 2019/2022)
- Qt 6.7.0 (64-bit, MSVC 2019 build)
- CMake 3.20+
- Visual Studio 2022 build tools

## Configure and Build

```powershell
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64"
cmake --build build --config Release
```

## Run Tests

Preferred, when CMake tools are available in `PATH`:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Local Windows fallback when `ctest` is not in `PATH` but the Release test executables already exist:

```powershell
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_PLUGIN_PATH = "C:\Qt\6.7.0\msvc2019_64\plugins"
$env:PATH = "C:\Qt\6.7.0\msvc2019_64\bin;$env:PATH"

$outDir = Join-Path $env:TEMP "md-backpack-reader-tests"
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
Get-ChildItem ".\build\tests\Release\*Test.exe" | ForEach-Object {
    $outFile = Join-Path $outDir "$($_.BaseName).txt"
    & $_.FullName -o $outFile
    Get-Content $outFile | Select-String "^Totals:"
}
```

Latest verified baseline (2026-10-02, MinGW 11.2 with warnings as errors):
54 test executables, about 1,100 QtTest checks, 0 failures.

For the strict build, the test layers and the coding rules, see
[docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) and [CONTRIBUTING.md](CONTRIBUTING.md).

## Windows Portable Package

```powershell
.\deployment\windows\package-windows.ps1
```

This creates `dist/windows/md-backpack-reader/` with all required Qt DLLs.

## Run Deployed App

```powershell
.\dist\windows\md-backpack-reader\md-backpack-reader.exe
.\dist\windows\md-backpack-reader\md-backpack-reader.exe .\tests\data\sample.md
```

No manual PATH modification needed. A clean Windows machine may require the
[Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Project Structure

```
src/
  main.cpp                  App entry point
  app/                      MainWindow, single-instance service
    appearance/             Reader colors: settings, CSS builder, persistence, dialog
    document/               Document open pipeline and tab page/state
  models/                   Plain data structs (MarkdownDocument)
  services/                 File loading, Markdown preprocessing facade
    text/                   Encoding detection and decoding
    markdown/               Front matter, definition lists, code-fence tracking
    pdf/                    PDF file preview service
  renderers/                md4c HTML renderer + HTML post-processing stages
  pdf/                      Custom PDF engine (parsing, content streams)
    rendering/              Page image / text / path rendering
  widgets/                  PDF preview widget, controller, zoom toolbar
  utils/                    Error logger, performance monitor
tests/                      Unit tests, fixtures, diagnostics, manual checks (see tests/README.md)
assets/                     App icons and images (icons/source/ holds the original icon pack)
deployment/windows/         Packaging script, installer (Inno Setup), Windows resources
docs/                       Documentation (see docs/README.md)
cmake/                      Shared CMake modules (compiler warnings)
CMakeLists.txt              Root build definition
CONTRIBUTING.md             Workflow and definition of done
```

Generated folders (ignored by git): `build/`, `dist/`, `Testing/`.
