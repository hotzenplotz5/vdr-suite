# Image Gallery – optional VDR-Suite add-on

Local-first gallery for the browser and CEF frontend. No running VDR, Backend Agent or Image Next plugin is required. Remote backend transport is a later integration, not implemented here.

## Local installation and configuration

The optional `vdr-suite-addon-image` Debian binary package stages its UI separately from the daemon. Its Core handler is compiled into the daemon. Installation does not enable the gallery; enable it in Settings → Add-ons → Image Gallery. Add named local directories or mounted NAS paths there. On Ubuntu the initial source is `/srv/picture`; saved sources, including an explicitly empty list, take precedence. Sources are stored next to the daemon database in `image-sources.tsv`. The daemon needs read access to the selected directories. Originals are never modified.

The browser loads the gallery runtime only when enabled and available. Grid and Cover Flow, keyboard/D-pad, mouse/touch navigation, fullscreen rotation/zoom/info, slideshow and device-local display preferences are implemented; actual TV/CEF visual acceptance is pending. Thumbnails use the shared FFmpeg artwork preview implementation with a separate cache directory and 896px profile. FFmpeg is recommended. Unsupported or unavailable previews do not substitute full originals. A single original is limited to 64 MiB. The gallery does not recursively enumerate arbitrary mounts.

## Public API v1

All gallery endpoints use the existing Core HTTP authorization path. The existing addon administration permission protects settings and activation; existing backend-scoped recording-view permission is used for read-only gallery access. No new role, database, Agent channel or polling loop is introduced.

| Method | Path | Purpose |
| --- | --- | --- |
| GET/POST | /api/v1/addons/image/activation | Existing audited desired-state and revision contract |
| GET/POST | /api/v1/addons/image/sources | Admin-only local source configuration |
| GET | /api/v1/addons/image/library | Enabled state and source IDs/titles |
| GET | /api/v1/addons/image/browse | Current source-relative folder entries and covers |
| GET | /api/v1/addons/image/preview | Bounded JPEG preview |
| GET | /api/v1/addons/image/photo | Original bytes on demand |

GET requests carry `backendId`; browse/preview/photo also carry `sourceId` and `path`. The provider confines relative paths to configured roots. Admin source mutation uses the same percent-encoded TSV snapshot pattern as the source editor. Absolute source roots are returned only to administrators. Only local backend requests are implemented.

## Development / staging

`make stage-image-gallery-runtime DESTDIR=/isolated/root PREFIX=/usr` stages the metadata plus the gallery UI in an explicitly selected isolated destination. The ordinary base install excludes the UI. The generic `make stage-addon MODULE=image ...` and `make build-addon-deb MODULE=image ...` remain **metadata-only** contracts; they are not functional gallery package builds. See [AGENTS.md](AGENTS.md), [ADR-0068](../../docs/adr/ADR-0068-optional-addons-and-source-packaging.md) and the [roadmap](ROADMAP.md). Metadata retains the existing inert manifest schema and does not grant rights or register arbitrary executable code.

Focused tests: `test-addon-image-catalog`, `test-addon-image-local-browse`, `test-addon-image-source-settings`, `test-addon-image-api`, `test-addon-image-gallery-frontend`. Full CI, actual package installation/upgrade and device acceptance must be recorded separately. No live installation is part of this work.
