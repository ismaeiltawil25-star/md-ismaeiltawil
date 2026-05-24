# Architecture Stabilization and Future-Development Hardening Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stabilize the current Qt/C++ Markdown reader architecture so future reader/editor features can be added safely without broad rewrites, hidden coupling, or performance regressions.

**Architecture:** Preserve current behavior while introducing explicit ownership boundaries, typed document/tab state, an explicit document-open pipeline, explicit render pipeline contracts, hardened normalizers, and measurable diagnostics. Each batch is intentionally narrow and must be implemented independently with tests before production changes where possible.

**Tech Stack:** C++20, Qt 6.7 Widgets/Core/Test, CMake, md4c, QSettings, QTextBrowser/QTextDocument, GitHub Actions on Windows.

---

## Execution Rules

This plan is stabilization work, not feature work.

| Rule | Requirement |
|---|---|
| No broad rewrite | Refactor in small reviewable batches only. |
| Preserve behavior | Current file open, rendering, appearance, zoom, recent files, links, and fallback behavior must remain unchanged unless a batch explicitly changes a broken internal behavior and adds tests. |
| Test first | Add or update tests before implementation whenever the behavior can be isolated. |
| One batch at a time | Do not merge unrelated batches into one implementation pass. |
| No speculative frameworks | Do not add plugin systems, alternate render engines, editor frameworks, or async infrastructure until a concrete feature requires them. |
| Performance protected | No batch may add extra full-document passes without documenting why the pass is necessary and measuring before/after impact. |
| No user-visible feature creep | Syntax highlighting, editor mode, tool panels, theme redesign, file associations, async loading, and multi-window support are out of scope. |
| Verification before completion | Every batch ends with build and full test verification. |

---

## C++/Qt Quality Gates

These gates apply to every implementation batch.

### Compiler Warnings Policy

| Gate | Policy |
|---|---|
| Existing warnings | Record the current warning state before changing build flags. Do not introduce new warnings. |
| MSVC target | Prefer `/W4` for project code once current warning baseline is clean. Avoid enabling high-warning policy on third-party `md4c` sources. |
| Warning failures | Once project code is clean, enable warnings-as-errors for project targets only in CI or behind an opt-in CMake option such as `MD_BACKPACK_WARNINGS_AS_ERRORS`. |
| Include boundaries | Core and appearance libraries must continue to avoid `Qt6::Widgets` links unless a batch explicitly moves UI code into app-only targets. |

### Static Analysis Recommendation

| Gate | Policy |
|---|---|
| clang-tidy | Add an optional CMake switch such as `MD_BACKPACK_ENABLE_CLANG_TIDY` if local tooling is available. Do not make it mandatory until CI/toolchain reliability is proven. |
| Scope | Run static analysis on project sources only: `src/app`, `src/services`, `src/renderers`, `src/utils`, `src/models`. Exclude `build/`, `dist/`, and fetched `md4c`. |
| Initial checks | Prefer maintainability, bugprone, performance, modernize, readability checks that do not force broad style churn. |
| Rollout | Start as advisory. Promote selected clean checks to CI only after the baseline is documented. |

### Ownership and Lifetime Policy

| Gate | Policy |
|---|---|
| QObject ownership | QObject-derived UI objects are owned by Qt parent/child ownership. Parent must be explicit at construction. |
| Non-QObject ownership | Use stack values or `std::unique_ptr` for owning non-QObject resources. Avoid raw owning pointers. |
| Observer pointers | Raw pointers are allowed only as non-owning observers with lifetime guaranteed by parent/object ownership. Use `QPointer` when a QObject can be deleted independently. |
| Service dependencies | Prefer constructor-injected concrete dependencies or small function injection for test seams. Avoid service locators. |
| Async readiness | Any future async batch must use explicit cancellation/lifetime ownership and must not capture raw UI pointers without `QPointer` or equivalent guard. |

### Copy/Move Policy for Large Payloads

| Payload | Policy |
|---|---|
| Markdown text | Pass read-only text as `const QString &` or `QStringView` when no ownership is needed. |
| Rendered HTML | Move into result/state objects when produced locally. Avoid repeated copies during tab creation and appearance reapply. |
| Document payloads | Keep one canonical raw text payload and one canonical rendered HTML payload per open tab unless a batch justifies additional cached forms. |
| Function returns | Returning `QString` by value is acceptable when relying on move/return value optimization. Avoid unnecessary intermediate `QString` copies in hot paths. |
| Dynamic properties | Do not store large payloads in `QObject::property`; use explicit typed state. |

### RAII and Dependency Ownership Rules

| Gate | Policy |
|---|---|
| Resource cleanup | File/socket/settings helpers must use RAII or Qt object ownership. Manual cleanup must be localized and tested. |
| Timers | Timing scopes should use small RAII timer objects and avoid heap allocation. |
| Settings | Persistence classes own QSettings interactions. UI classes should not directly own persistence details. |
| Errors | Functions crossing module boundaries must return typed results or explicit status, not silently log and continue unless the contract says so. |

### Global State Policy

| Gate | Policy |
|---|---|
| New global state | No new mutable global state unless explicitly justified in the batch design. |
| Existing singletons | `ErrorLogger` and `PerformanceMonitor` may remain, but any expanded use must consider thread-safety before async work. |
| Constants | File-local constants and `constexpr` values are acceptable. |
| Test isolation | Any global or singleton mutable state must have test isolation or reset strategy before tests depend on it. |

---

## Performance Guardrails

Performance must be measured before and after batches that touch loading, preprocessing, rendering, normalization, RTL, appearance reapplication, or tab state.

### Representative Document Sizes

| Size Class | Approximate Size | Purpose |
|---|---:|---|
| Tiny | 1-10 KB | Common README/simple note behavior. |
| Medium | 100-250 KB | Realistic long document with tables, code, lists, and RTL. |
| Large | 1-2 MB | Stress normalizer and RTL passes without approaching guard limit. |
| Near Limit | 9-9.5 MB | Validate behavior near current 10 MB guard while avoiding rejection. |
| Over Limit | 10 MB + 1 byte | Verify rejection remains fast and does not read the full file unnecessarily. |

### Required Measurements

| Area | Measurement |
|---|---|
| File loading | File size, existence/type check time, read time, rejection time for over-limit file. |
| Preprocessing | YAML stripping time, definition-list conversion time, output size. |
| Markdown rendering | md4c render time, generated HTML size, md4c return status. |
| HTML normalization | Code block wrapping time, blockquote wrapping time, raw tag normalization time, task-list normalization time. |
| RTL processing | RTL scan time, number of blocks visited if cheap to record, whether code blocks were skipped. |
| Appearance reapply | Time to apply stylesheet and reset existing HTML, scroll/cursor preservation result. |
| Tab/document state | Time to construct tab state and attach browser for representative payloads. |
| Error logging | Confirm normal successful render path does not log large payloads or high-volume messages. |

### Regression Thresholds

| Stage | Requirement |
|---|---|
| Before baseline exists | Batch A must record baseline timings manually or document how to collect them. No performance claims may be made without baseline evidence. |
| After baseline exists | Any batch touching hot paths should stay within 10% of baseline for Tiny/Medium documents and 15% for Large/Near Limit documents, unless the change includes a written justification and updated baseline. |
| Full-document passes | Stabilization must not add extra full-document passes without justification. If an additional pass is required, the batch must document the reason, affected document sizes, and measured cost. |
| CI thresholds | Keep strict timing thresholds out of CI until measurements are stable across machines. Use manual or smoke thresholds first. |

---

## Batch A: Documentation and Baseline Verification Only

### Goal

Correct documentation and establish the current verification/performance baseline without production code changes.

### Why This Batch Is Needed

Current docs have stale test counts and overstate renderer observability. Local test execution also depends on Qt runtime setup that should be documented clearly before deeper refactors begin.

### Audit Issues Addressed

| Issue | Addressed By |
|---|---|
| `MVP_STATUS.md` has stale test counts | Update status to match current CTest registration and latest run. |
| Architecture docs overstate observability | Clarify current top-level timing vs future per-stage timing. |
| Local test execution depends on Qt runtime setup | Document PowerShell environment requirements and commands. |

### Expected Changed Files

| File | Expected Change |
|---|---|
| `docs/MVP_STATUS.md` | Update test executable and test count section. |
| `docs/ARCHITECTURE_GUIDELINES.md` | Clarify current observability and planned per-stage diagnostics. |
| `README.md` | Add or correct local test command notes for Qt runtime setup. |
| `docs/superpowers/plans/2026-05-24-architecture-stabilization-hardening-plan.md` | This plan file. |

### Tests To Add Before Implementation

No automated tests are required because Batch A is documentation and baseline verification only.

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

If `ctest` is unavailable in the shell PATH, use the documented Qt test executable fallback after setting:

```powershell
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_PLUGIN_PATH = "C:\Qt\6.7.0\msvc2019_64\plugins"
$env:PATH = "C:\Qt\6.7.0\msvc2019_64\bin;$env:PATH"
```

### Performance Considerations

Record baseline measurements for the current representative documents if available. If no benchmark harness exists, record that baseline measurement is manual until Batch I.

### Rollback Risk

Low. Reverting this batch only reverts documentation corrections and baseline notes.

### Acceptance Criteria

| Criterion | Required Result |
|---|---|
| Docs match tests | Documentation reflects 11 test executables and 148 tests if still current at implementation time. |
| Observability wording accurate | Docs do not claim per-stage timing until Batch F implements it. |
| Local verification clear | README or docs show the Qt environment variables required for direct local test execution. |
| Behavior unchanged | No production code files are modified. |

### Explicit Non-Goals

No production refactor, no test runner script, no build flag changes, no per-stage diagnostics implementation.

---

## Batch B: Explicit Document and Tab State

### Goal

Replace hidden dynamic `QTextBrowser` properties with explicit typed state for open documents and tabs.

### Why This Batch Is Needed

Dynamic properties hide ownership and make future document behaviors risky. Tab state should be explicit before adding broader rendering, editor, tool, or session features.

### Audit Issues Addressed

| Issue | Addressed By |
|---|---|
| Dynamic `QTextBrowser` properties hide document/tab state | Introduce typed state model. |
| Document/tab state coupled to appearance reapply and zoom | Centralize state access behind explicit methods. |
| MainWindow owns too much rendering state | Move state responsibility into document/tab module. |

### Expected Changed Files

| File | Expected Change |
|---|---|
| `src/app/document/DocumentTabState.h` | Create explicit state struct/class. |
| `src/app/document/DocumentContentMode.h` | Create enum if not nested in state. |
| `src/app/document/DocumentTabWidget.h` | Create lightweight wrapper around `QTextBrowser` if needed. |
| `src/app/document/DocumentTabWidget.cpp` | Implement wrapper behavior if needed. |
| `src/app/MainWindow.h` | Replace direct dynamic-property assumptions with typed tab access. |
| `src/app/MainWindow.cpp` | Use explicit state for file path, zoom, content mode, original content, and base URL. |
| `CMakeLists.txt` | Add new files to app target. |
| `tests/unit/DocumentTabStateTest.cpp` | Add state tests. |
| `tests/CMakeLists.txt` | Register new tests. |

### Tests To Add Before Implementation

| Test File | Test Cases |
|---|---|
| `tests/unit/DocumentTabStateTest.cpp` | Constructs HTML state with file path, display name, base URL, and original HTML. |
| `tests/unit/DocumentTabStateTest.cpp` | Constructs plain-text fallback state with original plain text. |
| `tests/unit/DocumentTabStateTest.cpp` | Updates zoom steps explicitly and preserves content mode. |
| `tests/unit/DocumentTabStateTest.cpp` | Rejects or avoids ambiguous empty content mode if class enforces invariants. |

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Performance Considerations

The state object must not duplicate large payloads beyond current behavior. Move rendered HTML/plain text into state where possible. Do not store large strings in `QObject::property`.

### Rollback Risk

Medium. Reverting may touch `MainWindow` and tab creation logic. Keep this batch isolated from pipeline extraction so rollback does not affect rendering logic.

### Acceptance Criteria

| Criterion | Required Result |
|---|---|
| No hidden tab payloads | Production code no longer stores `filePath`, `zoomSteps`, `contentMode`, `originalHtml`, or `originalPlainText` as dynamic `QTextBrowser` properties. |
| Current behavior preserved | Open tabs, duplicate-tab detection, zoom, base URL for images, appearance reapply, and fallback display behave as before. |
| Tests pass | New state tests and all existing tests pass. |
| Ownership clear | `QTextBrowser` remains view/UI object; document/tab state is explicit and typed. |

### Explicit Non-Goals

No session restore, no multi-window support, no async loading, no document editing, no render pipeline changes.

---

## Batch C: Document Open and Render Pipeline Extraction

### Goal

Move document loading, preprocessing, rendering, and fallback result creation out of `MainWindow` into a focused document-open service.

### Why This Batch Is Needed

Future features need a single reliable path for opening a document. Callers should not manually remember YAML stripping, definition-list conversion, render fallback, base URL, or diagnostics.

### Audit Issues Addressed

| Issue | Addressed By |
|---|---|
| `MainWindow` owns document opening, preprocessing, rendering | Extract `DocumentOpenPipeline`. |
| Full document pipeline is not encapsulated | One service owns the sequence. |
| Future callers may skip preprocessing | Public open result comes only through pipeline. |
| Renderer errors not surfaced enough | Result includes render diagnostics for UI to show/log. |

### Expected Changed Files

| File | Expected Change |
|---|---|
| `src/app/document/DocumentOpenResult.h` | Create typed result with status, metadata, content mode, diagnostics, and payload. |
| `src/app/document/DocumentOpenPipeline.h` | Create focused service API. |
| `src/app/document/DocumentOpenPipeline.cpp` | Implement current load-preprocess-render-fallback sequence. |
| `src/app/MainWindow.cpp` | Replace manual calls to `DocumentLoader`, `MarkdownPreprocessor`, and `MarkdownHtmlRenderer`. |
| `src/app/MainWindow.h` | Store/use pipeline dependency if needed. |
| `CMakeLists.txt` | Add new files. |
| `tests/unit/DocumentOpenPipelineTest.cpp` | Add pipeline tests. |
| `tests/CMakeLists.txt` | Register tests. |

### Tests To Add Before Implementation

| Test File | Test Cases |
|---|---|
| `tests/unit/DocumentOpenPipelineTest.cpp` | Valid Markdown file loads and returns HTML mode. |
| `tests/unit/DocumentOpenPipelineTest.cpp` | YAML front matter does not appear in rendered HTML. |
| `tests/unit/DocumentOpenPipelineTest.cpp` | Definition list conversion is applied before render. |
| `tests/unit/DocumentOpenPipelineTest.cpp` | Missing file returns load failure without creating tab payload. |
| `tests/unit/DocumentOpenPipelineTest.cpp` | Base URL points to containing folder for relative images. |

If render failure is difficult to trigger with md4c, add a narrow test seam using a renderer function dependency:

```cpp
using RenderFunction = std::function<MarkdownHtmlRenderer::Result(const QString &)>;
```

The default constructor must use `MarkdownHtmlRenderer::render` so production behavior does not change.

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Performance Considerations

The service must not add extra preprocessing or render passes. Keep the sequence synchronous and equivalent to current behavior. Use moves for large rendered payloads in `DocumentOpenResult`.

### Rollback Risk

Medium. Reverting affects `MainWindow::openDocumentFromPath()` and new service files. Rollback should not require reverting Batch B if interfaces are kept narrow.

### Acceptance Criteria

| Criterion | Required Result |
|---|---|
| Pipeline centralized | `MainWindow::openDocumentFromPath()` no longer manually performs load, preprocess, and render sequence. |
| Current behavior preserved | Valid files open as HTML, render failures fall back to plain text, load failures show current error path. |
| Diagnostics available | Open result carries loader/render error text for UI/status/logging. |
| Tests pass | New pipeline tests and all existing tests pass. |

### Explicit Non-Goals

No async document loading, no progress UI, no editor model, no alternate renderer, no new file formats.

---

## Batch D: Explicit Rendering Pipeline Contracts

### Goal

Make render stage order, diagnostics, and stage contracts explicit without changing rendered output.

### Why This Batch Is Needed

The current renderer sequence works but is implicit. Later stages depend on earlier generated HTML and exact class names, which makes future changes risky.

### Audit Issues Addressed

| Issue | Addressed By |
|---|---|
| Rendering pipeline order is implicit and fragile | Introduce named stages and explicit order tests. |
| Weak error boundaries between transformation stages | Stage result carries diagnostics/warnings. |
| Docs overstate observability | Implementation starts matching documented pipeline concept. |

### Expected Changed Files

| File | Expected Change |
|---|---|
| `src/renderers/RenderPipelineStage.h` | Create enum or descriptor for stage names. |
| `src/renderers/RenderPipelineResult.h` | Create stage diagnostics/result type if separate from renderer result. |
| `src/renderers/MarkdownHtmlRenderer.h` | Extend `Result` with diagnostics while preserving `isValid()`. |
| `src/renderers/MarkdownHtmlRenderer.cpp` | Run current sequence through named stage helpers. |
| `tests/unit/MarkdownHtmlRendererTest.cpp` | Preserve current output tests. |
| `tests/unit/RenderPipelineContractTest.cpp` | Add order/diagnostics tests if separated. |
| `tests/CMakeLists.txt` | Register new tests if a new test executable is added. |
| `docs/ARCHITECTURE_GUIDELINES.md` | Update only after implementation matches wording. |

### Tests To Add Before Implementation

| Test File | Test Cases |
|---|---|
| `tests/unit/RenderPipelineContractTest.cpp` | Stage order is `md4c`, `blockquote-wrap`, `code-block-wrap`, `html-normalize`, `rtl`, `rtl-blockquote-order`, `task-list-normalize`. |
| `tests/unit/RenderPipelineContractTest.cpp` | Diagnostics include stage names when enabled or when a warning/error exists. |
| `tests/unit/MarkdownHtmlRendererTest.cpp` | Existing rendering output remains unchanged for representative Markdown. |

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Performance Considerations

Use direct function calls or a fixed lightweight stage table. Avoid virtual dispatch and heap allocation per stage unless a concrete need appears. Diagnostics must not copy full HTML payloads.

### Rollback Risk

Medium. This touches central rendering code but should not affect app/UI files except diagnostic consumption in later batches.

### Acceptance Criteria

| Criterion | Required Result |
|---|---|
| Output stable | Existing renderer tests pass without expected-output changes except tests that intentionally inspect new diagnostics. |
| Stage order explicit | Stage order is represented in code and covered by tests. |
| Diagnostics typed | Renderer can identify stage context without parsing log text. |
| No extra passes | The batch does not add additional full-document transformation passes. |

### Explicit Non-Goals

No alternate renderers, no plugin stage system, no syntax highlighting, no async rendering.

---

## Batch E: Normalizer Hardening

### Goal

Make HTML normalization and task-list replacement tolerant of harmless generated HTML shape changes while preserving current output.

### Why This Batch Is Needed

Exact string and fragile regex transformations can silently fail if md4c changes attribute order, spacing, or tag forms.

### Audit Issues Addressed

| Issue | Addressed By |
|---|---|
| `TaskListNormalizer` relies on byte-exact md4c checkbox HTML | Use tolerant input matching. |
| `HtmlNormalizer` uses fragile regex/string transforms | Define supported tag contracts and improve scanners where needed. |
| Renderer stages depend on generated HTML shapes | Tests document expected tolerance and limits. |

### Expected Changed Files

| File | Expected Change |
|---|---|
| `src/renderers/TaskListNormalizer.cpp` | Replace exact string replacement with tolerant checkbox tag matching. |
| `src/renderers/TaskListNormalizer.h` | Keep API stable unless a helper is needed. |
| `src/renderers/HtmlNormalizer.cpp` | Replace fragile cases with narrower scanners or documented regex improvements. |
| `tests/unit/TaskListNormalizerTest.cpp` | Add attribute order and spacing tests. |
| `tests/unit/HtmlNormalizerTest.cpp` | Add supported raw HTML shape tests. |
| `tests/unit/MarkdownHtmlRendererTest.cpp` | Keep integration coverage for task lists and raw HTML. |

### Tests To Add Before Implementation

| Test File | Test Cases |
|---|---|
| `tests/unit/TaskListNormalizerTest.cpp` | Checked checkbox is replaced when `checked` appears before `disabled`. |
| `tests/unit/TaskListNormalizerTest.cpp` | Checkbox is replaced when attributes are reordered. |
| `tests/unit/TaskListNormalizerTest.cpp` | Unchecked checkbox is replaced when extra harmless attributes are present. |
| `tests/unit/TaskListNormalizerTest.cpp` | Non-task checkbox input is not replaced unless policy intentionally replaces all markdown checkboxes. |
| `tests/unit/HtmlNormalizerTest.cpp` | `<kbd>` with supported simple attributes normalizes if this is within contract. |
| `tests/unit/HtmlNormalizerTest.cpp` | Nested unsupported raw HTML behavior is documented by an explicit test. |

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Performance Considerations

Keep normalizers O(n) over HTML length. Avoid broad backtracking regex. Do not introduce a full DOM parser in this batch. Do not add an extra full normalization pass.

### Rollback Risk

Medium. Output differences can affect many rendering tests. Keep changes narrow and lock existing output before modifying implementation.

### Acceptance Criteria

| Criterion | Required Result |
|---|---|
| Existing behavior stable | Current task-list/raw-HTML tests pass. |
| Tolerance improved | New attribute-order/spacing tests pass. |
| Unsupported cases explicit | Tests or docs state what raw HTML shapes remain unsupported. |
| Performance protected | No additional full-document pass is introduced. |

### Explicit Non-Goals

No full HTML parser, no sanitizer, no new raw HTML feature set, no browser-grade DOM transformation.

---

## Batch F: Diagnostics and Per-Stage Visibility

### Goal

Make rendering failures and slow stages discoverable without noisy logs or measurable slowdown.

### Why This Batch Is Needed

The app currently logs md4c errors but post-processing stages cannot expose warnings/errors. UI render warnings also omit concrete renderer error details.

### Audit Issues Addressed

| Issue | Addressed By |
|---|---|
| Renderer errors not surfaced with enough diagnostic detail | Show/render error details through typed result. |
| Architecture docs overstate observability | Implement or accurately document per-stage timing. |
| ErrorLogger/PerformanceMonitor untested | Add focused tests before expanding their role. |
| Weak error boundaries | Stage diagnostics record context. |

### Expected Changed Files

| File | Expected Change |
|---|---|
| `src/renderers/MarkdownHtmlRenderer.h/.cpp` | Add stage diagnostics and optional per-stage timing. |
| `src/utils/PerformanceMonitor.h/.cpp` | Add testable reporting API or minimal thread-safety if needed. |
| `src/utils/ErrorLogger.h/.cpp` | Add tests or stable behavior for log level filtering. |
| `src/app/MainWindow.cpp` | Include renderer error detail in warning/status where appropriate. |
| `tests/unit/PerformanceMonitorTest.cpp` | Add tests if API changes. |
| `tests/unit/ErrorLoggerTest.cpp` | Add tests if API changes. |
| `tests/unit/MarkdownHtmlRendererTest.cpp` | Add diagnostic tests. |
| `docs/ARCHITECTURE_GUIDELINES.md` | Update observability description after code changes. |

### Tests To Add Before Implementation

| Test File | Test Cases |
|---|---|
| `tests/unit/MarkdownHtmlRendererTest.cpp` | Diagnostics contain stage names for render path when diagnostics are enabled or populated. |
| `tests/unit/MarkdownHtmlRendererTest.cpp` | md4c failure result carries error message and stage context if a test seam exists. |
| `tests/unit/PerformanceMonitorTest.cpp` | Reporting a named stage records elapsed time without requiring log parsing. |
| `tests/unit/ErrorLoggerTest.cpp` | Minimum log level filtering behaves predictably if exposed/testable. |

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Performance Considerations

Per-stage timing must be cheap RAII timing. Do not log every successful stage by default. Log slow stages above threshold or store diagnostics in result without writing high-volume logs.

### Rollback Risk

Medium. Renderer result shape and UI error display can affect tests. Keep existing `isValid()` contract stable.

### Acceptance Criteria

| Criterion | Required Result |
|---|---|
| Failure context clear | Render failure identifies stage and message. |
| UI has useful detail | MainWindow warning/status can include concrete render error without exposing huge payloads. |
| Observability true | Docs match actual timing/logging behavior. |
| No noisy success logs | Normal successful renders do not spam logs. |

### Explicit Non-Goals

No telemetry, no crash reporter, no remote logging, no strict performance CI thresholds.

---

## Later Batch G: SingleInstanceService Hardening

### Goal

Make app activation and local IPC reliable before expanding file operations or launch flows.

### Expected Changed Files

| File | Expected Change |
|---|---|
| `src/app/SingleInstanceService.h/.cpp` | Add activation signal, explicit buffering, safer stale-server behavior. |
| `src/main.cpp` | Connect activation signal to `MainWindow` instead of relying on parent cast. |
| `tests/unit/SingleInstanceServiceTest.cpp` | Add IPC tests with unique server names. |
| `tests/CMakeLists.txt` | Register test. |

### Tests To Add Before Implementation

Activation with no files, open one file, open multiple files, fragmented packet read, malformed packet ignored without crash.

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Rollback Risk

Medium. IPC tests can be environment-sensitive; use unique server names and no sleeps.

### Acceptance Criteria

`SingleInstanceService` no longer casts `parent()` to `QWidget`. Partial packets are buffered until complete. Existing second-instance routing behavior is preserved or made more reliable.

### Explicit Non-Goals

No multi-window support, no remote IPC, no cross-machine routing.

---

## Later Batch H: Settings Semantics and Persistence Reliability

### Goal

Clarify settings ownership and ensure persistence failures are visible.

### Expected Changed Files

| File | Expected Change |
|---|---|
| `src/app/appearance/ReaderAppearanceController.cpp` | Handle `ReaderAppearanceRepository::save()` result. |
| `src/app/appearance/ReaderStylesheetBuilder.cpp` | Enforce or document `customEnabled` semantics. |
| `src/app/appearance/ReaderAppearanceRepository.cpp` | Keep validation and status behavior testable. |
| `src/app/settings/ReaderViewSettingsRepository.h/.cpp` | Add only if zoom persistence is implemented in this batch. |
| `src/app/MainWindow.cpp` | Use settings repository for zoom only if zoom persistence is included. |
| Relevant tests | Extend appearance/controller/repository tests and add view settings tests if needed. |

### Tests To Add Before Implementation

Save failure is surfaced if testable, `customEnabled=false` behavior is explicit, `customEnabled=true` uses custom colors, zoom persists only if the batch includes zoom persistence.

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Rollback Risk

Medium. Clarifying `customEnabled` may reveal behavior ambiguity. Lock expected behavior before changing production code.

### Acceptance Criteria

Appearance save failure is not silently ignored. `customEnabled` has one documented owner. Zoom persistence is either implemented and tested or explicitly left unchanged.

### Explicit Non-Goals

No full theme system, no settings import/export, no profile manager.

---

## Later Batch I: Performance Smoke Tests and Measurement Harness

### Goal

Make performance measurable before future large-document, async, or render-mode work.

### Expected Changed Files

| File | Expected Change |
|---|---|
| `tests/unit/RenderPerformanceSmokeTest.cpp` | Add broad smoke measurement if stable enough. |
| `tests/data/perf/` | Add or generate representative fixtures if repository size remains reasonable. |
| `scripts/measure-render.ps1` | Add manual benchmark command if preferred over CI timing test. |
| `README.md` or `docs/ARCHITECTURE_GUIDELINES.md` | Document how to run performance measurement. |

### Tests To Add Before Implementation

If using automated smoke test: verifies representative document renders under a broad timeout in Release builds. If using manual script: no unit test required, but script must run and print stage timings.

### Verification Commands

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.
\scripts\measure-render.ps1
```

### Rollback Risk

Low to medium. Timing tests can be flaky. Prefer manual script first if CI variance is high.

### Acceptance Criteria

Developers can measure file load, preprocess, md4c render, normalizers, RTL, and appearance reapply on representative document sizes. No strict CI timing gate is added until baseline is stable.

### Explicit Non-Goals

No async loading, no memory-mapped files, no worker pool, no strict CI performance gate.

---

## Later Batch J: Future Extension Documentation

### Goal

Document where future features should attach after stabilization without creating unused frameworks.

### Expected Changed Files

| File | Expected Change |
|---|---|
| `docs/ARCHITECTURE_GUIDELINES.md` | Update final stabilized architecture. |
| `docs/FUTURE_EXTENSION_POINTS.md` | Add extension-point guide if useful. |

### Tests To Add Before Implementation

No automated tests required for documentation-only changes.

### Verification Commands

```powershell
ctest --test-dir build -C Release --output-on-failure
```

### Rollback Risk

Low. Documentation can be reverted independently.

### Acceptance Criteria

Future contributors can identify the correct extension point for syntax highlighting, render modes, file operations, settings, document tools, and async loading without editing unrelated UI code.

### Explicit Non-Goals

No plugin system, no alternate renderer, no feature implementation.

---

## Minimum Stabilization Scope Before Broad Text-Rendering Development

The minimum scope required before broad text-rendering expansion is:

| Required Batch | Reason |
|---|---|
| Batch A | Establish accurate docs and verification baseline. |
| Batch B | Remove hidden tab/document state before render behavior expands. |
| Batch C | Centralize document load/preprocess/render/fallback path. |
| Batch D | Make rendering stage order and contracts explicit. |
| Batch E | Reduce md4c/raw HTML shape fragility. |
| Batch F | Add useful diagnostics and per-stage visibility. |

SingleInstanceService, settings semantics, and performance smoke tests are important but can follow unless the next development task touches launch routing, settings behavior, or performance-sensitive large-document work.

---

## Feature Work That Must Not Be Mixed With Stabilization

| Feature Work | Reason To Keep Separate |
|---|---|
| Syntax highlighting implementation | Requires stable code-block/render contracts first. |
| New rendering modes | Requires explicit render pipeline contracts first. |
| Editor mode | Requires document state ownership and document model decisions first. |
| Async loading | Requires explicit lifecycle/ownership and performance baselines first. |
| Tool panels | Requires app state boundaries first. |
| Full theme system | Requires settings semantics to be stabilized first. |
| File associations/installer | Deployment concern unrelated to architecture stabilization. |
| Plugin system | Speculative and unnecessary until multiple concrete extension implementations exist. |

---

## Safest Execution Order

| Order | Batch |
|---:|---|
| 1 | Batch A: Documentation and baseline verification only |
| 2 | Batch B: Explicit document/tab state |
| 3 | Batch C: Document open/render pipeline extraction |
| 4 | Batch D: Rendering pipeline contracts |
| 5 | Batch E: Normalizer hardening |
| 6 | Batch F: Diagnostics and per-stage visibility |
| 7 | Later Batch G: SingleInstanceService hardening |
| 8 | Later Batch H: Settings semantics and persistence reliability |
| 9 | Later Batch I: Performance smoke tests and measurement harness |
| 10 | Later Batch J: Future extension documentation |

---

## Plan Self-Review

| Check | Result |
|---|---|
| Required corrections included | C++/Qt quality gates, performance guardrails, implementation batches, acceptance criteria, expected files, tests, verification, rollback risk, and non-goals are included. |
| Batch separation | Each batch is independent and must be implemented separately. |
| Full rewrite avoided | Plan uses targeted extraction and contracts only. |
| User-visible feature work excluded | Explicitly excluded from stabilization. |
| Performance protected | Representative sizes, measurements, regression limits, and no-extra-pass rule are specified. |
| File created as markdown | This document is the revised `.md` plan. |
