# SignPath Foundation setup checklist

Already done in this repository: MIT `LICENSE`, `THIRD_PARTY_NOTICES.md`,
product name/version in the .exe, `docs/CODE_SIGNING_POLICY.md`, privacy and
uninstall sections in README, `.github/workflows/release.yml`, and
`.signpath/artifact-configuration.xml`.

Steps only the owner can do:

1. Enable two-factor authentication on GitHub.
2. Done: public repository https://github.com/ismaeiltawil25-star/md-ismaeiltawil
   (default branch `main`).
3. Done: README and `docs/CODE_SIGNING_POLICY.md` link to it.
4. Publish a first (unsigned) release: `git tag v0.3.0 && git push origin v0.3.0`.
   SignPath requires the project to be released already.
5. Apply at <https://signpath.org/apply>. Suggested answers:
   - Project name: MD Backpack Reader
   - Repository URL / Homepage: the GitHub repository URL
   - Description: lightweight offline Markdown and PDF reader for Windows (Qt 6, C++20)
   - License: MIT
6. After approval: install the SignPath GitHub App, enable MFA in SignPath,
   paste `.signpath/artifact-configuration.xml` into the SignPath project, then
   add to the GitHub repository settings:
   - Secret `SIGNPATH_API_TOKEN`
   - Variables `SIGNPATH_ORGANIZATION_ID`, `SIGNPATH_PROJECT_SLUG`, `SIGNPATH_SIGNING_POLICY_SLUG`
7. Tag the next release. The workflow submits it to SignPath; approve the
   request in SignPath, and the signed ZIP is published.

Note: SignPath decides acceptance at its discretion and requires verifiable
reputation for executables, so approval is not guaranteed.
