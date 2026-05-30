# Baseline — Packaged PDF Preview MVP

**Date:** 2026-05-30
**Commit:** `a6306b6` (latest)
**Status:** Visually validated, PATH-isolated package ready.

## Regression status

- **Full regression:** 50/50 test executables passed, 0 failures, 0 warnings.

## Packaged app

- **Location:** `dist/windows/md-backpack-reader/`
- **Size:** 35.56 MB, 51 files
- **Self-contained:** Qt 6.7 DLLs, MSVC runtime DLLs (`msvcp140`, `vcruntime140`, `vcruntime140_1d`), `qwindows` platform plugin, image format plugins, style plugin, TLS backends, translations.
- **PATH-isolated:** App launches without Qt or Visual Studio in PATH.
- **Visual validation:** Markdown, PDF text, PDF paths, zoom controls, invalid-PDF diagnostics — all confirmed visually by a human operator.

## Markdown pipeline

Unaffected by PDF changes. All existing Markdown tests pass.

## PDF Preview MVP — supported capabilities

- PDF 1.7 header parsing and cross-reference tables.
- Indirect object loading, stream decoding (FlateDecode).
- Catalog and page tree resolution.
- Content stream tokenizing and operator interpretation.
- Path rendering via `QPainterPath` / `QPainter`.
- MVP text rendering via fallback `QFont("sans-serif")` with Latin-1 / ASCII decoding.
- Text rendered on top of paths.
- Zoom toolbar: `[-] [100%] [+]`.
- Invalid PDF: graceful diagnostic display, no crash.

## PDF Preview MVP — explicit limitations

- No `/ToUnicode` CMap support.
- No font resource resolver (`/Resources`, `/Font` dictionaries ignored).
- No embedded font parsing (Type1, TrueType, CFF, CID).
- No Arabic shaping, no bidi, no glyph shaping.
- No text matrix rotation/shear/scale (translation only).
- No image XObject rendering.
- No multi-page navigation.
- No fit-to-width / fit-to-page zoom.
- No search, select, or copy text.
- No annotations, forms, or AcroForm.
- No encrypted PDF support.
- No xref streams or object streams.

## Architecture

- `pdf_core` — QtCore only, no QtGui/QtWidgets.
- `pdf_rendering` — QtCore + QtGui, no QtWidgets.
- `pdf_preview_services` — file-to-image orchestration.
- `pdf_widgets` — QtWidgets, preview display + zoom controls.
- `MainWindow` — orchestration only, no PDF internals.
- Shared test helpers at `tests/helpers/` (3 header files).

## Next phase

Requires a separate planning decision. Candidates: `/ToUnicode` support, font resource resolver, multi-page navigation, fit-to-width zoom, or packaging/release checklist formalization.
