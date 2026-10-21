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
first; Check and Shrink are not available or grantable yet.

The add-on now contains a real, testable pinned descriptor executor and
protected Agent command/result integration. The production Agent still does
not instantiate or advertise this executor; no service is started and no
manifest capability is advertised.

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
