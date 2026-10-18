# Real VDR-Suite screenshots — capture and publication checklist

## Current evidence

The README includes operator-supplied captures of VDR-Suite Web Home / “Was läuft jetzt” (`docs/media/web-home.png`), the recording detail view (`docs/media/web-recording-details.png`), the EPG timeline (`docs/media/web-epg-timeline.png`), an HbbTV view (`docs/media/web-hbbtv.png`), and the OSDTeletext interface (`docs/media/web-osdteletext.png`), all added on 2026-10-16. The precise installed build has not been independently verified. The [separate VIDAA repository](https://github.com/hotzenplotz5/vdr-suite-vidaa) still has no verified real-television capture in this gallery. A logo, generated mockup, fixture view or static illustration is **not** a real-device screenshot.

**No fabricated screenshots belong in the README.** This page tracks the five submitted real Web captures and the still-missing views without implying more acceptance than the device demonstrates.

## Gallery status and proposed captures

| Image | Required evidence / current status | Filename |
| --- | --- | --- |
| Home / Now & Next | **Image provided**; exact installed build not independently established | `docs/media/web-home.png` |
| Recording detail view | **Image provided**; detail view preferred to the recording browser | `docs/media/web-recording-details.png` |
| EPG timeline / Zeitleiste | **Image provided**; operator-supplied programme timeline view | `docs/media/web-epg-timeline.png` |
| HbbTV | **Image provided**; operator-supplied HbbTV view, not full interoperability acceptance | `docs/media/web-hbbtv.png` |
| OSDTeletext / Videotext | **Image provided**; operator-supplied OSDTeletext interface | `docs/media/web-osdteletext.png` |
| Playback view | Real playback controls and displayed recording, with explicit client/source context | `docs/media/web-playback.png` |
| Hisense VIDAA app (optional) | Actual television showing the first-party app; verify the installed candidate and acceptance scope | `docs/media/vidaa-on-tv.jpg` |

Take images only with the actual installation and its operator's permission. Verify source build/installed assets if describing them as a specific release. The README embeds only genuine submitted captures using relative paths and short captions. Other views remain explicit placeholders until evidence files are provided.

## Help replace the README screenshot slots

The README now includes **five real Web screenshots** (Home, recording details, EPG timeline, HbbTV and OSDTeletext). The VIDAA slot remains a text-only placeholder, not a visual mockup.

### 1. Web Home / Now & Next

1. Open the **installed** VDR-Suite Web frontend and switch to Home / Now & Next.
2. Capture the actual browser content, crop away browser chrome and redact server URLs, names, identifiers and account information.
3. The submitted Home image is stored at `docs/media/web-home.png`. Confirm the installed server build separately; do not infer it from `main`.

### 2. Web Recordings

1. Open the actual Recordings list (or detail view) with representative, non-sensitive content.
2. Capture browser UI with private titles, filenames, backend paths and personal account data hidden or redacted.
3. The submitted recording detail image is stored at `docs/media/web-recording-details.png`. This screenshot does not establish recording playback acceptance.

### 3. EPG Timeline

1. Open the actual installed EPG timeline / Zeitleiste in the VDR-Suite Web frontend.
2. Crop away browser chrome and redact private server or account data.
3. The submitted EPG timeline image is stored at `docs/media/web-epg-timeline.png`. It illustrates the timeline, not EPG correctness across channels or any specific installed release.

### 4. HbbTV

1. Open the HbbTV view on the actual VDR-Suite Web installation.
2. Capture only shareable on-screen content; redact private URLs and identifiers.
3. The submitted image is stored at `docs/media/web-hbbtv.png`. A screenshot alone does not establish interactive HbbTV functionality or cross-device compatibility.

### 5. OSDTeletext / Videotext

1. Open OSDTeletext in the real VDR-Suite installation.
2. Capture only shareable on-screen content; redact private URLs and identifiers.
3. The submitted image is stored at `docs/media/web-osdteletext.png`. A still image illustrates the interface but does not establish correct teletext rendering on all channels.

### 6. Hisense VIDAA TV

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

The Home, recording detail, EPG timeline, HbbTV and OSDTeletext captures are included in the README. Add each further reviewed capture in its matching slot, especially a real TV photo when available. Keep this checklist available for later updates.

## Back

- [README](../README.md)
- [First steps](getting-started.md)
- [Documentation home](index.md)
