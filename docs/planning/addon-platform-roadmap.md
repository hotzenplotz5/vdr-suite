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
| A2M. Manual-install registration path | Explicit safe declaration of a source-built, administrator-installed and integrity-pinned backend handler | **Partial:** root-admin issue/revoke, native combined approval/descriptor snapshot, nonroot Agent-file-access CI; no live wiring; disabled handler; no daemon trust/Agent adapter | Fixed reviewed module-to-binary mapping, version and digest, secure owner/mode checks, revocation/uninstall detection; no caller-selected executable |
| A3. Core access/desired-state preview | Canonical actor/backend policy, revision-fenced SQLite intent, read-only eligibility preview | C++ isolated checks 21 + 17 + 20 accepted before TVScraper removal | No execution permit; denied by default and on any lost evidence |
| A4. Trusted runtime provider | Common core authority over either reviewed manual-install or reviewed Debian package provenance, with fixed binary identity and healthy backend-local handler | **Partial:** disabled Core admission, pinned identity/generation, approval comparison and native combined approval/pinned-descriptor checks and unprivileged Agent-path CI; no daemon/Agent adapter | Both paths use the same registry, signed/admin-approved provenance policy and negative trust tests; neither dpkg alone nor source files confer authority |
| A5. Administration and operation rights | Admin enable/disable with audit/idempotency/migrations; scoped media-import grant | SQLite desired-state prototype, audited admin coordinator and admin-only three-module desired-state catalog; exact backend-scoped human import grant and Core-owned dynamic permission descriptors, categorized user-grant settings UI and fail-closed effective-capability projection adapter; runtime API source pending | Proper grant vocabulary, authorization, CSRF, preconditions, revocation and upgrade/removal semantics |
| A6. Protected backend execution | Fenced command/worker, bounded resources and crash-safe durable jobs | **Partial:** non-executing canonical command/generation gate plus real Agent-poll denial of unregistered add-on command namespaces; no command adapter/executor | Agent generation and job fencing, cancellation/retry/readback, no arbitrary paths or shell |
| A7. Public API and client UI | Entitlement-filtered discovery, safe admin surface, addon action/status | **Not implemented** | Versioned v1 routes, account/device tests, frontend empty-state/fallback, safe install staging |
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
6. Only after controlled real yaVDR acceptance expose A7 API/UI capabilities.
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
