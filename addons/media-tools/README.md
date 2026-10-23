# Media Tools add-on

This is the canonical location for **new Media Tools-specific A6 code**.
All development stays on `work/addon-platform-registry-lifecycle`; there
is no new branch, fork or repository.

The existing C++17 importer is maintained here under `include/`, `src/`
and `tests/` (moved, not copied, from the legacy source layout).
Manifest and metadata-only packaging remain under `modules/rectools/`.

Suite Core owns accounts, grants, command reservation, durable operations,
backend generation, capability discovery and recording reconciliation.
Media Tools owns only the import/check/shrink implementation. Import is
first. Media Check now has a source-level local-backend API v1 and Recording
Hero integration, but is not installed or activated as a production add-on.
Shrink is not available yet.

The add-on includes the pinned descriptor executor and protected Core/Agent
command/results path. The production Agent executable has a **strictly explicit**
`--enable-media-import` opt-in composition; it still requires a root-controlled
approval, metadata, source inventory and Core authorization. Merely installing
an add-on or setting COMMAND_TYPES cannot activate imports. No service has been
installed or restarted as part of this implementation, and the module manifest
remains disabled.

## M2: erster tatsächlich nutzbarer Media Check (read-only)

Der neue eigenständige C++17-Befehl `vdr-suite-media-check` prüft die
**TS-Paketstruktur einer vorhandenen VDR-Aufnahme**, ohne sie zu verändern.
Die zugehörige Library liegt in `include/MediaCheck.h` und
`src/MediaCheck.cpp`, der reine Lese-CLI in
`src/media_check_main.cpp`. Der Aufruf lautet:

```text
vdr-suite-media-check check VIDEO_ROOT RECORDING_RELATIVE
```

`VIDEO_ROOT` ist ein absoluter, vom lokalen Backend-Betreiber vorgegebener
VDR-Video-Root; `RECORDING_RELATIVE` benennt eine darunterliegende,
normalisierte `.rec`-Aufnahme einschließlich Titelordner. Symlink-Pfade
und nicht zusammenhängende, fortlaufend nummerierte `00001.ts`,
`00002.ts`, … werden abgelehnt. Das Ergebnis ist ein JSON-Objekt mit
`status` (`ok`, `warnings`, `errors`), analysierten Paketen/Segmenten,
TS-Synchronisationsfehlern, Transportfehler-Indikatoren, fehlerhaften
Adaptationsfeldern, unvollständigen Paketen, PID-Kontinuitätswarnungen,
Discontinuity-Flags sowie Indexdatei-Präsenz. Die Daten werden nur
lesend über Dateideskriptoren eingelesen. Exitstatus 0 steht für
`ok` oder `warnings`, 1 für erkannte Strukturfehler, 2 für eine
unvollständige/unsichere Prüfung.

Both read-only Media Check modes additionally inspect relevant VDR
sidecars (`index`, `info`, `marks`, `resume`, `.srt`, `.nfo`).
The JSON includes `sidecarReports` and counts of unsafe/nonregular or
obviously empty sidecars. FFprobe receives only a verified read-only TS
descriptor on stdin, never a browser-selected recording path; it reports
video/audio codec, resolution, channel count and container attributes
in the bounded `ffprobe` result. The Recording Hero summarizes these
attributes without implementing a separate scanner.

**M2.1 FFmpeg-Decodierung:** Die zusätzliche Aktion `check-decode`
verarbeitet die nummerierten TS-Segmente **als einen einzigen Stream** und
decodiert Video/Audio in den FFmpeg-`null`-Muxer, ohne Output-Dateien
anzulegen. Nur ein festes, nicht vom API-Nutzer auswählbares
`/usr/bin/ffmpeg` wird gestartet; die Eingabe kommt ausschließlich
über eine Pipe aus bereits geprüften, nur lesend geöffneten Segmenten.
FFmpeg wird mit `-xerror -err_detect explode` ausgeführt und nach
maximal 900 Sekunden beendet. Der JSON-Bericht enthält zusätzlich
`decoder.status` (`ok`, `errors`, `incomplete`),
`exitCode`, `timedOut`, `streamedBytes` sowie begrenzte Diagnosezeilen.
Timeouts und fehlendes FFmpeg sind **unvollständige Prüfungen**, keine
erfolgreich geprüften Aufnahmen. CLI-Exitstatus 0 = keine harten
Fehler, 1 = dekodierte/strukturelle Fehler, 2 = unvollständig/unsicher.

```text
vdr-suite-media-check check-decode VIDEO_ROOT RECORDING_RELATIVE
```

Dies ist eine vollständige Software-Decodierprobe über alle vorhandenen
TS-Segmente – nicht der Nachweis garantiert störungsfreier Wiedergabe
auf jedem Client. Die separat getestete `check`-Aktion bleibt ein
schneller reiner Struktur-Scan. **M2.2/M2.3 Entwicklungsstand:** Die Versioned API kann per
`POST /api/v1/backends/{backendId}/media-checks` einen reinen Lesecheck
mit den Feldern `recordingId` (Suite-eigene `rec_...`-Kennung) und
`mode` (`structure` oder `decode`) starten. Das Ergebnis ist unter
`GET /api/v1/backends/{backendId}/media-checks/{checkId}` abrufbar.
`GET /api/v1/backends/{backendId}/capabilities/media-check` gibt die
lokale Verfügbarkeit zurück. Der bestehende Recordings-2-Hero zeigt
`Aufnahme prüfen` und das Prüfergebnis an.

**Wichtige Grenzen:** Diese Entwicklungsversion arbeitet ausschließlich
auf serverlokal vorhandenen VDR-Videoverzeichnissen. Die
Daemon-Umgebungsvariable `VDR_SUITE_MEDIA_CHECK_VIDEO_ROOTS` enthält
die expliziten, backend-spezifischen Wurzeln im Format
`backendId=/ABSOLUTER/VIDEO_ROOT` (mehrere Einträge mit Semikolon).
Ohne diese Konfiguration wird die Funktion nicht angeboten.
Die Aufnahme wird serverseitig anhand der `rec_...`-Kennung und eines
aktuellen Backend-Snapshots erneut aufgelöst. Browserpfade werden nie
ausgeführt. Genau eine Prüfung läuft gleichzeitig; abgeschlossene Berichte
werden in der **bestehenden Suite-Core-SQLite-Datenbank** archiviert und
über den Check-ID-Endpunkt nach einem Daemon-Neustart wiedergefunden.
Eine beim Absturz laufende Prüfung wird als `incomplete` gespeichert,
nicht als Erfolg. Jeder Status-Abruf prüft das backend-spezifische Recht
`media.recording.play` erneut und bindet den Bericht an den ursprünglichen
Akteur und das Backend. Die Anbindung an Remote-Agenten und die Migration
in die Core-Operations-Jobausführung fehlen weiterhin. Es gibt
weiterhin **keine** produktive Add-on-Aktivierung oder Paketinstallation,
keinen automatischen Reparatur-, Lösch- oder Mailversand. Eine echte
yaVDR-Abnahme ist ausdrücklich noch offen.

`make addon-media-check` kompiliert nur die CLI;
`make test-addon-media-check` und
`make test-addon-media-check-api` verwenden ausschließlich künstliche
TS-Pakete und temporäre Verzeichnisse und prüfen die Aufnahmen auf
Unverändertheit. Der Hero-/Client-Vertrag wird durch
`web/frontend/tests/test_media_check_composition.js` geprüft. Keine Marker, Mail, Reparatur,
Index-Neuerzeugung, Änderung der Aufnahme, automatische Paketinstallation
oder Add-on-Aktivierung. Das bestehende Media-Import-Interface bleibt
unberührt; `addon.json` bewirbt Media Check noch nicht.

## Importer: source-preserving metadata and codec fallback

The existing backend C++ import implementation remains the only importer.
A root-controlled opaque source selection proceeds through the existing
Suite API v1 and Core/Agent operation identity and verifies the recording
via the existing VDR recording-cache reconciliation. Video in H.264, HEVC
or MPEG-2 uses stream copy; other video codecs use bounded software
H.264/AAC encoding with FFmpeg and fixed arguments. In both cases a private
input snapshot and output-size limit protect the staging volume. External
tools have an explicit deadline, and the original source file is never
moved or deleted. Capacity preflight leaves the required free-space reserve.

If an adjacent same-stem `.nfo` exists, bounded plain-XML fields
`title`, `outline`, `plot`, `year`, and `genre` are copied into
the resulting recording's native VDR `info` file. XML beyond the
supported simple tags/entities is rejected rather than silently interpreted.
An adjacent same-stem `.srt` is preserved as `00001.srt`, the
existing Suite subtitle reader's preferred sidecar. Language-tagged
companions such as `Movie.de.srt` and `Movie.en-US.srt` are also copied
as bounded `subtitles.de.srt` or `subtitles.en-US.srt`. Originals remain
untouched; all sidecars together have a 16 MiB staging budget. Compatible
video/audio tracks remain stream copy; incompatible video is encoded as
H.264, incompatible audio as AAC. The title directory is matched to the NFO
title when supplied and duplicate recordings are rejected.

An explicitly selected browser **Mehrfachimport** can now submit currently
authorized source IDs sequentially through separate durable Core Operations.
It advances only after canonical `succeeded` readback, never after an
ambiguous Agent result, and stops if its bounded status polling ends. This
is a foreground browser feature, **not** an unattended daemon scheduler.

### HEVC shrink: offline review workflow only

`make addon-media-shrink test-addon-media-shrink` builds the real C++
converter and its isolated source-preservation tests. A backend-local
administrator can inspect `vdr-suite-media-shrink plan VIDEO_ROOT RELATIVE.rec`
without writing, explicitly stage a private HEVC candidate with
`stage VIDEO_ROOT RELATIVE.rec --confirm-staging` and separately publish
as a **new** recording with
`publish VIDEO_ROOT RELATIVE.rec CANDIDATE --confirm-new-recording`.
Existing TS segments, the VDR index and native `info` sidecar are
validated; the original recording is never replaced or deleted. Optional
`--max-height=720`, `--night-mode`, and
`--encoder=software|vaapi|qsv|nvenc` affect staging only. Hardware
failure can fall back to software. Compatible AAC/AC3/E-AC3/MP2 audio
is preserved by stream copy; incompatible audio is encoded to AAC (and
night mode deliberately uses AAC). FFprobe duration/codec, smaller output
size and VDR index must pass before review. Publication rechecks the
source identity, the staged metadata/subtitle bytes and retained HEVC
space savings before an atomic no-replace rename. The review/publish binaries remain outside
the standard daemon and Agent mutation/API composition.

The new `MediaShrinkOperationSelection.h` is a **non-executing**
path-free request shape for the eventual Core/API-v1 bridge. It only
checks opaque `rec_...` recording IDs, `shk_...` review IDs and supported
options; it neither resolves identifiers nor grants permissions, creates
candidates or enables a route. Core must independently bind a candidate
to the actor/backend/source operation, persist its lifecycle and require
separate authorization for staging and publication. Browser filesystem
paths remain forbidden. Its invalid-shape regressions run in the existing
`test-addon-media-shrink` target. No new media mutation is exposed.

### HEVC read-only API v1 and durable Core review lifecycle

The Recording Hero exposes **HEVC-Vorprüfung** only when an explicitly
configured local backend video root and current snapshot are available.
Its canonical JS client lives at `clients/reference-js/public-v1-client.js`
and is reused through the original frontend symlink.
`POST /api/v1/backends/{backendId}/media-shrink-plans` accepts only an
opaque `rec_...` identifier; it resolves the path server-side and calls
`inspectShrink` **read-only**. The security gate requires an authenticated
actor, backend-specific `media.recording.play`, CSRF and accountability.
`GET .../capabilities/media-shrink` exposes `planningAvailable` but
deliberately reports `available:false`: no shrink write action is enabled.
`GET /api/v1/backends/{backendId}/media-shrink-reviews/{reviewId}`
reads a persisted `shk_...` candidate's `staged`, `publishing` or
`published` status. Every read rechecks the effective backend-specific
`media.recording.play` grant, audits the decision and reads only the
requesting actor's own Core review row. No staging path, agent credential or
source fingerprint is returned; the response says `publishAvailable:false`
until the separately authorized publishing pipeline is implemented.

Core `MediaShrinkReviewRepository` stores an actor/backend-scoped review
binding in the **existing SQLite authority**, not a second job queue.
A candidate requires a succeeded `media.shrink.stage` Operation with
matching opaque result reference and generation. A separate reserved
`media.shrink.publish` Operation claims the review under an exact revision.
Unresolved/ambiguous publication remains claimed, never freely retryable.
A succeeded canonical publish Operation with new opaque recording ID is
necessary to mark the review published. The daemon initializes this schema
alongside the existing Operations store; disposable tests cover optimistic
revisions, cross-actor isolation and database restart.

The canonical Agent wire parser now recognizes exact, version-1 path-free
`addon.media.shrink.stage` and `addon.media.shrink.publish` payloads.
`MediaShrinkCommandFence.h` only assesses consistency with the immutable
Core Operation and payload, never authorization. A configured
`COMMAND_TYPES` entry **cannot advertise or execute** either command
without a trusted Agent executor. Tests cover forged paths, malformed
options, stale backend generations, request/operation mismatches and
terminal publication replay. No command is yet assigned or executed.

**Not productive execution yet:** there is no Agent-authorized stage/publish
handler, opaque candidate-to-file binding, dedicated effective shrink grant,
or recording-cache reconciliation for publication. The UI only performs
the read-only plan. This checkpoint changes no live recordings and does not
enable the addon manifest.

### Administrator-opted unattended imports (source integration)

The **existing daemon event loop** can now evaluate a single backend's
Agent-published sources once a minute during a configured local-time window.
The root-managed `/etc/default/vdr-suite-daemon` can opt in using all three
non-secret values below; their absence or an invalid window keeps automatic
imports disabled:

```ini
VDR_SUITE_MEDIA_IMPORT_AUTO_ACTOR=existing_service_actor_id
VDR_SUITE_MEDIA_IMPORT_AUTO_BACKEND=existing_backend_id
VDR_SUITE_MEDIA_IMPORT_AUTO_WINDOW=01:00-05:00
```

The actor must already exist as an active **Service** identity, hold the
backend-specific `addons.media.import` grant, and pass all current Core
activation, Agent capability and backend write fences. The administrator
must separately authorize the Agent's local inventory and writable staging
path. Configuring the three values does not create a service identity,
grant privileges or enable an add-on. There is no new queue: scheduling
calls the same Core admission/assignment used by API v1. At most one
source is submitted per minute after consulting the existing durable
`media.import` Operations. A running or ambiguous operation blocks the
next one. Stable idempotency keys prevent retries and duplicate imports
across daemon restarts; a failed source requires manual review. Windows
may cross midnight (for example `23:00-04:00`). There is no browser
toggle or automatic production deployment yet.

**Not yet production-complete:** the Agent's 600-second assignment
deadline limits long imported video transcodes. Unattended scheduling is
implemented in source but has not been accepted on a real VDR host.
External subtitle search or playback language preference, VDR-compatible
chapter conversion, remote Media Check, and shrink through authorized API
v1/Core Operations remain open. The existing Suite recording-cache enrichment and artwork owners
must remain the only metadata/scraper architecture. A host acceptance
using real VDR recordings has not been authorized or performed. The
default backend-agent systemd unit also denies writes outside its own
state directory; administrative activation requires a separately approved
write-path policy, not a permissive package default.

## Installed handler metadata gate (A2M/A4)

A root-approved handler is no longer considered available from binary bytes and
an approval JSON alone. Its separate installed
`/usr/share/vdr-suite/addons/rectools/addon.json` must also be root-owned,
free of writable/symlinked parents and exactly match the approved module,
package and version. The current schema-v1 manifest must remain a disabled
scaffold with empty executable, permission and capability declarations.
Source/manual installations and eventual Debian packages use this same fixed
path; neither dpkg presence nor the manifest itself grants authorization.
The combined snapshot revalidates administrator approval after hashing, and
the pinned Agent executor revalidates the installed manifest at dispatch.
The recognized handler interface is fixed at reviewed version `0.1.0`;
a newer installed binary/manifest does not gain execution compatibility by
self-reporting a matching version to an approval. A privileged,
administrator-invoked `admin_approval_issuer.py rotate` command can
atomically exchange one active **same-version** approval for another, but
only when the exact old SHA-256 is supplied and the replacement binary matches
a separate approved digest. The record is never overwritten by `issue`,
and `rotate` cannot reverse `revoke`. Root-owned disposable CI fixtures
test CLI rotation, stale expectations, replay, missing binary, missing
metadata, and persistent denial after uninstall/reinstall. No Debian
maintainer script invokes this command; a future package upgrade/remove
sequence must explicitly coordinate disable, drain, revoke, install,
review, and optional re-approval before real activation. Product activation
and full installer/upgrade handling remain separate acceptance steps.

## A4 preflight for later package maintenance (read-only)

Core now exposes an internal read-only maintenance inspection that checks
the canonical per-backend administrative disabled intent and the real durable
`media.import` operations. Accepted, queued, dispatching,
executed-unverified and outcome-unknown operations block a locally quiescent
diagnostic result; missing state or a saturated operation query also blocks it.
This does not drain the Agent, prevent new admissions, serialize an installer,
audit a package transaction or prove signed Debian provenance. **No package
install, upgrade, uninstall or source/recording modification is performed.**

## A4 global reservation freeze (Core source only)

The global Core-owned `AddonMediaImportMaintenanceFence` now records
administrator-audited, revision-fenced freeze/unfreeze state in the same
SQLite file as the existing durable import operation store. New
`media.import` reservations are checked *inside* the operation insert
transaction, across every backend; unrelated timer operations are unaffected.
The default remains the normal disabled Add-on contract. Missing schema
denies new imports, and previous exact operation replays are still readable.

This does **not** stop an already executing Agent process, grant an update,
install a package, or reconcile an unknown result. No public management route
is exposed. The read-only no-unresolved-import inspection, stopped Agent,
independently reviewed replacement bytes, package provenance and explicit
host approval remain necessary before any real package maintenance.

## A4 Agent child-lifetime drain fence (isolated source)

The fixed root-controlled
`/etc/vdr-suite/addon-approvals/rectools.import-running.lock` is a separate,
persistent, empty, root-owned and non-writable-by-others **run-lock inode**.
Before opting in, a source/manual or future Debian installation must
independently stage it under the same reviewed installation root; neither
the Agent nor a package action creates it on demand. Missing, symlinked,
writable, replaced or otherwise unsafe lock evidence denies Media Import
at both Agent capability discovery and actual handler dispatch.

A pinned Media Import executor holds a **shared advisory Linux flock**
from BEFORE its final administrator/manifest/authorization/source rechecks,
through `fork`, and across the imported executable's `fexecve` (including
its ffmpeg descendants). The unprivileged Agent retains its own shared FD
while waiting; the child explicitly inherits the lock FD even if its Agent
parent crashes. A future maintenance owner must obtain and **hold** an
exclusive lock on this exact protected inode throughout all changes to the
approved binary and its associated metadata, but only AFTER globally freezing
new imports, explicitly disabling the addon for all affected backends,
draining canonical `MutationOperation` and durable Agent assignments,
and independently verifying package provenance. An exclusive try-lock
without those other checks is NOT a safety proof or an update permit.

Disposable root-controlled CI fixtures exercise missing/writable/contended
locks, the real unprivileged Agent, and an ELF child that retains the kernel
lock after exec. No installer or package hooks currently acquire this lock;
the release maintenance protocol and full host-specific acceptance remain
unimplemented. No VDR data or actual recordings are modified by these tests.

## A4 global Core/Agent quiescence inspection (diagnostic)

A Core-only, admin-gated `AddonMediaToolsMaintenanceInspection` now
combines the global frozen-admissions revision, all backend activation
intents, all unresolved Media Import Operations, Agent assignments and
acknowledged results, residual Agent capability advertisements and the
root-owned exclusive handler lock. It refuses a positive diagnostic if a
state table is unavailable, an import is uncertain, an assignment has not
completed, an Agent still advertises the command, or another process
holds the shared execution lock. The source-only test uses disposable
root fixtures and a synthetic schema compatible with actual Agent rows.
The server also rechecks the durable freeze when an already assigned Agent
command seeks receipt authorization.

**Still not a maintenance permit.** The inspection releases the exclusive
lock immediately; no installer is coupled to it and a competing admin
could otherwise unfreeze. A final authorized package owner must hold
locks through the whole transaction, attest exact Agent state/database
identity, review release provenance and provide safe rollback. None of
this is enabled on a live host.

## A4 retained exclusive inspection handle (not installation)

The global Core/Agent maintenance inspector also has an optional
`inspectAndHold()` diagnostic. On success, the move-only result keeps
the *same root-owned* exclusive Media Import lock across C++ ownership
handoffs, rather than dropping it at the end of the snapshot. A moved-from,
released or shared lock cannot count as an exclusive hold. Root CI verifies
that concurrent imports cannot acquire their shared lock while the handle
remains held. Failures never return a usable hold.

This diagnostic does not freeze future administrator writes, and its
positive snapshot can become stale. It deliberately offers no package
operation, automatic unfreeze, handler replacement, rollback, runtime
activation or public API. The eventual Ubuntu maintainer must hold its
own exclusive maintenance lease through the independently authorized
upgrade/remove process and recheck Core and Agent state under durable
maintenance ownership.

## A4 durable ownership of a frozen maintenance window

The Core-owned global Media Import admission freeze now has a *separate*
additive SQLite maintenance-owner record. It may be claimed only by a
resolved administrator who already holds the exclusive fixed handler
lock and supplies the exact frozen revision. Core creates an unpredictable
128-bit ownership token, atomically audits the claim and increments the
freeze revision. As long as that record exists, even the original
administrator cannot unfreeze via the legacy API; another administrator
cannot acquire a replacement owner. Releasing ownership requires the
same actor, token, exact revision, exclusive lock and a successful audit,
and leaves admissions **frozen** until a separate intentional unfreeze.
If the owner crashes, its durable claim remains after the kernel lock
disappears, preventing silent takeover. There is no auto-expiry.

This is source-only coordination, **not** permission to install or
remove `.deb` files. A future authorized package transaction must
preserve the same exclusive handle, confirm current agent and database
state and signed package provenance, manage safe rollback, and define
a separate audited root recovery for abandoned owners before deploying.
The module remains disabled, with no live install, public endpoint or
implicit permission grant.

## A6 work in progress

The optional importer is a single C++17 implementation under this directory,
without a second repository or branch. The read-only canonical Agent bridge
now rejects non-dispatching operations, stale generation, missing Agent
instance/claim identity, mismatched request fingerprint and operations without
recording reconciliation policy. This is **not** a registered executor.

The import plan now records the source device, inode and nanosecond modification
time. A same-size replacement between `plan` and explicit `import`
is rejected before probing or remuxing; focused tests exercise this swap.
Source bytes are never deleted by this importer. The existing Agent command
inbox and durable receipts are integrated in isolated tests; productive
registration and real backend inventory/grant adapters remain gated.
No production capability is advertised.

### Immutable per-import media input

After validating a planned source's identity, the offline importer now
copies the opened bytes into a private, bounded staging snapshot on the
destination filesystem. Both `ffprobe` and `ffmpeg` consume this
same snapshot instead of reopening the source pathname. The copy is
checked for length and original file identity; the snapshot is removed
before atomic no-overwrite recording promotion. The caller-owned
source is never deleted. Focused tests confirm that a replaced
same-size file is rejected before probing, and that probing and
remuxing share the same private input. This does **not** substitute
for Core's canonical recording reconciliation and Agent job lifecycle.


## A6 isolated durable dispatch and pinned execution candidate

The existing Core Agent command inbox now accepts the strict opaque
`addon.media.import` assignment schema with `reconciliation_required`,
persists an accepted receipt, writes `starting` before invoking an explicitly
injected executor, and emits a durable unknown/unverified result for recording
reconciliation. Crash recovery never blindly runs a remux twice.

`PinnedMediaImportAgentExecutor` is the concrete add-on-owned Agent execution
implementation. It rereads root approval, retains the verified binary descriptor
from the common Core install snapshot, resolves only a local inventory token,
rechecks the effective backend-specific authorization hook, and invokes the pinned
descriptor without a shell with bounded child lifetime. No file paths are read
from the API command payload; the original recording is never deleted.

The trusted inventory and grant adapters and the server-side operation completion
are **not yet wired to production**. `apps/agent/main.cpp` deliberately injects
no executor or command advertisement. This is testable in isolated CI without
mutating yaVDR; it is not yet a product-ready Media Import feature.

The isolated A6 CI now also exercises `BackendAgentClientRuntime` itself with an injected synthetic executor: authenticated heartbeat, command poll, durable receipt, execution, result acknowledgement and subsequent heartbeat replay without a second dispatch. The shipped Agent still injects no Media Tools executor.

The existing Agent delivery service additionally refuses Media Import receipts unless its trusted `setMediaImportReceiptFenceCheck` verifies the exact reserved operation, effective grant and claim. The daemon now installs that callback using the real MutationOperation, immutable payload, account/grant, backend and Agent-capability authorities. Durable Agent results update the *same* MutationOperation to outcome_unknown or an appropriate verified rejection via a revision-fenced transition; neither remux exit nor Agent receipt marks an import succeeded. Production execution still remains disabled because apps/agent/main.cpp does not install or advertise an executor and productive backend inventory/grant adapters remain unavailable. Positive and negative tests operate exclusively on in-memory SQLite and synthetic Agent contexts.

The pinned executor snapshots the trusted local source/target resolution during authorization and rejects remapped inventory selections immediately before `fexecve`; CI explicitly tests the swap after authorization as a negative case.


### A6 authoritative recording readback (isolated)

The pinned handler's bounded stdout is parsed according to the actual C++17
`std::filesystem::path` quoted-output format. The Agent stores only a
SHA-256 correlation digest of the committed absolute `.rec` directory,
never the source or destination pathname, in the existing durable command
result. The protected local Agent state stores this identity in its scoped
version-4 Media Import representation, with compatibility for older command
state versions. A synthetic non-writing ELF fixture exercises this output
through the real pinned-fd execution path in CI.

The daemon reuses its *existing*, read-only, coalesced native Recording cache
refresh. Only a successful refresh that **started after** the Agent result,
matches the backend, contains exactly one matching VDR recording with
non-empty native identity and title, and matches the persisted command,
operation, and immutable payload may advance the shared
`MutationOperation` to `succeeded` by revision-fenced CAS. Empty, stale,
unmatched, ambiguous, mismatched-generation and replay cases remain denied
or `outcome_unknown`. An exact Agent result replay after `succeeded` can
be acknowledged without another dispatch or state regression.

A crash after native promotion but before a usable handler receipt can
remain `outcome_unknown` and requires independent operator reconciliation;
there is deliberately no blind import retry. Recording-cache path
normalization differences likewise fail closed rather than invent success.
**This is not production activation:** trusted local inventory/grant runtime
composition, final operator/readback acceptance and effective capability API
discovery still require further implementation and CI. No live VDR data is
modified by hosted tests.

### A6.1 local source mapping (default denied)

`RootOwnedMediaImportInventory` reads a *backend-local*, root-owned
`/etc/vdr-suite/addon-sources/media-import.tsv` (relative to its trusted
installation root). Each TSV row binds one exact backend ID, opaque source ID
(minimum 16 URL-safe characters), configured source root, VDR video root and
a source file. No payload-derived paths, scanning service or second database
are introduced. Missing, symlinked, writable, ambiguous, malformed or
unreadable mappings deny. Every selected entry passes the existing C++17
`inspectImport` checks and carries source inode, device, size and mtime
identity into the executor's second resolution before `fexecve`. The ledger
does **not** grant import execution; the current production Agent remains
unregistered/default-deny until effective server-side authorization is wired.
Only disposable, root-controlled CI fixtures are used by its tests.

### A6.2 effective Core permission revalidation (not activated)

`ReceiptRevalidatingMediaImportAuthorization` reads the existing durable
Agent inbox and replays its *previously acknowledged exact receipt* over the
existing authenticated Agent transport. The server reevaluates the canonical
operation, current backend-specific grant, live Agent identity/generation,
lease, claim and command. Only an exact successful receipt **replay** allows
dispatch; missing or inconsistent state, revocation, network or server failure
deny. The check occurs both before and after durable `starting`, before the
pinned handler is executed. It does not create a second permission database.
Production composition remains default-deny until the full activation gates.

### A6.3 server activation gate

The *same* Core `AddonActivationIntentRepository` now supplies a necessary
per-backend administrator desired-enabled condition to the **actual** media
import operation/receipt fence. Missing schema, absent row, disabled intent or
database failure denies before dispatch even when a user still has the
independent `addons.media.import` grant. It is not an execution capability
by itself. The isolated command-delivery test exercises default deny and
administrative disable/re-enable while retaining the existing grant tests.
No production Agent registration or handler activation is performed by this.

### A6.3 explicit production Agent composition

`apps/agent/main.cpp` now owns the real Media Import Agent composition:
root-controlled `RootOwnedMediaImportInventory`, effective Core receipt
revalidation and the Add-on's `PinnedMediaImportAgentExecutor`. The Agent
config parser still rejects `addon.*` in `COMMAND_TYPES`. Only an
**explicit** `--enable-media-import` runtime option installs this
composition; the packaged normal Agent starts without it and continues
default-deny. Even with opt-in, capability discovery requires a current
Agent identity/generation, trusted pinned install bytes and at least one
valid root-owned local source. At each command, the server also requires an
enabled audited per-backend administrator activation intent, effective
actor grant, live Agent, receipt and immutable operation. All requirements
are checked again before the handler runs. `Media Check` and `Shrink`
remain disabled.

The root-only A6.3 composition test uses the *actual* Agent capability
discovery function, the real protected local command inbox, the real
install-snapshot and root-ledger adapters, the new Core receipt-replay
authorization adapter and the Add-on pinned-fd executor. Its synthetic
handler is a copied `/bin/true` ELF; no VDR recordings are touched.
Negative cases include default missing executor, revocation, stale backend
generation, writable inventory and a modified source before execution.

### A6.4 contiguous real-Agent synthetic lifecycle

The isolated root-only composition test also drives the *existing* protected
Agent inbox, first authenticated receipt, two exact effective-grant replays,
durable `starting`, pinned `/bin/true` ELF execution, protected result
persistence, authenticated result acknowledgement and restart replay without
a second execution. Revocation immediately after receipt but before
dispatch denies, and recovery from persistent `starting` does not run an
importer. Every test uses disposable CI root fixtures and no real VDR
recordings. The real Core command-delivery test separately proves that only
fresh matching recording-cache readback can transition an operation to
`succeeded`. The explicit `--enable-media-import` option is not allowed
in `--once`, credential-rotation or native-probe modes.

### A7.5 capability-gated Webfrontend (source integration only)

The shipped Webfrontend's real `index.html` script composition loads
`modules/media-import.js` before `app.js`. The established backend
selector and `app.js` module-navigation owner show the initially hidden
**Media Import** tab only when authenticated
`GET /api/v1/backends/{backendId}/capabilities/media-import` returns
the exact backend, capability and `available:true`. Backend switches,
session expiry and logout invalidate that view; activation checks again.

The view lists only opaque `sourceId` tokens using the public v1
source-list route, refreshes the ephemeral list before submitting, and sends
only one `sourceId` with a cryptographically random, caller-owned
`Idempotency-Key` and the normal browser session CSRF header through
`POST /api/v1/backends/{backendId}/media-imports`.
It observes only the existing authenticated
`GET /api/v1/operations/{operationId}` resource, with bounded active-view
reads and explicit manual refresh. Core states including
`executed_unverified` and `outcome_unknown` are never represented as
success. Unknown POST results block a fresh request for that backend in
the current browser session; there is no automatic resubmission.
A new import is offered only after a terminal operation state.

This is **source integration, not a production release**. Only an explicitly
opted-in and trusted Agent, effective Core grant, activation intent and
independent native Recording readback can authorize and confirm execution.
No production frontend installation or host acceptance has occurred; Media
Check and Shrink remain disabled. The production composition and user-action
regression is `web/frontend/tests/test_media_import_composition.js`.

### A7.4 per-backend source list, no filesystem paths

An explicitly opted-in Agent reads its root-controlled TSV locally and
reports at most 32 freshly inspected opaque `sourceId` tokens via
authenticated `POST /api/agent/v1/observations/media-import-sources`.
Missing approval or an invalid inventory sends an empty snapshot. The daemon
holds this report only in RAM, bound to the current Agent actor, instance,
backend generation, active lease and a maximum 90-second age.

Authenticated `GET /api/v1/backends/{backendId}/media-import-sources`
rechecks the SecurityHttpGate discovery scope and Core's live
`addons.media.import` authorization/activation/Agent capability before
exposing the bounded IDs. Missing/expired observations yield 503, denied
users 403, unscoped backends 404. No paths or writable source claims are
returned. Actual import rereads and pins the local source from the
protected ledger and requires the canonical VDR recording readback.

### A7.3 protected Media Import admission

`POST /api/v1/backends/{backendId}/media-imports` accepts only
`{"sourceId":"opaque_source_123456"}` and a valid
`Idempotency-Key`. The common SecurityHttpGate requires
authenticated backend-specific `addons.media.import` authorization,
browser-session CSRF and audit. The Core admission service separately
reevaluates the effective grant, agent lease/capability, enabled
administrator intent and writable backend. It atomically reserves one
ADR-0042 `media.import` operation with immutable payload and
scope-specific idempotency. The daemon then invokes the *existing* system
authority Agent assignment path and returns the same v1 Operation resource
on retry. No source pathname, output pathname, arbitrary command body or
new media worker queue is accepted. A failing assignment can leave an
accepted, non-executed durable operation which the same-key caller may
safely resubmit; terminal operations are never reassigned.

A public POST is *not* proof of verified execution: the Agent needs
separate root-owned inventory selection and explicit opt-in. Completion
requires independent readback of the new VDR recording.

### A7.2 authenticated public API v1 capability read

`GET /api/v1/backends/{backendId}/capabilities/media-import` is a
read-only actor/backend-scoped view. The existing HTTP gate supplies
independently authenticated discovery scopes; an out-of-scope backend
returns 404. The daemon only delegates the live check to Core's
`effectiveMediaImportCapability`. An absent delegate or unavailable Agent,
activation or effective user permission yields `available:false`, not an
optimistic flag or information about internal rejection reasons. The route
never accepts a path selected by the caller as an import source, creates
no operation and performs no import. This is **not** a production launch.

### A7.1 authoritative backend capability discovery

`MediaImportOperationFence::effectiveMediaImportCapability` evaluates the
current nonrevoked Agent identity, live lease and exact published
`addon.media.import` capability **together with** the existing Core
user/service actor, backend-scoped effective grant, writable backend and
audited administrator enable intent. It shares the dispatch code's actor and
backend authority evaluator rather than duplicating a permission system.
Missing/stale/invalid input is default-deny. This is a read-only capability,
not a mutation or permission grant. The real Core operation/command fixture
covers the enabled, grantless, disabled, wrong backend and stale lease cases.
