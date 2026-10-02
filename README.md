# MD Backpack Reader

A lightweight Markdown reader for Windows built with Qt6 Widgets and C++20.
Renders `.md` files as HTML in tabs using the **md4c** parser (no WebEngine).

## MVP Capabilities

- Open `.md`, `.markdown`, `.txt` files via File > Open, drag-and-drop, or command line
- Automatic encoding detection: UTF-8, UTF-16/32 (with or without BOM), legacy
  Windows-1256 (Arabic) and Windows-1252; the status bar says when the encoding was guessed
- Render GitHub-flavored Markdown with md4c: tables, task lists, ~~strikethrough~~,
  automatic links for bare URLs
- Right-to-left layout for Arabic/Hebrew paragraphs, lists and quotes
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
- **Reader Appearance Settings**: customize colors via `Settings > Reader Appearance...`
  - Enable `Use custom reader colors`
  - Adjust 12 color roles (page background, main text, heading, link, code, blockquote, horizontal rule)
  - **Apply**: saves immediately and updates all open tabs
  - **OK**: applies final settings and closes the dialog
  - **Cancel**: discards unapplied dialog changes (previously Applied settings survive)
  - **Restore Defaults**: resets dialog to the calm palette (only takes effect after Apply/OK)
  - Settings persisted in QSettings across app restarts

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
