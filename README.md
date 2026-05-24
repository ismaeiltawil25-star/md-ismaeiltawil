# MD Backpack Reader

A lightweight Markdown reader for Windows built with Qt6 Widgets and C++20.
Renders `.md` files as HTML in tabs using the **md4c** parser (no WebEngine).

## MVP Capabilities

- Open `.md`, `.markdown`, `.txt` files via File > Open, drag-and-drop, or command line
- Render Markdown to HTML with md4c
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

Latest verified local baseline: 11 test executables, 148 QtTest reported passes, 0 failures, 0 skipped.

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
  app/
    MainWindow.h/.cpp       Main window (tabs, menus, zoom, DnD, recent files)
    appearance/
      ReaderAppearanceSettings.h     12-role color settings struct
      ReaderAppearanceDefaults.h     Default calm palette
      ReaderStylesheetBuilder.h/.cpp Generates CSS from settings
      ReaderAppearanceRepository.h/.cpp QSettings persistence
      ReaderAppearanceController.h/.cpp State owner + signal bus
      ReaderAppearanceDialog.h/.cpp  Settings UI dialog
  models/
    MarkdownDocument.h       Plain data struct (header-only)
  services/
    DocumentLoader.h/.cpp   File I/O with size guard
    MarkdownPreprocessor.h/.cpp  YAML front-matter stripper
  renderers/
    MarkdownHtmlRenderer.h/.cpp  md4c-based HTML renderer
tests/
  CMakeLists.txt            Test build config
  data/                     Test fixtures
  unit/                     Unit tests (6 executables)
deployment/windows/
  package-windows.ps1       windeployqt packaging script
docs/
  DEPLOYMENT_WINDOWS.md     Detailed deployment guide
  MVP_STATUS.md             Current status and known limitations
CMakeLists.txt              Root build definition
```
