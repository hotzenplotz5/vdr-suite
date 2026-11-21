# Rectools audit and C++ media-import slice

## Inspected reference sources

Repository `hotzenplotz5/vdr-rectools` at `86b52e52ece8808727a52109348d295535446b32` (main). Relevant full architecture surfaces: `usr/bin/vdr-rectools`, `usr/bin/vdr-rectools-worker`, `usr/share/vdr-rectools/functions.sh`, `usr/share/vdr-rectools/media_tools.sh`, `debian/control`, `debian/vdr-rectools.conf`, `docs/architecture.md`, `docs/vdr-suite-integration.md`, README and legacy PHP dashboard descriptions. The historical document still prefers Bash integration; this C++ import slice is a deliberate later user-approved strategy, **not** a modification of the external Rectools repository.

## Feature inventory and new owner

| Legacy concern | Evidence and hazards | Suite decision |
| --- | --- | --- |
| Auto-import scan | `run_scan(import)` finds mkv/mp4/ts/avi/mov under `IMPORT_DIR`, maxdepth 2, mutable `MAX_FILES`; newline-delimited path transport | C++ single-file and later separate batch discovery; registered backend-local roots |
| Metadata | adjacent NFO `title, plot, outline, year, genre`, fallback basename; unsafe/inconsistent XML extraction | filename-only, sanitized title in first slice; proper bounded parser later |
| Codec switch | `ffprobe`; some inputs remux, DV/web/legacy trigger H264/HEVC re-encode; optional downmix | first C++ slice remux only for h264/hevc/mpeg2video, no background GPU encode |
| Repair and stream QA | import unconditionally calls `smart_repair`; `sanitize_stream` can rename/write streams | **excluded**; require separate read-only checker design |
| Stage/index/commit | staging under configurable `REPAIR_STAGING`, `vdr --genindex`, `mv -T` to date.rec | private staging on destination filesystem; index evidence; atomic no-replace commit |
| Original media | successful import `rm -f` source; duplicates/unsupported files get renamed; subtitles may be removed | never remove/move original or sidecars |
| Chapters and subtitles | FFprobe chapters written as `marks`, local SRT copied then deleted, optional network subliminal | defer; future parsing and authorization/security review |
| Postprocessing | `process_folder(normal)`, images/NFO links, TVScraper SVDRP, `.update`, mail/Telegram | delegate to existing Suite metadata/reconciliation and notification owners when integrated |
| Shrink | HEVC transcoding replaces `00001.ts` and index after conversion | not integrated; redesign verified non-destructive operation |
| Check | FFmpeg scan writes `.vdr-rectools.broken`, status/mail artifacts | not integrated; future genuinely read-only output |
| Repair, PES→TS, cut, rename, move | legacy repair unsafe; PES→TS native in target VDR; editing already Suite-owned | excluded from Media Tools |
| Worker and PHP UI | 0777 `/tmp/vdr-rectools-jobs`, 0666 status, broad action including VDR restart | **never** use as Suite trust boundary |
| Debian packaging | separate CLI/web packages with legacy dependencies | new optional C++ binary package later; no conflicting ownership today |

## First C++ implementation boundary

C++17 standalone `MediaImport` inspection and explicitly confirmed import, isolated from the Suite daemon. The library rejects non-absolute or traversing paths, path-component symlinks, files outside source roots, overlapping source/destination trees, unsupported extensions, existing recording folders and unsupported video codecs. Import holds an exclusive per-video-root lock, performs capacity preflight (minimum 4 GiB / 15% remaining after double-source-size staging budget), writes a private staging recording, remuxes with fixed argument vectors (not shell commands), invokes VDR index generation, checks nonempty TS/index and unchanged source stat signature, then publishes via no-replace rename.

Boundaries explicitly still missing: backend identity/actor permission, timeouts/cancellation, stable source file descriptor anti-TOCTOU enforcement, crash-recovery job state, TV playback compatibility readback, canonical recording-cache reconciliation, metadata/NFO, subtitle policy, real end-to-end VDR acceptance and installable architecture-specific worker packaging. No live deployment, no public API, and no add-on capability declaration in this slice.

## Validation

Focused offline test target: `make test-addon-media-import`. Standalone build: `make addon-media-import`. No root access, VDR restart, media install or user recording mutation is required for the tests. Follow root and module AGENTS.md. Do not run `import` against live VDR tree until the missing gates above are completed.
