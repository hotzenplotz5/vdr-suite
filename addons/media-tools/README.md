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
