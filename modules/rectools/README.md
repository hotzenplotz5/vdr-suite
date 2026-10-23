# Media Tools (Rectools) – optional VDR-Suite add-on

**State (2026-10-23):** Import, read-only Check and non-destructive HEVC
Shrink have C++ implementations and source-integrated Core/API/Agent paths.
Media Check works at source level for configured local backends; Import and
Shrink require explicit Agent opt-in, authorized operations and approved
backend activation. The manifest remains disabled. Real host acceptance
and trusted administrative enablement are outstanding.

Manifest/staging location: `modules/rectools/`; implementation: `addons/media-tools/`; prospective binary package: `vdr-suite-addon-media-tools`. The legacy Rectools repository is the audited functional reference, **not** the implementation engine: the new importer is C++17 and does not call the Rectools Bash CLI. Protected backend-local Agent execution exists in source. Neither the
browser nor the VDR plugin can submit arbitrary filesystem paths or execute
shell commands.

## Allowed scope

1. **Import** local media from administrator-registered source roots into VDR recording format, with preview, codec analysis, identity/duplicate check, staged commit, index verification, provenance and library reconciliation.
2. **Check** existing recordings for media/integrity information, strictly non-destructively; report findings through Suite.
3. **Shrink** optionally; separately reviewed, explicit consent, staged result, verification and original preservation. Never a silent rewrite.

## Explicit exclusions

- `repair` and `smart_repair`: **not permitted**, including indirect invocation during import.
- PES-to-TS: handled by current VDR; do not add a Rectools adapter for it.
- cut/marks, rename, move: canonical VDR-Suite functionality; no duplicate Rectools actions.
- No use of the legacy world-writable `/tmp/vdr-rectools-jobs` worker as the Suite trust boundary.
- Do not treat today's Rectools `import`, `check_single`, or `shrink_single` as a safe public API. Import currently removes the source on success and may invoke repair. Check currently writes diagnostic markers, mail/status artifacts. Shrink currently replaces TS segments and index. These contracts require hardening, isolated verification and tests **in Rectools** before execution wiring in Suite.

## Runtime boundaries

Actor authorization and backend write policy are Suite-owned. Backend Agent generation fencing and job/idempotency guarantees apply to real asynchronous operations; no arbitrary user-supplied filesystem paths, unbounded transcoding or direct client execution. Imported VDR recordings must be re-read through the existing recording reconciliation owner. Minimize impact on live TV and existing recordings; preflight both staging and destination capacity.

## Packaging

`addon.json` is a disabled scaffold contract with zero advertised runtime capabilities. `make stage-addon MODULE=rectools DESTDIR=/isolated/root PREFIX=/usr` installs only manifest, README and AGENTS into staging. Do not claim or add runtime dependencies until the handler and worker boundary exist. The server remains functional when this add-on is absent.

See [module rules](AGENTS.md), [ADR-0068](../../docs/adr/ADR-0068-optional-addons-and-source-packaging.md) and [packaging guide](../../docs/development/addon-source-packaging.md).

To build a distinct, **inactive** metadata-only `.deb` from sources, use the explicit `make build-addon-deb MODULE=rectools OUTPUT_DIR=/isolated/output ADDON_MAINTAINER='Name <valid@example.org>'` target from the repository root. This does not install or enable any runtime functionality.

## Agent drain boundary (not packaged or activated)

For a future authorized Media Import runtime, the source-built pinned Agent
requires a separate protected root-owned, empty file at
`/etc/vdr-suite/addon-approvals/rectools.import-running.lock`.
It must be staged by a trusted administrator independently of the inert
`addon.json`. The Agent cannot create or repair this lock. A missing,
unsafe or exclusively held lock disables Media Import capability discovery
and execution. The handler inherits a shared kernel lock through its
execution lifetime so a later package maintenance owner can attempt an
exclusive lock only after stopping new admissions and draining durable jobs.
No installer or package maintainer hook currently creates or acquires this
lock, and an exclusive try-lock alone is **not** permission to change files.
The source/manual installation and final Ubuntu package release paths
must share this contract without automatically enabling the module.

## M2 Media Check – erste read-only Funktion

Unter `addons/media-tools/` liegt jetzt ein eigenständiger
`vdr-suite-media-check`-Quellstand. Er analysiert vorhandene,
fortlaufend benannte VDR-TS-Segmente ausschließlich lesend und meldet
Paket-Synchronisations-, TEI-/Adaptations- und
Kontinuitätsauffälligkeiten als JSON. `make addon-media-check` und
`make test-addon-media-check` gehören zum getrennten Funktions-/Testpfad.
M2.1 ergänzt nun `vdr-suite-media-check check-decode VIDEO_ROOT
RECORDING_RELATIVE`: Die TS-Segmente werden als durchgehender Stream
mit einem festgelegten `/usr/bin/ffmpeg` softwaredecodiert und nach
maximal 900 Sekunden abgebrochen. Der JSON-Bericht unterscheidet
Decoderfehler und unvollständige Prüfungen. Das Tool liest ausschließlich
die Aufnahme und schreibt keine Ergebnisse, Logs oder Marker hinein.
API v1, lokaler Daemon-Report und Recording-Hero-Anzeige sind im
Quellstand verbunden; Build, produktive Installation und echter Backend-Test
bleiben voneinander getrennte Abnahmen. Das `addon.json`-Manifest bleibt
absichtlich deaktiviert, bis die Freigabe erfolgt.

## C++ import prototype (not a live deployment)

Source implementation: `../../addons/media-tools/include/MediaImport.h`, `../../addons/media-tools/src/MediaImport.cpp`, `../../addons/media-tools/src/main.cpp`. Focused tests: `../../addons/media-tools/tests/test_media_import.cpp`. Build and test independently from the daemon via `make addon-media-import test-addon-media-import`. Build output: `.build/addons/vdr-suite-media-import`; this binary is **not installed** by `make install`, `stage-addon` or `build-addon-deb`.

This first slice implements an offline `plan` and an explicit `import ... --confirm-writes` for a **single** input, using allowlisted caller-supplied source/video roots. Core Operation, backend-scoped grant and Agent dispatch are now integrated
in source behind explicit opt-in. The standalone CLI remains a development
tool: **do not invoke its write command against your installed VDR video tree**. The C++ process uses fixed `/usr/bin/ffprobe`, `/usr/bin/ffmpeg`, `/usr/bin/vdr` executable paths through `execv` (no shell) only for the write path. It accepts native-compatible `h264`, `hevc` or `mpeg2video` input;
compatible streams are remuxed, other supported inputs use bounded encoding,
and a VDR index is verified before atomic no-replace promotion. It never removes or renames the source; no repair, re-encode, external subtitle search or TVScraper call is allowed. The Core/Agent integration has durable Operations and independent VDR
recording-cache reconciliation. End-to-end host validation remains mandatory
before production use.

Legacy feature inventory and security analysis: [C++ import audit](../../docs/development/rectools-cpp-import-audit.md).

## Isolated source-built C++ Debian package

`make build-media-tools-cpp-deb OUTPUT_DIR=/isolated/output ADDON_MAINTAINER='Name <valid@example.org>'` compiles and tests the standalone C++ engine, then creates an architecture-specific `~cpppreview1` Debian package using its source-produced executable. The package owns `/usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import` plus the common manifest/docs. It advertises no runtime capability, installs no service and does not connect to the daemon. **Do not install this development prototype on the live yaVDR.** The ordinary `build-addon-deb MODULE=rectools` remains the earlier, metadata-only `~scaffold1` package.

The installed catalog recognizes this module metadata only through the [read-only registry](../../docs/development/addon-installed-registry-contract.md). Installation and C++ binaries do **not** activate it, grant privileges or expose API operations.

## HEVC source integration and remaining release gate

The Agent supports explicit `--enable-media-shrink --media-shrink-state-dir
ABSOLUTE`. The Recording Hero presents read-only planning, supported
software/VAAPI/QSV/NVENC selections, height limits and Night Mode. Authorized
staging creates a private candidate; a separately confirmed Publish creates
an additional VDR recording after independent cache reconciliation. The
original recording is never deleted or replaced. Hardware encoder failure
may fall back to x265; availability depends on the actual Agent hardware.

The existing `AddonAdministrationService` still refuses **enable** without
a trusted Core provider. A source-defined executor or HTTP grant eligibility
alone cannot satisfy this gate. Trusted administrative activation, live
Daemon/Agent integration, packaging/install contracts and real-host
acceptance remain required. No repair or other excluded Rectools function
may be added.
