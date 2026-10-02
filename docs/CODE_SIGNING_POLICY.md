# Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io/),
certificate by [SignPath Foundation](https://signpath.org/).

## What is signed

Only binaries built from this repository's source code are signed: the
`md-backpack-reader.exe` program. Every signed file is built by the
[release workflow](../.github/workflows/release.yml) on GitHub-hosted runners
from a tagged commit. No build is signed from a developer's machine.

Third-party libraries shipped with the program (Qt, the compiler runtime) are
not signed with this project's certificate. Qt's own DLLs carry The Qt
Company's signature.

## Team roles

| Role | Members |
|---|---|
| Committers and reviewers | [Ismaeil Tawil](https://github.com/@GITHUB_USER@) |
| Approvers | [Ismaeil Tawil](https://github.com/@GITHUB_USER@) |

- **Committers** may change the source code without additional review.
- **Reviewers**: every change proposed by anyone else (pull requests) is
  reviewed by a team member before it is merged.
- **Approvers**: every signing request is approved manually by an approver
  before the release is signed.

All team members use multi-factor authentication for GitHub and SignPath.

## Privacy policy

This program will not transfer any information to other networked systems
unless specifically requested by the user or the person installing or
operating it.

In practice:
- Documents are read from local disk only. Images are loaded only from local
  files; remote images are never downloaded.
- Clicking a web link (`http`, `https`, `mailto`) hands it to the default web
  browser or mail program, which then follows its own privacy policy.
- There is no telemetry, no update check and no crash reporting.

## Reporting a problem

If you believe a signed release violates this policy, open an issue in this
repository, or contact SignPath at support@signpath.io.
