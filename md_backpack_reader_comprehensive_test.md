---
title: "MD Backpack Reader — Comprehensive Markdown Rendering Test"
author: "Test Fixture"
tags: [markdown, renderer, md4c, qt, reader, visual-test]
created: 2026-05-22
description: "A broad Markdown file designed to test headings, lists, tables, code, quotes, links, symbols, RTL/LTR text, and visual hierarchy."
---

# MD Backpack Reader — Comprehensive Markdown Test

This file is designed to test how a Markdown reader renders many common document structures: headings, paragraphs, lists, tables, code blocks, links, images, task lists, blockquotes, Unicode symbols, HTML blocks, and mixed Arabic/English text.

> Expected behavior: the YAML front matter above should be hidden if front-matter stripping is enabled.

---

## 1. Heading Hierarchy

# H1 — Main Document Title

## H2 — Major Section

### H3 — Subsection

#### H4 — Minor Topic

##### H5 — Small Topic

###### H6 — Smallest Heading

Regular paragraph after headings. This paragraph contains **bold**, *italic*, ***bold italic***, ~~strikethrough if supported~~, `inline code`, and a [link to Qt](https://www.qt.io/).

---

## 2. Paragraphs and Inline Styling

This is a normal paragraph with multiple inline elements. The reader should keep the line height comfortable and the text clear.

This paragraph includes:
- **Strong importance**
- *Emphasis / muted text*
- `inline_code_sample()`
- A URL: <https://example.com>
- An email: <reader@example.com>
- Escaped Markdown characters: \*not italic\*, \`not code\`, \# not heading

Arabic mixed with English:

> هذا نص عربي داخل ملف Markdown، ويحتوي على كلمات English و `inline code` ورابط [مثال](https://example.com).  
> الهدف هو اختبار الاتجاه البصري وتناسق الخط داخل القارئ.

---

## 3. Lists

### 3.1 Unordered List

- First item
- Second item
  - Nested item A
  - Nested item B
    - Deep nested item
- Third item with `inline code`
- Fourth item with **bold** and *italic*

### 3.2 Ordered List

1. Install dependencies
2. Configure CMake
3. Build Release
4. Run tests
5. Package the app
6. Verify the deployed executable

### 3.3 Mixed List

1. Core features
   - Markdown rendering
   - YAML stripping
   - Multi-tab reading
2. User interaction
   - File > Open
   - Drag-and-drop
   - Command-line file opening
3. Reader appearance
   - Day preset
   - Night preset
   - Custom semantic colors

### 3.4 Task List

- [x] Render headings
- [x] Render tables
- [x] Render code blocks
- [ ] Add in-document search
- [ ] Add installer and file association
- [ ] Add session restore

---

## 4. Tables

### 4.1 Basic Table

| Feature | Status | Notes |
|---|---|---|
| Markdown rendering | Done | Uses md4c |
| YAML front matter | Done | Hidden before rendering |
| Tabs | Done | Closeable tabs |
| Recent files | Done | Stored with QSettings |
| Reader colors | Done | Semantic color roles |

### 4.2 Alignment Table

| Left aligned | Center aligned | Right aligned |
|:---|:---:|---:|
| Alpha | 10% | 120 |
| Beta | 50% | 2,430 |
| Gamma | 100% | 98,201 |

### 4.3 Wide Technical Table

| Module | Responsibility | Input | Output | Risk |
|---|---|---|---|---|
| `DocumentLoader` | Reads file safely | path | `MarkdownDocument` | large files |
| `MarkdownPreprocessor` | Removes YAML front matter | raw text | clean markdown | malformed delimiters |
| `MarkdownHtmlRenderer` | Converts Markdown to HTML | markdown | HTML | parser failure |
| `ReaderStylesheetBuilder` | Builds semantic CSS | appearance settings | CSS strings | contrast mismatch |
| `ReaderAppearanceRepository` | Persists colors | settings | QSettings values | corrupted values |

---

## 5. Code Blocks

### 5.1 PowerShell

```powershell
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64"
cmake --build build --config Release --clean-first
ctest --test-dir build -C Release --output-on-failure
.\deployment\windows\package-windows.ps1
.\dist\windows\md-backpack-reader\md-backpack-reader.exe README.md
```

### 5.2 C++

```cpp
#include <QApplication>
#include "app/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
```

### 5.3 JSON

```json
{
  "readerAppearance": {
    "customEnabled": true,
    "pageBackground": "#0F141A",
    "mainText": "#D6DEE8",
    "headingText": "#EEF3F8",
    "linkText": "#7AA2F7"
  }
}
```

### 5.4 YAML

```yaml
reader:
  mode: night
  colors:
    pageBackground: "#0F141A"
    mainText: "#D6DEE8"
    headingText: "#EEF3F8"
```

### 5.5 SQL

```sql
SELECT file_path, opened_at
FROM recent_files
ORDER BY opened_at DESC
LIMIT 10;
```

### 5.6 Plain Text / ASCII Diagram

```text
QSettings
   ↓
ReaderAppearanceRepository
   ↓
ReaderAppearanceController
   ↓
ReaderStylesheetBuilder
   ↓
QTextBrowser
```

---

## 6. Blockquotes

> A normal blockquote should have a distinct border and readable muted text.
>
> It may contain **bold text**, *italic text*, `inline code`, and a [link](https://example.com).

Nested blockquote:

> Level 1
>
> > Level 2
> >
> > > Level 3

---

## 7. Horizontal Rules

Above this text is a normal paragraph.

---

Below this line, the horizontal rule should be visually subtle and not confused with a code block border.

---

## 8. Links and Images

### 8.1 Links

- External link: [Qt Documentation](https://doc.qt.io/)
- Plain URL: <https://commonmark.org/>
- Email: <support@example.com>

### 8.2 Image Syntax

The reader may show a broken image if the file is missing; that is acceptable for path-resolution testing.

![Relative image test](assets/images/sample-cover.png)

---

## 9. HTML Inside Markdown

Some Markdown files contain raw HTML. A reader may render or ignore parts depending on its HTML support.

<div style="border: 1px solid #888; padding: 8px;">
  <strong>HTML block:</strong> This is a raw HTML div with inline styling.
</div>

Inline HTML: <kbd>Ctrl</kbd> + <kbd>O</kbd>, <mark>highlighted text if supported</mark>, H<sub>2</sub>O, E = mc<sup>2</sup>.

<details>
<summary>Collapsible details if supported</summary>

This content may or may not collapse depending on the renderer.

</details>

---

## 10. Symbols, Emojis, and Unicode

### 10.1 Symbols

- Arrows: → ← ↑ ↓ ⇒ ⇐
- Math-like symbols: ± × ÷ ≈ ≠ ≤ ≥ ∞
- Currency: $ € ₪ ¥ £
- Check marks: ✓ ✔ ✗ ✘
- Stars: ★ ☆ ✦ ✧

### 10.2 Emojis

- Status: ✅ ❌ ⚠️ ℹ️
- Reading: 📖 📝 🔍
- Tools: 🛠️ ⚙️ 🧪
- UI: 🌙 ☀️ 🎨

---

## 11. Data Representation Examples

### 11.1 Progress Matrix

| Stage | Build | Tests | Package | Manual Verify |
|---|:---:|:---:|:---:|:---:|
| M0 | ✅ | — | — | ✅ |
| M4 | ✅ | ✅ | — | ✅ |
| M17 | ✅ | ✅ | ✅ | ✅ |
| M18 | ✅ | ✅ | ✅ | ✅ |

### 11.2 Risk Register

| Risk ID | Risk | Probability | Impact | Mitigation |
|---|---|---:|---:|---|
| R-001 | Large files freeze UI | Medium | High | 10 MB guard, future async |
| R-002 | Missing Qt DLLs | High | High | `windeployqt` packaging |
| R-003 | Poor color contrast | Medium | Medium | Reader Appearance presets |
| R-004 | Broken file path | Medium | Low | Warning dialog + status bar |

### 11.3 Key/Value Fields

| Field | Value |
|---|---|
| App name | MD Backpack Reader |
| Platform | Windows |
| UI toolkit | Qt6 Widgets |
| Language | C++20 |
| Parser | md4c |
| Rendering target | QTextBrowser |
| Deployment | Windows portable folder |

---

## 12. Definition-Style Content

Renderer
: Converts Markdown into HTML.

Preprocessor
: Removes YAML front matter before rendering.

Appearance Controller
: Stores current reader color settings and notifies open tabs when colors change.

Repository
: Saves and loads structured settings from `QSettings`.

---

## 13. Markdown Edge Cases

### 13.1 Long Inline Code

Here is a long inline code sample that should remain readable: `cmake --build build --config Release --clean-first && ctest --test-dir build -C Release --output-on-failure`.

### 13.2 Very Long Link Text

[This is a deliberately long link label intended to test line wrapping inside the reader window when a sentence-like link spans a wide area of the document](https://example.com/very/long/path/for/testing/wrapping/behavior).

### 13.3 Pipes in Code vs Tables

Inline code with table pipe: `name | value | status`

| Name | Value |
|---|---|
| pipe in text | `a | b | c` |
| escaped pipe | a \| b \| c |

### 13.4 Backslashes

Windows path:

```text
C:\Users\ismae\Desktop\md\dist\windows\md-backpack-reader\md-backpack-reader.exe
```

---

## 14. Arabic Section

### عنوان عربي

هذه فقرة عربية لاختبار عرض النص العربي داخل قارئ Markdown. يجب أن يبقى النص واضحًا ومقروءًا، وأن لا تتداخل الرموز أو الروابط أو الكود مع اتجاه القراءة.

- عنصر أول
- عنصر ثانٍ يحتوي على `كود_داخلي`
- رابط عربي: [مثال عربي](https://example.com)

> اقتباس عربي لاختبار لون الاقتباس وحدوده في الوضع النهاري والليلي.

```text
مثال كود نصي عربي
سطر 1
سطر 2
```

---

## 15. Appearance Palette Test

Use this section to test Reader Appearance colors.

### Heading color test

Normal paragraph should use main text color.

*Italic text should use muted text color.*

[Link text should use link color](https://example.com).

Inline code should use inline code colors: `inline sample`.

```cpp
// Code block should use code block text/background
int value = 42;
return value;
```

> Blockquote should use blockquote text and border colors.

---

## 16. Final Checklist

- [ ] Headings are clear
- [ ] Paragraphs are readable
- [ ] Links are visible but not aggressive
- [ ] Inline code is distinct
- [ ] Code blocks are readable
- [ ] Tables are aligned
- [ ] Blockquotes are visually separate
- [ ] Horizontal rules are subtle
- [ ] Arabic text is readable
- [ ] Day/Night palettes are comfortable

End of test document.
