# Tests

| Folder | Contents |
|---|---|
| `unit/` | QtTest unit tests for Markdown, appearance, document and app layers |
| `unit/RenderedDocumentContractTest.cpp` | Asserts on the `QTextDocument` the reader shows (strike-out, links, RTL, no lost content) |
| `unit/RenderRobustnessTest.cpp` | Seeded fuzzing, pathological inputs and time budgets (catches freezes) |
| `unit/pdf/` | Custom PDF engine tests (parser, content streams, rendering) |
| `unit/widgets/`, `unit/services/` | PDF preview widget and service tests |
| `helpers/` | Shared header-only helpers for building test PDFs |
| `data/` | Markdown fixtures used by unit tests (`TEST_DATA_DIR`) |
| `diagnostic/` | Standalone debugging executables (built, but not registered with CTest) |
| `manual/` | Manual visual-validation checklists and the visual test document |

Run all registered tests:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Diagnostic tools:

```powershell
.\build\tests\Release\PdfDiagnostic.exe path\to\file.pdf
.\build\tests\Release\WidgetDisplayDiagnostic.exe
```

For a manual visual check, open `manual/comprehensive-visual-test.md` in the app.
