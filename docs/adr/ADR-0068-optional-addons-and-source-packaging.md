# ADR-0068: Optional Add-on Boundary and Source Packaging

## Status

Accepted as an architectural direction on 2026-10-08. The packageable scaffold delivered under `modules/` does **not** establish a runtime plugin loader or enabled media processing.

## Context

VDR-Suite already has internal domain modules, an in-browser `VdrSuitePlatform` module registry and empty `modules/{rectools,image,music,tvscraper}` placeholders. Internal C++ module boundaries are not a stable public ABI (ADR-0037). External VDR tools remain behind adapters (ADR-0005/0026); the daemon owns policy, jobs, metadata and APIs (ADR-0013/0043/0061).

Adding Rectools directly to core would couple import/optional transcode dependencies and unsafe legacy filesystem workflows to every installation. Building a general third-party C++ plugin ABI now would promise stability we cannot prove.

## Decision

VDR-Suite shall support **optional source-packageable add-ons** on top of existing Suite-owned services. Stage 1 is an inert, declarative add-on source/metadata/package contract; later product slices may add trusted adapters, workers and client UI through reviewed, versioned integration points.

### Ownership

- **Suite Core:** actor authentication/permissions, CSRF where relevant, backend registry/capabilities, protected API contracts, jobs/operation lifecycle, audit, VDR-native recording editing, media sessions and recording reconciliation.
- **Add-on:** a bounded domain handler, metadata or media workflow, optional UI bindings and capability declarations. It may maintain domain state only through an explicitly designed core-owned database/service boundary.
- **External processors:** restricted subprocess/worker behind the owning backend/Agent. They are not browser APIs, VDR plugin threads, database clients or independent schedulers.
- **Clients:** discover only server-authorized capabilities; a hidden menu item is not an authorization control.

### Module package metadata v1

Every `modules/<name>/` contains `addon.json`, `README.md` and `AGENTS.md`. The versioned JSON manifest has a strict schema and a unique module ID/package name. For the scaffold release its state is `scaffold`; **entrypoints, advertised capabilities, required permissions and dependencies are empty**. Presence/staging must never activate routes, inject frontend scripts, launch processes or grant rights.

`make stage-addon MODULE=<name> DESTDIR=<isolated-root> PREFIX=/usr` stages only this metadata and human documentation under `/usr/share/vdr-suite/addons/<name>/`. No optional add-on is part of base `make install`. An explicit `make build-addon-deb` can produce an inert `~scaffold1` binary Debian package from that source staging and user-supplied maintainer metadata; it does not install or activate anything. Full runnable add-on packages and Debian source-package metadata require implementation-specific dependency, maintainer and migration work.

Package builders must use the same staging contract rather than maintaining an independent file list. They must not install runtime components into live paths from a checkout. FHS paths, configuration ownership, upgrades, remove/purge and migration rules continue to be governed by ADR-0037.

### Future runtime activation gate

Before any manifest can declare runnable entrypoints/capabilities, add a separate approved design and tests for:
- signed/trusted or administrator-installed package provenance and version compatibility;
- fail-closed activation, provider presence/health and capability negotiation;
- least-privilege actor/operation permissions and backend/source scoping;
- versioned public API contracts, client feature discovery and UI asset ownership;
- isolated worker process, fixed command allowlist, safe path handling, job/idempotency/fencing and restart recovery where required;
- migrations, upgrades/rollback, optional dependency management and removal lifecycle;
- independent, safely package-staged install/test and real backend acceptance.

Do not expose an arbitrary dynamic library loader, arbitrary shell hooks or manifest-specified host paths as an extension API. No generic plug-in manager service should be created without a demonstrated runtime owner and failure model (ADR-0063).

### First proof: Media Tools

The Rectools-backed Media Tools add-on is the first intended runtime slice, limited to import, **read-only** media checking and later explicitly gated shrink. No repair/smart_repair, PES-to-TS, cutting, marks, rename or move. The existing Rectools source must be hardened before Suite can execute it; do not reuse the legacy world-writable worker.

Images and Music are independent media domains (not forced into VDR Recording identity). TVScraper is a metadata provider adapter, not a second metadata authority. The same manifest/AGENTS/staging contract applies to each.

## Verification, rollout and non-goals

`make check-addon-contract` must validate the source/manifest contract, unique IDs/packages and isolated metadata staging. It does not prove user-visible add-on execution. No live services, packages, VDR plugins or databases are touched by this scaffold.

Out of scope for this decision: dynamic third-party marketplace, untrusted arbitrary code execution, public C++ ABI, automatic `apt` installation from the Suite web UI, and activation of any Rectools mutation.

## Related

- [Modules overview](../../modules/README.md)
- [Source-package contract](../development/addon-source-packaging.md)
- [ADR-0037](ADR-0037-packaging-install-api-boundary.md)
- [ADR-0043](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0063](ADR-0063-mutation-complexity-proportionality-reuse.md)

## C++ import implementation clarification (2026-10-08)

The first Media Tools implementation is being written in C++17 and built as an **optional, standalone CLI/library**, not as a port of Rectools' Bash worker. The old CLI remains available for existing installations but is **not** the execution provider for new Suite import. A later Slice must stage the architecture-specific executable in a distinct add-on package, define its constrained backend Agent invocation and enforce core-owned actor permissions, durable jobs and recording reconciliation before enabling capabilities. The metadata-only scaffold packages remain inert. No public C++ ABI is promised.

## Installed metadata registry extension (2026-10-08)

A distinct, read-only installed module inventory is now implemented by
`tools/addons/installed_registry.py` and tested with isolated installed
package roots. It uses the same source-manifest validator and a core-owned
list of reviewed module/package identities. A manifest cannot grant
capabilities, insert rights or activate itself. The `plan-enable` entrypoint
only explains why activation is denied and never changes state. **No production
module management API or activation flow is implied.** See the
[installed registry contract](../development/addon-installed-registry-contract.md).

## Activation-intent persistence proof (2026-10-09)

A separate internal SQLite repository now models *administrative intent*
per known module/backend with CAS revision, fail-closed global administrator
authorization and evidence requirements for future enable intentions. It
does **not** activate modules. No executor, daemon API, background service,
effective capability, upgrade migration or production DB schema activation
is introduced. The existing runtime trust, audit, grant-vocabulary and
backend reconciliation gates remain mandatory.
