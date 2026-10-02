# Contributing

Read [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) first: it explains the
pipeline, the folder responsibilities, the coding rules and the test layers.

## Workflow

1. Branch from `main`: `feat/<topic>` or `fix/<topic>`.
2. One logical change per commit, with a Conventional Commits prefix
   (`feat:`, `fix:`, `perf:`, `build:`, `docs:`, `test:`, `chore:`).
3. Every behaviour change comes with a test. Every bug fix comes with a test
   that fails without the fix.
4. Before pushing:
   - build with `-DMDBR_WARNINGS_AS_ERRORS=ON`, with zero warnings;
   - run `ctest --output-on-failure`, with every test passing.
5. Open a pull request. Both CI jobs (MSVC and strict MinGW) must be green.

## Definition of done

- [ ] The code follows the rules in DEVELOPER_GUIDE section 3
- [ ] Unit tests cover the new or changed unit
- [ ] The change is visible to users → `RenderedDocumentContractTest` case
- [ ] It scans nested structures → `RenderRobustnessTest` row with a time budget
- [ ] Docs are updated (pipeline diagram, MVP_STATUS limitations)
