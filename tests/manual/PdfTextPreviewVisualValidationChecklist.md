# PdfTextPreviewVisualValidation Checklist

Phase 14 Task 1 — validate that Phase 13 text rendering produces visible text in the running application.

---

## Prerequisites

- Build completed with 0 warnings: `cmake --build build --config Release`
- Full regression: minimum 47/49 passing (tests 48-49 have a pre-existing environmental failure unrelated to text rendering)

---

## Steps

### 1. Rebuild the application

```powershell
cmake --build build --config Release
```

**Expected:** Build succeeds, 0 warnings.

**Result:** [ ]

---

### 2. Launch the application

```powershell
.\build\Release\md.exe
```

**Expected:** MainWindow opens with an empty tab area (or placeholder).

**Result:** [ ]

---

### 3. Open a text-only PDF

Create `text-only.pdf` with content stream `BT /F1 12 Tf (Hello) Tj ET`, MediaBox `[0 0 612 792]`.

Open the file in the application.

**Expected:**
- A PDF tab is created
- The word "Hello" appears as rendered text in the preview area
- No crash, no garbage, no placeholder-only display

**Result:** [ ]

---

### 4. Open a mixed path + text PDF

Create `mixed.pdf` with content stream `0 0 m 100 100 l S BT /F1 12 Tf (Hello) Tj ET`, MediaBox `[0 0 612 792]`.

Open the file in the application.

**Expected:**
- A PDF tab is created
- A diagonal line AND the word "Hello" both appear
- Text appears on top of the path (rendered second)

**Result:** [ ]

---

### 5. Open a path-only PDF

Create `path-only.pdf` with content stream `0 0 m 100 100 l S`, MediaBox `[0 0 612 792]`.

Open the file in the application.

**Expected:**
- A PDF tab is created
- A diagonal line is displayed
- Behavior is unchanged from before Phase 13

**Result:** [ ]

---

### 6. Open a blank PDF

Create `blank.pdf` with empty content stream, MediaBox `[0 0 612 792]`.

Open the file in the application.

**Expected:**
- A PDF tab is created
- A blank white page is displayed
- No crash, no garbage pixels, no text artifacts

**Result:** [ ]

---

### 7. Open a Markdown file

Create `test.md` with content `# Hello` and markdown body.

Open the file in the application.

**Expected:**
- A Markdown tab is created with rendered content
- Markdown behavior is unchanged (no regression)

**Result:** [ ]

---

## Summary

| Step | Pass? |
|------|-------|
| 1. Rebuild | [ ] |
| 2. Launch | [ ] |
| 3. Text-only PDF | [ ] |
| 4. Mixed path + text PDF | [ ] |
| 5. Path-only PDF | [ ] |
| 6. Blank PDF | [ ] |
| 7. Markdown file | [ ] |

---

## Notes

- Manual validation performed: [ ] Yes / [ ] No
- Date:
- Tester:
- Any issues found:
