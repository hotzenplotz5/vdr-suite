# Optional Add-on Platform — Execution Roadmap

**Scope:** Non-numbered add-on productization workstream; this document does
not reopen completed VDR-Suite phases or authorize Phase 70.
**Development branch:** `work/addon-platform-registry-lifecycle`.
All slices of this workstream remain on this branch. Main updates enter by
ordinary fast-forward-safe merge; no per-slice branch, force-push or automatic
deployment. Live status must be checked against GitHub and docs/CURRENT.md.

**Binding architecture:** [ADR-0068](../adr/ADR-0068-optional-addons-and-source-packaging.md),
[ADR-0037](../adr/ADR-0037-packaging-install-api-boundary.md) and the
[existing API/backend security owners](../development/current-architecture-state.md).

## Product boundary

Three optional Suite add-ons: **Media Tools (Rectools)**, **Images**, **Music**.
TVScraper is **not an add-on**: its VDR plugin remains a native metadata
provider integrated through SuiteBridge and the Suite Core under
[ADR-0036](../adr/ADR-0036-tvscraper-recording-metadata-integration.md).

The existing browser UI module registry does not confer backend authority.
No add-on may self-register permission grants, HTTP routes, arbitrary code,
package install scripts or executable capability merely by dropping a manifest.

## Actual install and packaging boundary

The current yaVDR has VDR-Suite installed by the existing conventional
source/manual deployment procedure, **not** by an installed VDR-Suite Debian
package. Its working SuiteBridge/VDR plugin and metadata functions must
continue to work unchanged. Add-on productization must support this mode
without pretending that a manually installed Suite is present in the dpkg
database.

Three independent concepts must never be conflated:

- **Source/manual deployment:** a later reviewed core/Agent handler with a
  trusted administrator-installed binary, explicit fixed module identity,
  version, provenance/pinned digest and lifecycle ownership. The present
  standalone C++ executable and metadata staging are **not** yet such a
  trusted/runtime-registered installation.
- **Debian/Ubuntu packaging:** package-building tools and daily builds
  already exist as development/release infrastructure. They do not prove that
  the current host has installed any particular package or binary.
- **Optional dpkg inspection:** `installed_registry.py --verify-dpkg`
  compares local dpkg ledger entries only when explicitly requested. This is
  a non-authorizing package diagnostic for the future Debian deployment
  path, **not an activation prerequisite for manually installed Suite**.

Both eventual deployment modes must enter the **same** core-owned handler,
permissions, backend fencing and operation/audit contract. No manifest path
or metadata-only presence may bootstrap trust.

## Milestones and acceptance gates

| Milestone | Product outcome | Source evidence at this checkpoint | Completion gate |
| --- | --- | --- | --- |
| A1. Optional source contract | Three independent, disabled manifests, module rules, isolated source staging and .deb scaffolds | Implemented; isolated packaging/tests accepted before TVScraper removal | Build/test all three only; no implicit base install |
| A2. Add-on inventory and discovery | Read-only core allowlist and strict metadata parsing, independent of installed VDR-Suite packaging | Metadata-only registry implemented; synthetic dpkg diagnostics added | Three add-on IDs only; no activation; default scan works without dpkg; optional ledger regression separate |
| A2M. Manual-install registration path | Explicit safe declaration of a source-built, administrator-installed and integrity-pinned backend handler | **Partial:** root-admin issue/revoke, native approval/descriptor and version-matched installed manifest snapshot, nonroot Agent-file-access CI; no automatic activation; disabled Core registry definition | Fixed reviewed module-to-binary mapping, version and digest, secure owner/mode checks, revocation/uninstall detection; no caller-selected executable |
| A3. Core access/desired-state preview | Canonical actor/backend policy, revision-fenced SQLite intent, read-only eligibility preview | C++ isolated checks 21 + 17 + 20 accepted before TVScraper removal | No execution permit; denied by default and on any lost evidence |
| A4. Trusted runtime provider | Common core authority over either reviewed manual-install or reviewed Debian package provenance, with fixed binary identity and healthy backend-local handler | **Partial:** disabled Core admission, pinned identity/generation, approval comparison, root-owned installed manifest/revocation checks and unprivileged Agent-path CI; no package-derived Core execution permit | Both paths use the same registry, signed/admin-approved provenance policy and negative trust tests; neither dpkg alone nor source files confer authority |
| A5. Administration and operation rights | Admin enable/disable with audit/idempotency/migrations; scoped media-import grant | SQLite desired-state prototype, audited admin coordinator and admin-only three-module desired-state catalog; exact backend-scoped human import grant and Core-owned permission vocabulary, categorized grant settings, backend-exact grant eligibility callback (default denied) and negative tests; live effective provider source pending | Proper grant vocabulary, authorization, CSRF, preconditions, revocation and upgrade/removal semantics |
| A6. Protected backend execution | Fenced command/worker, bounded resources and crash-safe durable jobs | **Partial:** non-executing canonical command/generation gate plus real Agent-poll denial of unregistered add-on command namespaces; no command adapter/executor | Agent generation and job fencing, cancellation/retry/readback, no arbitrary paths or shell |
| A7. Public API and client UI | Entitlement-filtered discovery, safe admin surface, addon action/status | **A7.1–A7.5 isolated source implementation and canonical install staging validated; frontend owner-handoff/late-response regressions included. Not deployed.** Capability, opaque source listing, protected admission and actor-owned Operation GET reuse Core and canonical frontend. | Versioned v1 routes, grants/session/CSRF tests, actual UI composition, source expiry and uncertain-state checks; safe install staging and separately authorized host acceptance before production |
| M1. Media Tools import | Single-file safe remux to VDR recording | Standalone C++ prototype; 11 isolated tests passed; private C++ preview .deb | Fix missing TOCTOU/fd, timeouts, crash recovery and recording reconciliation before live writes |
| M2. Media Tools Check | Read-only FFprobe/FFmpeg recording integrity report | **Not implemented** | No markers, messages or recording-file writes; bounded parser/job/tests |
| M3. Media Tools Shrink | Optional space reduction preserving original until separately verified | **Not implemented** | Explicit consent, staged output/verification, rollback and capacity checks; no silent replacement |
| I1. Images and Music | Independent photo and music services via core auth/media APIs | Metadata-only scaffolds | Review separate capability, persistence, playback, source and client contracts |

**A1–A3 are foundations, not proof of a usable add-on manager.** The
non-executing admission preview may report synthetic policy eligibility but
its `executable` field always remains false. A consistent local dpkg ledger
is **not** cryptographic package provenance: dpkg's local database and MD5
records can be changed together. Do not derive `packageTrusted` or effective
capabilities from that diagnostic.

## A2M/A4 installed-metadata binding checkpoint

The Core/Agent install snapshot now requires an existing, root-controlled
`/usr/share/vdr-suite/addons/rectools/addon.json` in addition to the
independent administrator approval and descriptor-pinned fixed handler.
The disabled schema-v1 manifest must match the approved module, package and
version and may declare no entrypoint, capability, permission or dependency.
Missing, replaced, symlinked, writable, malformed or version-mismatched
metadata denies a fresh snapshot. A compiled, reviewed handler protocol
version (`0.1.0`) also rejects unreviewed binary upgrades even when their
manifest and administrator approval agree. For a separately reviewed
same-version binary replacement, the root-only approval issuer now supports
an explicit conditional `rotate`: it checks the old approved SHA-256,
matches the new independently approved digest against the installed binary,
takes a directory lock and atomically publishes the new root-owned record.
A revoked record cannot be rotated back to approved, and reinstalling the
old executable cannot resurrect a revoked approval. This is an isolated
operator lifecycle primitive, **not** a package maintainer hook or automatic
upgrade. Approval and metadata are re-read after binary hashing; the pinned
executor also checks them again immediately before dispatch.
Root-owned, nonroot-Agent and synthetic command lifecycle tests use
disposable fixture installations. This is the same installation contract for
source/manual staging and future Debian packaging, **not** permission or a
release-signature substitute. Production activation, upgrade choreography,
installed-package verification and real-host acceptance remain open.

## A4 read-only package maintenance admission checkpoint

The new `AddonMediaToolsMaintenanceReadService` queries the **existing**
Core `AddonActivationIntentRepository` and `MutationOperationRepository`
for one exact backend. It denies a maintenance snapshot unless an
administrator explicitly disabled the module with a durable revision and
no `media.import` operation remains in `accepted`, `queued`,
`dispatching`, `executed_unverified` or `outcome_unknown`.
Missing schemas and saturated operation listings deny as well; an unknown
outcome is never silently considered resolved. Isolated tests use real
in-memory SQLite and exercise cross-backend scoping and large listings.
This is **diagnostic only**: it is not an exclusive admission/Agent drain
lease, does not stop new work, and cannot authorize package updates,
uninstalls or re-activation. Before any genuine release workflow, Core
must coordinate a durable no-new-admission fence, already executing
Agent commands, authenticated effective rights, all relevant backends,
package provenance, rollback and audited administrator intent.

## A4 durable global Media Import reservation freeze (isolated)

The Core `AddonMediaImportMaintenanceFence` persists a **global** freeze
for the fixed `rectools` Media Import action in the *same* SQLite
authority as ADR-0042 operations. A global administrator changes its state
only through a revision-fenced, audited SQLite write transaction. The
canonical `MutationOperationRepository` now checks the freeze inside
the SAME `BEGIN IMMEDIATE` transaction used for new `media.import`
reservations, across all backends and independent SQLite connections.
New imports cannot slip between a separate preflight read and their
reservation; an existing exact idempotent replay remains readable.
Uninitialized/missing gate schema fails closed for Media Import, while
unrelated timer operations keep their original semantics. An explicit,
audited revision-fenced unfreeze changes only the reservation gate, never
administrator desired-enabled state or executable permissions.

This is **not** an authorized package upgrade, exclusive Agent child drain,
or a live maintenance command. The earlier read-only diagnostic must
still show explicitly disabled activation and zero unresolved operations;
in-flight Agent work, complete multi-backend readiness and trusted package
provenance must be checked separately. No public API or startup activation
is added in this slice.

## A4 Agent child-lifetime drain lock checkpoint

A fixed, root-controlled
`/etc/vdr-suite/addon-approvals/rectools.import-running.lock` inode
complements the global Core SQLite admission freeze and read-only durable
operations inspection. Media Import now refuses to run if this protected
lock inode is missing or unsafe. The pinned Agent executor holds its shared
`flock` throughout approval revalidation and into the handler's `fexecve`
lifetime, including descendants, so an Agent crash cannot silently free the
advisory execution lock while its handler still lives. Disposable CI fixture
tests include an exec'd non-recording child and a blocked exclusive
maintenance acquisition. This change is source-only: the production install
has no automatic lock creation and the default-disabled module remains
unchanged. A future package/source upgrade owner must explicitly stage,
lock and hold the exact inode while it performs a separately approved
transaction, plus freeze new operations, await authoritative job and
Agent reconciliation across backends, validate the package/signature
provenance, and handle rollback safely. None of these later package
actions is activated by this checkpoint.

## A4 combined global maintenance inspection checkpoint

The internal `AddonMediaToolsMaintenanceInspection` now combines an
exclusive, fixed-inode Media Import handler-lock probe with one coherent
SQLite `BEGIN IMMEDIATE` snapshot of the actual Core freeze, all stored
`rectools` desired-state rows, all unresolved `media.import` Operations
(without the earlier 256-result ceiling), Agent command assignment states,
missing persisted Agent results and remaining Media Import capability
advertisements. Missing schemas/unsafe lock/unknown Agent states all
deny. Root fixture CI exercises the genuine activation, operations, audit
and freeze repositories plus **synthetic Agent schema rows** matching the
actual Agent command owner. The separate server-side
`MediaImportOperationFence` also now rejects frozen imports at final
receipt/capability revalidation, not only at new reservation time.

This diagnostic **does not** grant an install token, retain the lock after
returning, prove that every runtime Agent owns the same state file, revoke
current actor grants, validate Debian/PPA release provenance, or perform a
rollback-safe package transaction. A future authorized maintenance owner
must acquire and HOLD exclusive locks, ensure a durable freeze throughout
the update, bind all database/Agent identities, independently validate
signatures, and explicitly manage rollback/unfreeze. This is not a live
host acceptance or package-installation release.

## A4 retained exclusive maintenance diagnostic (source only)

`AddonMediaToolsMaintenanceInspection::inspectAndHold()` now returns a
move-only diagnostic object that retains the same verified **exclusive**
`rectools.import-running.lock` kernel lock after the global Core/Agent
quiescence snapshot returns. Its `holdsExclusiveLock()` proof is false
after move-from, after release, and for a non-exclusive lock mode.
`inspect()` keeps its old point-in-time contract and releases the lock
immediately. Root-controlled CI checks held-lock contention, transfer,
release, denied diagnostics and absence of leaked locks; no package
files or media are changed.

**No maintainer operation is authorized by this result.** Administrator
intent, global SQLite freeze, Agent command evidence and package provenance
can change after the snapshot, even while the handler inode remains locked.
Before an actual Ubuntu `.deb` upgrade/removal a separately designed
package transaction must own the lock for its *whole* lifetime, serialize
freeze ownership and revalidate current durable state and trusted release
bytes under that ownership, then handle rollback safely. Lock inode
replacement, privilege, host runtime and real Debian maintenance hooks
remain unaccepted.

## Recommended execution order

1. Finish A2's **no-dpkg default** and rejection regressions, using only
   isolated test roots. The optional dpkg fixture test is future-package
   compatibility evidence, not a current yaVDR acceptance requirement.
2. Design/implement A2M for the actual manual/source deployment: locate
   independently approved installed handler ownership and immutable binary
   identity. Do not infer trust from a checkout, staged manifest or arbitrary
   administrator-provided executable path.
3. Unify A2M with A4's package pathway under **one** core-owned provider
   identity and handler trust check. Keep daily-build and PPA/deb acceptance
   separate from the manually installed host until packages are installed.
4. Finish A5 before runtime execution: exact grants, audit, durable admin
   intent, revocation/update reconciliation and explicit opt-in.
5. Integrate a single controlled Media Tools import with A6: constrained
   source roots, staging, durable jobs, VDR indexing/readback,
   recording reconciliation and no source deletion.
6. Do not install or enable A7 API/UI in production until separately authorized controlled real yaVDR acceptance; source-level client integration and isolated CI may precede deployment.
7. Add read-only M2 Check, later M3 Shrink, then independent Images/Music.

## Validation, release and safety boundaries

- Offline targets (separate focused groups, not an instruction to rerun all
  of them after each doc change): `check-addon-contract`,
  `check-addon-registry`, `check-addon-deb`,
  `test-addon-media-tools-package`, `test-addon-media-import`,
  `test-addon-access-policy`, `test-addon-activation-intent`,
  `test-addon-admission-preview`; use an isolated workspace and an exact
  candidate. Never inspect the host's dpkg database as part of routine tests.
- Never run prototype `import ... --confirm-writes` against an installed
  VDR recording tree. No add-on installation, service restart, or database
  migration without explicit separate real-host authorization and preflight.
- Every runtime slice must reuse existing Suite-owned actor/session/CSRF,
  backend authorization and Agent fencing, accountability, job state and
  recording-cache reconciliation. A successful local C++ test is not live
  backend acceptance.
- Update [installed-registry contract](../development/addon-installed-registry-contract.md),
  [source packaging](../development/addon-source-packaging.md),
  [Media Tools audit](../development/rectools-cpp-import-audit.md) and the
  affected module's README/AGENTS/manifests at coherent milestones.

## Gate before productive `vdr-suite-addon-media-tools` work

Do **not** equate an inert C++ import preview or a successful
`ProviderBinding` validation with readiness to enable Media Tools.
Implementation may proceed on the existing add-on branch, but the product
integration cutover is gated on concrete reviewed evidence:

1. **Core provider:** manual and Debian paths supply independently verified
   approval, exact binary/version identity, revocation and backend-local
   provenance; no caller-created `RuntimeEvidence.packageTrusted`.
2. **Administrative lifecycle:** canonical global-admin checks, audit and
   idempotent revision-fenced enable/disable, explicit removal/upgrade
   invalidation and durable backend-scoped state.
3. **Operation authorization:** the established grant administration
   supports an exact backend-scoped import permission; generic administrator,
   recorder or device rights cannot substitute.
4. **Backend agent:** one registered command handler with descriptor/path
   confinement, generation/lease fencing, deadline and resource limits,
   cancellation and durable outcome reconciliation.
5. **Public API:** versioned protected endpoints, CSRF, account/device scopes
   and capability discovery derived from effective backend state.
6. **Media acceptance boundary:** no source deletion, no legacy worker,
   verification of created VDR recording/index, capacity preflight,
   authoritative reconciliation, and crash/duplicate tests.

Until all six are satisfied and focused isolated/host-specific checks pass,
keep `AddonHandlerRegistry::kMediaToolsCandidate.executable=false`,
`RuntimeEvidence.packageTrusted=false` in diagnostic adapters, and the
Media Tools manifest without executable capabilities. Use existing Suite
security, operation/job, Agent and frontend owners; do not fork them.

## Media Tools A6 code ownership

New add-on-specific A6 contract and tests live under
`addons/media-tools/` on the **existing** add-on branch.
There is no new branch or repository. The existing standalone C++17
importer under `modules/rectools/` is not duplicated; a future
single tested source/packaging migration must move it rather than fork it.
The add-on request envelope is non-executing and does not replace the
Core-owned command, permission or operation lifecycle. Runtime integration
and API effective capability projection remain open.

## A6 implementation location (same branch, no repo fork)

The existing standalone C++17 media importer was **moved** (Git blobs
retained, old implementation paths removed) from `modules/rectools/`
into `addons/media-tools/include/`, `src/` and `tests/`. No
duplicate implementation exists. The old module directory owns only
the existing disabled metadata manifest and its staging/packaging
documentation. All new Media Tools-specific code is under
`addons/media-tools/`, while Core/Agent continue to own execution,
authorization, backend generations, jobs and recording reconciliation.

The first A6 module-local command boundary consumes canonical
`MutationOperation` and `BackendAgentCommandAssignment` values,
with denied mismatched identity/generation/deadlines and no executor.
It is **not** registered or advertised. Compiled importer/package tests
are included in the isolated Add-on CI after the source relocation.

## A7.5 Web source integration checkpoint

The optional `Media Import` tab belongs to the current `index.html` /
`app.js` production composition. It is hidden without a live, actor/backend-
scoped `addons.media.import` capability and invalidated on backend/session
changes. The module reads only opaque source IDs, submits with browser CSRF
and one unique Idempotency-Key, then follows the existing actor-owned Operation.
It displays `outcome_unknown` without claiming success or submitting a second
job. An in-flight HTTP admission remains owned by the shared backend attempt
across tab re-composition: a late accepted Operation is adopted by the current
view, a definite pre-admission rejection releases the lock, and a lost reply
continues to block duplicate submission. Logout discards this in-memory
association so late responses cannot restore the prior session's Operation.
The source/test/CI owner is `web/frontend/modules/media-import.js`,
`web/frontend/tests/test_media_import_composition.js` and
`make test-ci-frontend`; isolated packaging checks additionally run
`make test-install-staging` and `make test-install-runtime-deployment`.
This milestone does not activate Media Check, Shrink or real-host execution,
and is not a claim of productive deployment.
