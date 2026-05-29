# PDF Preview Smoke Checklist

Manual validation for the Phase 11 PDF open integration.

## Prerequisites

- Built/packaged application binary

## Steps

| Step | Action | Expected |
|---|---|---|
| 1 | Launch application | Window opens with placeholder / empty state |
| 2 | File → Open → select `.md` file | Markdown rendered in a tab |
| 3 | File → Open → select simple path PDF | PDF page image displayed in a separate tab |
| 4 | File → Open → select invalid PDF (`.pdf` extension, non-PDF bytes) | Diagnostic tab opens with visible error/status text |
| 5 | File → Open → select same PDF opened in step 3 | Existing PDF tab focused; no duplicate tab created |
| 6 | Close PDF tab | Tab removed; placeholder shown if it was the last tab |
| 7 | File → Recent Files → select PDF entry | PDF opens in new tab |
| 8 | Drag `.pdf` file onto application window | PDF opens in new tab |

## Results

| Step | Pass/Fail | Notes |
|---|---|---|
| 1 | | |
| 2 | | |
| 3 | | |
| 4 | | |
| 5 | | |
| 6 | | |
| 7 | | |
| 8 | | |
