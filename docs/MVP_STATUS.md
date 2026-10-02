# MVP Status

## Completed Milestones

| Milestone | Description |
|---|---|
| M0  | CMake + Qt6 Widgets scaffold |
| M1  | MainWindow with File > Open |
| M2  | MarkdownDocument model + DocumentLoader service |
| M3  | md4c HTML renderer + test fixtures |
| M4  | CTest setup (3 executables) |
| M5  | md_backpack_core static library target |
| M6  | Hygiene: PRIVATE md4c include/linkage |
| M7  | QTabWidget with placeholder page |
| M8  | YAML front-matter stripping + CRLF normalization |
| M9  | Status bar updates on tab events |
| M10 | Reader CSS stylesheet |
| M11 | Multi-file opening via File > Open |
| M12 | Drag-and-drop file opening |
| M13 | Command-line file opening |
| M14 | Global zoom (Ctrl++ / Ctrl+- / Ctrl+0) |
| M15 | Large-file guard (10 MB) |
| M16 | Recent Files (QSettings, max 10) |
| M17 | Windows portable package (windeployqt) |
| 18.1 | Reader Appearance architecture: settings struct, defaults, CSS builder |
| 18.1b | md_backpack_appearance static library target |
| 18.2 | ReaderStylesheetBuilder unit tests (4 cases) |
| 18.3 | ReaderAppearanceRepository with QSettings persistence + validation |
| 18.4 | ReaderAppearanceController (state owner, signal bus, 7 test cases) |
| 18.5 | MainWindow integration: controller ownership, signal wiring |
| 18.6a | ReaderAppearanceDialog UI skeleton (checkbox, color controls, buttons) |
| 18.6b | Dialog wired to MainWindow via Settings > Reader Appearance... |
| 18.7 | Clean build, CTest, packaging, deployed-app verification |
| M19 | Tables enabled via MD_FLAG_TABLES, table CSS with headingText / inlineCodeBackground / horizontalRule color roles, cell border |
| M20 | Fenced code blocks wrapped in `<table class="code-block-table"><td class="code-block-cell">` with language class preservation, codeBlockBackground cell border; inline code excluded from wrapping |
| M21 | Blockquotes wrapped in `<table class="blockquote-table"><td class="blockquote-bar"><td class="blockquote-cell">` with nested wrapper support |
| M22 | Horizontal rules: CSS refinements (border-top, margins) |
| M23 | Links open in system browser via `setOpenLinks(false)` + `anchorClicked` → `QDesktopServices::openUrl`; relative images from document base URL |
| M24 | HTML normalization: `<kbd>`→`<code>`, `<mark>`→inline-styled amber span, `<summary>`→`<p class="html-summary">`, `<details>` stripped, `<div>`→`<table class="html-block-table">` |
| M25 | Body font-family extended with emoji/symbol fallbacks (`'Segoe UI Emoji'`, `'Segoe UI Symbol'`, `'Noto Color Emoji'`, `'Apple Color Emoji'`) |
| M26 | Definition lists pre-processed from `Term\n: Description` pattern to `<table class="definition-table"><td class="definition-term"><td class="definition-description">` |
| M27 | RTL direction support: `applyRtlDirection()` detects Arabic Unicode (U+0600–U+08FF) in block elements and sets `dir="rtl" align="right"`; skips code blocks |
| M28 | Task lists enabled via MD_FLAG_TASKLISTS; `<input type="checkbox">` replaced with Unicode ☐ (unchecked) / ☑ (checked); `li.task-list-item { list-style-type: none; }` CSS; normal list items unchanged |

## Architecture

```
md_backpack_core         (static lib, Qt6::Core PUBLIC)
  DocumentLoader
  MarkdownPreprocessor
  MarkdownHtmlRenderer

md_backpack_appearance   (static lib, Qt6::Core PUBLIC, no Widgets dependency)
  ReaderAppearanceSettings
  ReaderAppearanceDefaults
  ReaderStylesheetBuilder
  ReaderAppearanceRepository
  ReaderAppearanceController

md-backpack-reader       (app executable, Qt6::Widgets)
  MainWindow
  ReaderAppearanceDialog (UI code, links Qt6::Widgets)

Test executables (6):
  DocumentLoaderTest
  MarkdownHtmlRendererTest
  MarkdownPreprocessorTest
  ReaderStylesheetBuilderTest
  ReaderAppearanceRepositoryTest
  ReaderAppearanceControllerTest
```

## Current Capabilities

- Open `.md`, `.markdown`, `.txt` files via File > Open, drag-and-drop, or CLI
- Multi-tab document viewer with closeable tabs, duplicate-tab prevention
- md4c-based Markdown-to-HTML rendering with the following feature support:
  - **Headings** (h1–h6) with size/weight hierarchy
  - **Paragraphs** with line-height: 1.55, bottom margin
  - **Inline styling**: bold (`<strong>`), italic (`<em>`/`<i>`), inline code (`<code>`)
  - **Lists**: unordered/ordered with `-qt-list-indent: 1`, nested indentation
  - **Tables**: pipe tables via `MD_FLAG_TABLES`, `<th>`/`<td>` with headingText/HR colors
  - **Fenced code blocks**: wrapped in `<table class="code-block-table"><td class="code-block-cell">` for background painting; language class preserved
  - **Blockquotes**: wrapped in `<table class="blockquote-table"><td class="blockquote-bar"><td class="blockquote-cell">` for border-bar + background; nested quotes supported
  - **Horizontal rules**: border-top style with configurable color, margins
  - **Links**: clickable links open in system default browser via `QDesktopServices::openUrl`; document-relative image paths resolved from file location
  - **Images**: rendered by QTextDocument; relative paths work when launched with a file path
  - **HTML inline tags**: `<kbd>`→`<code>`, `<mark>`→inline-styled amber span, `<sub>`/`<sup>` preserved; `<summary>`→`<p class="html-summary">`, `<details>` stripped, `<div>`→`<table class="html-block-table">`
  - **RTL direction**: Arabic text auto-detected; block elements receive `dir="rtl" align="right"`
  - **Definition lists**: `Term\n: Description` pattern converted to table-based layout with bold term
  - **Task lists**: `- [ ]`/`- [x]` rendered as ☐/☑ static markers; `li.task-list-item` removes default bullet
- YAML front-matter auto-stripping
- Global zoom (10% step, range -50% to +200%)
- Recent Files menu (persistent via QSettings, max 10)
- Large-file guard (10 MB, before read)
- Reader CSS styling (system font stack with emoji fallbacks)
- Plain-text fallback on render failure
- Status bar with path/zoom feedback
- Windows portable deployment via windeployqt
- **Reader Appearance Settings**: `Settings > Reader Appearance...`
  - 12 customizable color roles (page bg, main text, heading, muted, link, inline code, inline code bg, code block, code block bg, blockquote text, blockquote border, horizontal rule)
  - Apply / OK / Cancel / Restore Defaults behavior
  - Settings persisted in QSettings across app restarts
  - Updates all open tabs immediately on Apply/OK

## Test Status

- **CTest executables**: 11
- **Latest local verification**: 2026-05-24 via direct Qt test executables because `cmake`/`ctest` were not in PATH in the current shell
- **QtTest reported totals**: 148 passed — 0 failed, 0 skipped, 0 blacklisted
- **Test suites**:
  - `ComprehensiveRenderTest` — 6 passed
  - `DocumentLoaderTest` — 6 passed
  - `HtmlElementWrapperTest` — 6 passed
  - `HtmlNormalizerTest` — 8 passed
  - `MarkdownHtmlRendererTest` — 38 passed
  - `MarkdownPreprocessorTest` — 15 passed
  - `MarkdownRenderContractTest` — 10 passed
  - `ReaderAppearanceControllerTest` — 9 passed
  - `ReaderAppearanceRepositoryTest` — 7 passed
  - `ReaderStylesheetBuilderTest` — 38 passed
  - `TaskListNormalizerTest` — 5 passed
- **Status**: 11/11 passing — 148/148 QtTest reported passes

## Deployment Status

- Portable folder via `deployment/windows/package-windows.ps1`
- Output: `dist/windows/md-backpack-reader/`
- Qt DLLs bundled: Core, Gui, Widgets, Network, Svg + platform/plugin DLLs
- Launches from `dist/` without `PATH` modification
- Deployed-app verification completed: dialog, persistence, CSS, all features confirmed with dist-only DLLs
- No installer yet

## Known Limitations

1. **Windows only** — no Linux/macOS packaging yet
2. **No installer** — manual unzip/copy required
3. **No file association** — double-clicking `.md` in Explorer does not open in this app
4. **No async loading** — large files under 10 MB still block the UI during read/render
5. **No search inside document** — Ctrl+F not implemented
6. **No session restore** — open tabs are not saved between launches
7. **No full app theme system** — only reader appearance (colors of the rendered document area); main window menus, toolbars, and tab bar remain in the system style
8. **Microsoft Visual C++ Redistributable may be required** on clean Windows machines
9. **Zoom does not persist** between app launches
10. **Single-window only** — no support for multiple independent windows
11. **No custom Markdown syntax extensions** — md4c output is used as-is
12. **Max file size (10 MB)** is a compile-time constant — no user-facing settings UI for this value
13. **Wide tables overflow** — QTextDocument does not support horizontal scrolling or table auto-wrapping; very wide tables may be clipped
14. **`<details>` non-interactive** — QTextDocument cannot toggle disclosure widgets; `<details>` content is always visible
15. **`<kbd>`→`<code>` conversion** — keyboard input styling is not preserved; rendered as inline code
16. **`<mark>` hardcoded amber** — highlight color (#FFF3B0/#24292F) is hardcoded in the renderer, not configurable via appearance settings
17. **Emoji appearance OS/font-dependent** — emoji rendering quality depends on available system fonts; cross-platform consistency is not guaranteed
18. **No syntax highlighting** — fenced code blocks preserve the language class attribute but no syntax coloring is applied
19. **Definition-list inline code** is pre-processed manually (backtick→`<code>` in preprocessor), which may behave unexpectedly with nested markdown inside backticks
20. **Task-list markers are static Unicode** — ☐/☑ cannot be toggled interactively; they are visual-only
21. **`assets/images/sample-cover.png` is a test fixture** — it is bundled only for the development verification file (`tests/manual/comprehensive-visual-test.md`); it is not a required runtime asset. The dist package does not ship this file.

## Recommended Next Milestones

- In-document search (Ctrl+F)
- Session restore (save/open tab list)
- Windows file association (`.md` → double-click)
- NSIS or WiX installer
- Async/non-blocking file loading
- Settings UI (max file size, zoom default)
- Optional full app light/dark theme
