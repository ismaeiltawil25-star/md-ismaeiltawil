# Developer Guide

Start here before changing code. This guide covers how a Markdown file becomes
what the reader shows, where each responsibility lives, the coding rules, and
how errors are caught early. Architecture principles (SRP, pipeline stages) are
in [ARCHITECTURE_GUIDELINES.md](ARCHITECTURE_GUIDELINES.md).

---

## 1. Markdown pipeline

Every Markdown file goes through these stages, in this order. Each stage has
exactly one responsibility and its own tests.

```
file bytes
  │  DocumentLoader              services/DocumentLoader.cpp        size guard, read raw bytes
  ▼
  │  mdbr::text::decode          services/text/TextDecoder.cpp       bytes → Unicode (encoding detection)
  │  normalizeLineEndings                                            CRLF / CR → LF
  ▼
  │  stripFrontMatter            services/markdown/FrontMatterStripper.cpp     remove YAML header
  │  convertDefinitionLists      services/markdown/DefinitionListConverter.cpp Term / ": desc" → table
  │      └─ both use FenceTracker  services/markdown/FenceTracker.cpp  never rewrite fenced code
  ▼
  │  md4c (MD_DIALECT_GITHUB)    renderers/MarkdownHtmlRenderer.cpp  Markdown → HTML
  │  wrapBlockquotes             renderers/HtmlElementWrapper.cpp    tables for quote bar
  │  wrapCodeBlocks              renderers/HtmlElementWrapper.cpp    tables for code background
  │  normalizeRawTags            renderers/HtmlNormalizer.cpp        HTML5 → QTextDocument subset
  │  applyRtl                    renderers/HtmlRtlProcessor.cpp      dir="rtl" on Arabic/Hebrew blocks
  │  swapBlockquoteOrderForRtl   renderers/HtmlElementWrapper.cpp    quote bar on the right for RTL
  │  normalizeTaskLists          renderers/TaskListNormalizer.cpp    checkboxes → ☐ / ☑
  ▼
QTextBrowser (DocumentTabPage)   app/document/DocumentTabPage.cpp
```

`DocumentOpenPipeline` (app/document/) is the single entry point that runs
all stages for the UI. If rendering fails, the tab falls back to plain text.

### Why the HTML is rewritten after md4c

The reader displays HTML with `QTextBrowser`, which understands only a
[subset of HTML 4 and CSS](https://doc.qt.io/qt-6/richtext-html-subset.html).
The stages after md4c translate modern HTML into that subset: tables instead
of CSS borders, `<s>` instead of `<del>`, and so on. When you add Markdown
features, check that QTextDocument actually renders the tag.
`RenderedDocumentContractTest` exists for exactly this.

---

## 2. Folder responsibilities

| Folder | Owns | Must not depend on |
|---|---|---|
| `src/services/text/` | Byte decoding, line endings | Markdown, Qt Gui/Widgets |
| `src/services/markdown/` | Text-level Markdown preprocessing | HTML, Qt Gui/Widgets |
| `src/services/` | File loading, preprocessing facade | Qt Widgets |
| `src/renderers/` | Markdown → HTML stages | Qt Widgets |
| `src/models/` | Plain data structs | anything with behaviour |
| `src/app/` | Windows, tabs, menus, settings UI | — |
| `src/pdf/`, `src/widgets/` | Custom PDF engine and preview | Markdown pipeline |
| `src/utils/` | Logging, performance timing | — |

The `md_backpack_core` library links only `Qt6::Core`. Keep it that way so
everything in it can be tested without a GUI.

---

## 3. Coding rules

**Language:** C++20 with Qt 6.

Rules for new code (existing code is migrated when it is touched):

- **Namespaces:** new modules go in `mdbr::<area>` (`mdbr::text`,
  `mdbr::markdown`). Use free functions for stateless transformations and a
  class only when there is state (e.g. `FenceTracker`).
- **Headers:** `#pragma once`. Each header starts with a comment that
  explains the unit's single responsibility and its rules.
- **`[[nodiscard]]`** on every function whose result must be used.
- **Use `QStringView`** for read-only text parameters and slicing. Never call
  `QString::mid()` inside a loop over a large document, because it copies.
- **Non-ASCII literals use Unicode escapes:** write `u"\u2611"`, not
  `"\xe2\x98\x91"` and not a raw `☑`. Byte escapes inside `QStringLiteral`
  produced garbage on GCC while working on MSVC (fixed in commit `0b75396`).
- **Linear time:** every HTML/Markdown pass must be O(n). Use a single scan
  with a stack for nesting, and collect edits to apply once at the end. Two
  quadratic stages froze the app on deep nesting; `RenderRobustnessTest` now
  guards against this.
- **Cap nested tables.** QTextDocument's layout time roughly doubles with each
  level of table nesting: depth 14 takes about 0.8 s, and 20 nested quotes
  froze the app. Wrappers that emit tables must cap their depth and fall back
  to plain tags. Current caps: `HtmlElementWrapper::kMaxBlockquoteTableDepth`
  (5) and `HtmlNormalizer::kMaxDivTableDepth` (3). `deepNestingLaysOutQuickly`
  in `RenderedDocumentContractTest` checks render plus layout end to end.
- **Text preprocessors must use `FenceTracker`,** so they never rewrite code.
- **Never drop content silently.** If input is ambiguous, keep it visible
  (see `FrontMatterStripper`: prose between `---` lines is not front matter).

### Adding a pipeline stage: checklist

1. Create one unit (header + source) in the right folder, with a header
   comment explaining what it does and what it refuses to do.
2. Add it to the `md_backpack_core` source list in `CMakeLists.txt`.
3. Add it to `MarkdownHtmlRenderer::render()` (HTML stage) or
   `DocumentOpenPipeline::open()` (text stage), plus a `RenderStage` enum value
   for diagnostics.
4. Write unit tests for the unit itself.
5. Add a `RenderedDocumentContractTest` case for what the user should see.
6. Add a pathological row to `RenderRobustnessTest` if the stage scans nesting.
7. Update the diagram in section 1.

---

## 4. Early error detection

Errors are caught in layers, from fastest feedback to slowest:

| Layer | Catches | Where |
|---|---|---|
| Compiler warnings | Unused code, shadowing, null dereference, bad comparisons | `cmake/CompilerWarnings.cmake` |
| Unit tests | Logic errors inside one stage | `tests/unit/` |
| Rendered-document contract | HTML that Qt does not render as intended | `RenderedDocumentContractTest` |
| Fuzzing | Crashes or failures on unexpected input | `RenderRobustnessTest` (seeded, reproducible) |
| Time budgets | Quadratic or worse slowdowns, freezes | `RenderRobustnessTest` |
| CI (two compilers) | Compiler-specific behaviour | `.github/workflows/ci.yml` |

`MDBR_WARNINGS_AS_ERRORS=ON` makes the build fail on any warning. CI's MinGW
job enables it.

When a fuzz test fails, it prints the seed and document index. Re-run with the
same seed (`kFuzzSeed`) to reproduce, then add the input as a regular unit test.

---

## 5. Building and testing

### MSVC (release toolchain)

```powershell
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### MinGW (strict, same as CI)

```powershell
cmake -B build-mingw -S . -G Ninja -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\mingw_64" -DMDBR_WARNINGS_AS_ERRORS=ON
cmake --build build-mingw
$env:QT_QPA_PLATFORM = "offscreen"
ctest --test-dir build-mingw --output-on-failure
```

### Windows Smart App Control

If Smart App Control is on, Windows may refuse to start freshly built,
unsigned test executables. ctest then reports them as `BAD_COMMAND`, or they
exit with code 126. This is not a test failure. Run the tests in CI, or on a
development machine where your own builds are allowed to run.

---

## 6. Known limitations and roadmap

These features are not supported yet. Each one needs its own unit and tests:

| Feature | Notes |
|---|---|
| Footnotes `[^1]` | md4c does not support them; needs a preprocessing stage or a parser change |
| Math `$...$` | md4c can emit spans (`MD_FLAG_LATEXMATHSPANS`); rendering needs a math engine |
| Syntax highlighting | Code blocks keep `class="language-x"`; a highlighter stage could color them |
| Mermaid diagrams | Out of scope for QTextBrowser |
| Remote images (`http://`) | QTextBrowser loads only local resources |
| Async loading | Large files block the UI while loading |
| Encoding override | A menu to re-open a file with a chosen encoding when the guess is wrong |
