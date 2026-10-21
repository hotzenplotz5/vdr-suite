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

Current files are inert pre-dispatch contracts and tests. No executable
handler is registered, no service is started, and no manifest capability
is advertised.

## A6 work in progress

The optional importer is a single C++17 implementation under this directory,
without a second repository or branch. The read-only canonical Agent bridge
now rejects non-dispatching operations, stale generation, missing Agent
instance/claim identity, mismatched request fingerprint and operations without
recording reconciliation policy. This is **not** a registered executor.

The import plan now records the source device, inode and nanosecond modification
time. A same-size replacement between `plan` and explicit `import`
is rejected before probing or remuxing; focused tests exercise this swap.
Source bytes are never deleted by this importer. Actual agent command
registration, durable job receipts and authoritative VDR recording-cache
reconciliation remain missing gates. No production capability is advertised.

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
