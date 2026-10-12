# Real VDR-Suite screenshots — capture and publication checklist

## Current evidence

At the audited `main` checkpoint (2026-10-12), the backend repository has brand/icon images but **no verified screenshots** of its running Web UI. The [separate VIDAA repository](https://github.com/hotzenplotz5/vdr-suite-vidaa) likewise currently contains app icons, not a verified television screen capture. A logo, generated mockup, fixture view or static illustration is **not** a real-device screenshot.

**No fabricated screenshots belong in the README.** This page defines the steps for obtaining two to four genuine, publishable images without implying more acceptance than the device demonstrates.

## Proposed gallery (only after real capture)

| Image to provide | Required evidence | Suggested descriptive filename |
| --- | --- | --- |
| Home / Now & Next | The loaded, real VDR-Suite Web interface on a known installed build | `docs/media/web-home.png` |
| Recording browser / details | Current Web Recording UI (avoid private paths or titles) | `docs/media/web-recordings.png` |
| Playback view | Real playback controls and displayed recording, with explicit client/source context | `docs/media/web-playback.png` |
| Hisense VIDAA app (optional) | Actual television showing the first-party app; verify the installed candidate and acceptance scope | `docs/media/vidaa-on-tv.jpg` |

Take images only with the actual installation and its operator's permission. Verify source build/installed assets if describing them as a specific release. The README should then embed **2–4** of the real images using relative paths and short captions; create `docs/media/` only once evidence files exist.

## Help replace the README screenshot slots

The README currently shows **text-only placeholders**, not visual mockups. The maintainer can replace them one at a time with genuine screenshots; there is no need to provide all three at once.

### 1. Web Home / Now & Next

1. Open the **installed** VDR-Suite Web frontend and switch to Home / Now & Next.
2. Capture the actual browser content, crop away browser chrome and redact server URLs, names, identifiers and account information.
3. Suggested eventual path: `docs/media/web-home.png`. Confirm which server build was installed; do not infer it from `main`.

### 2. Web Recordings

1. Open the actual Recordings list (or detail view) with representative, non-sensitive content.
2. Capture browser UI with private titles, filenames, backend paths and personal account data hidden or redacted.
3. Suggested eventual path: `docs/media/web-recordings.png`. Do not use a source screenshot as evidence that recorded playback was accepted.

### 3. Hisense VIDAA TV

1. Open the installed app on the physical Hisense/VIDAA television and capture its real screen (a photo is acceptable).
2. Include only visible UI that can lawfully be shared; remove pairing codes, private server URLs and channel/movie art without publication permission.
3. Suggested eventual path: `docs/media/vidaa-on-tv.jpg`. Include television model, installed client build and capture date in the PR description, **not** in the picture if these details are sensitive.

### Replacement process

Once a capture has been privacy-reviewed, commit only that real file and replace its matching README table cell with a Markdown image linked by the corresponding relative path. Keep remaining placeholders labeled until their images exist. No empty PNG/JPEG assets, simulated screenshots or AI-generated product screenshots are necessary.

## Publication safety

- Redact local/public IPs, domains, tokens, pairing codes, user names, playback cookies, recording paths, credentials, personal video titles and account details before upload.
- Confirm that television channel logos, programme artwork and video frames may lawfully be published; otherwise capture UI without copyrighted artwork/video.
- Avoid screenshots that could leak backend IDs and attached private systems if they are sensitive.
- Record capture date, client/browser or VIDAA model, installed build evidence and what was actually tested. **A still image cannot prove working sound, seeking, subtitle conversion or acceptance.**
- Do not add low-fidelity AI renderings, stock TV pictures or unverified thumbnails to fill the gallery.

## Where to publish

Once genuine redacted files are committed in a separate reviewed PR, replace the README's explicit screenshot-pending note with a small gallery (prefer two Web screenshots and one real TV photo if available). Keep this checklist available for later updates.

## Back

- [README](../README.md)
- [First steps](getting-started.md)
- [Documentation home](index.md)
