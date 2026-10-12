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
