# Platform Productization Roadmap

## Navigation

- [Strict Roadmap](roadmap.md)
- [ADR Index](../adr/index.md)
- [ADR-0037 Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)
- [ADR-0060 Backend Catalog and Operator Onboarding](../adr/ADR-0060-backend-catalog-operator-onboarding.md)
- [ADR-0061 Client Identity and Permission Profiles](../adr/ADR-0061-client-identity-permission-profiles.md)
- [ADR-0062 First-Party Living-Room Output Client](../adr/ADR-0062-first-party-living-room-output-client.md)
- [Phase 57 Local Server Permission Model](phase-57-local-server-permission-model.md)
- [Current State](../CURRENT.md)

---

## Purpose

This document turns four previously scattered productization topics into one explicit, reviewable plan:

1. adding and operating multiple VDR backends;
2. deciding what each client/front end may do;
3. delivering a first-party television/living-room output client, including a VDR output-plugin integration path;
4. producing real Debian/Ubuntu packages instead of relying on source-tree installation.

These are cross-cutting product milestones. They do **not** silently start Phase 68 or renumber the strict numbered runtime roadmap.

The implementation order is constrained by existing ownership:

```text
identity/RBAC + BackendRegistry + Agent lifecycle
  -> durable backend catalog and operator onboarding
  -> explicit actor/device/service permission profiles
  -> stable Public API/client contract (Phase 69)
  -> supported first-party living-room rollout
  -> release-grade Debian/Ubuntu packaging
```

Packaging preparation may continue before Phase 69, but the supported release package is gated on the stable client/API contract so packaging does not freeze transitional interfaces accidentally.

---

## 1. MultiBackend productization

### Current foundation

The repository already has backend identity, `BackendRegistry`, backend-scoped snapshots/caches, backend access policy, Backend Agent generation/lease fencing and Phase-64 multi-backend Timer orchestration.

That is **not yet the same thing as a complete operator workflow for adding arbitrary backends**.

The product gap is an operator-owned, durable backend catalog and lifecycle.

### Target operator workflow

```text
Administrator
  -> Add backend
  -> assign stable backendId + display name
  -> configure backend type and connection/bootstrap material
  -> choose enabled/disabled state
  -> choose server-side access mode
  -> enroll/bind technical Agent where required
  -> verify health/capabilities/generation
  -> explicitly choose default/preferred backend where product behavior needs one
  -> grant actor/device/service access separately
```

Required operations:

- list backends;
- create backend;
- edit mutable operator metadata/configuration;
- enable/disable backend;
- rotate/rebind technical Agent identity without changing the logical backend identity;
- retire/remove a backend only under explicit safety checks;
- set or clear an explicit default/preferred backend;
- show effective health, generation, capabilities and access state;
- preserve backend-specific data provenance and history when a backend is temporarily unavailable.

### Hard rules

- No backend becomes writable merely because it is reachable.
- Creating a backend does not auto-grant users or devices access.
- Agent enrollment proves technical identity; it is not end-user authorization.
- A backend ID is durable product identity, not a host name or URL.
- Backend removal must not silently orphan unresolved Timer/media/native operations.
- Multi-backend code must not use `backendRuntimeContexts_.front()` or list order as product policy.
- "First backend" is never a substitute for explicit default/preferred-backend state.
- Cross-backend views preserve backend provenance and partial-failure truth.
- Backend-specific transport secrets remain private to the server/Agent boundary.

### Acceptance

A supported MultiBackend product milestone requires a real two-backend acceptance:

- both backends persist across daemon restart;
- independent health/generation/capability state is visible;
- one backend may be read-write while the other is read-only;
- user/device grants can differ between backends;
- read operations target the intended backend;
- protected writes are blocked on the read-only or ungranted backend;
- Timer assignment can deliberately use eligible backends without changing backend administration semantics;
- disabling one backend does not silently redirect an already-owned operation to another backend;
- explicit default/preferred-backend behavior is deterministic;
- no code path depends on registry iteration order.

Binding architecture: [ADR-0060](../adr/ADR-0060-backend-catalog-operator-onboarding.md).

---

## 2. Client and frontend permission profiles

A frontend is a presentation surface, **not a security principal**. Authorization remains actor/device/service based and is enforced by the server.

### Product profiles

The product should support at least these profiles without inventing a separate permission model for each UI:

| Client family | Identity shape | Typical authority |
| --- | --- | --- |
| Web browser | authenticated user/browser session | only operations granted to that actor for that backend/resource |
| Administrative Web/CLI | authenticated administrator actor | explicit administration grants; never implied by being local |
| Living-room/output client | paired user/device identity | playback/browse/remote functions deliberately granted to that device/user |
| Independent API/Kodi/mobile client | user/device or application identity | stable Phase-69 API grants only |
| Automation/integration | service identity | narrow machine grants, backend/resource scoped |
| Backend Agent | technical Agent identity | backend-local execution/observation only; not end-user authority |
| HbbTV application session | bounded application-session identity | HbbTV session capabilities only; no general Suite administration |

### Rules

- UI visibility may mirror effective access but never replaces server enforcement.
- A living-room client does not get administrator rights because it runs beside VDR.
- A Backend Agent cannot reuse its technical trust as a user session.
- HbbTV application code never inherits browser-user or Agent credentials.
- Independent clients depend on the stable Phase-69 public contract.
- Effective-access reporting should explain why an action is available/blocked without leaking credentials or private policy internals.
- Backend access mode and actor grants are separate inputs; effective permission is their intersection.

Binding architecture: [ADR-0061](../adr/ADR-0061-client-identity-permission-profiles.md).

---

## 3. First-party VDR output / living-room client

### Product goal

Provide a first-party television client that can replace the browser as the primary living-room presentation while preserving VDR-Suite ownership.

The intended user-facing capability includes:

- Live TV;
- Recordings and resume;
- EPG, Home and search;
- playback controls and track selection;
- Teletext and HbbTV integration through their existing Suite domains;
- later Legacy OSD compatibility through Phase 68;
- remote-control/key input mapped to normalized Suite actions;
- hardware-accelerated decode/rendering on supported hardware.

### Architecture

```text
VDR-Suite domains / stable client contract
  -> first-party living-room client
  -> Suite MediaSession / MediaPlaybackContract
  -> platform playback engine
  -> DRM/KMS/Wayland/X11/audio output as selected by the client implementation
```

A VDR output plugin is a supported integration direction, but it must remain a **thin host/integration boundary**. It must not become:

- a second MediaSession owner;
- a second authorization/control plane;
- a direct SQLite client;
- a direct RESTfulAPI/SuiteBridge client for normal product behavior;
- a provider-selector;
- a place where long-running network/transcode work executes under VDR locks.

If implementation evidence shows that decode/render lifecycle is safer in a companion process, the VDR plugin may be paired with that process while the Suite client contract remains unchanged.

### Hardware direction

The architecture is not tied to one GPU vendor. The current yaVDR reference system with Intel Gemini Lake/UHD 605 and VAAPI is a required real-system acceptance target for the first Linux implementation. Legacy VDPAU-only hardware such as GT 210 is compatibility/legacy scope, not the primary design center.

### Sequencing

Development experiments may happen earlier, but a **supported first-party client contract** is gated on Phase 69 so the client is not built against transitional private endpoints.

Binding architecture: [ADR-0062](../adr/ADR-0062-first-party-living-room-output-client.md).

---

## 4. Debian/Ubuntu release packaging

ADR-0037 already established staged install layout and the rule that internal C++ modules are not a promised public ABI. The remaining work is real distribution packaging.

### Gate

Release-grade Debian/Ubuntu packaging starts after Phase 69 Public API and Client Compatibility Hardening.

Reason: packaging should freeze supported service/client/install contracts, not transitional internals. This does not prevent maintaining `make install DESTDIR=...` readiness before Phase 69.

### Required package work

The packaging milestone must add and validate:

- canonical `debian/` packaging metadata;
- `dpkg-buildpackage`/equivalent reproducible package build;
- Build-Depends and runtime Depends/Recommends/Suggests;
- architecture declarations and supported Debian/Ubuntu baseline;
- systemd units and enable/start policy;
- tmpfiles/runtime-directory ownership where needed;
- conffile policy under `/etc/vdr-suite`;
- state/cache/log ownership under FHS-compatible paths;
- database schema migration behavior during upgrades;
- upgrade from at least one previous supported package version;
- rollback/reinstall recovery strategy;
- remove versus purge semantics;
- package ownership of SuiteBridge, Backend Agent, Web assets and optional living-room client/plugin artifacts;
- dependency handling for FFmpeg, VDR plugin ABI/package versions and optional hardware acceleration;
- clean-install acceptance on a fresh Debian/Ubuntu host;
- package-upgrade acceptance on the real yaVDR target;
- staged install parity: package contents must come from the same supported install contract rather than a parallel hand-maintained file list;
- lint/build-policy checks appropriate to the target distributions;
- source-package and binary-package artifact documentation.

### Package-boundary principles

Exact binary package names remain a packaging decision until the manifest is implemented, but the conceptual split is:

```text
server/runtime
web assets
CLI/admin tooling
Backend Agent
VDR-local SuiteBridge integration
optional first-party living-room/output integration
documentation
optional test/debug tooling (not installed by default)
```

No `-dev` package or public C++ ABI is implied unless a future ADR explicitly creates one.

### Acceptance

The release package is not accepted until:

1. a fresh install produces a bootable, enabled/disabled-as-documented service state;
2. configuration survives package upgrades correctly;
3. database migration succeeds without losing recordings/timers/metadata policy state;
4. uninstall leaves user data according to documented remove semantics;
5. purge removes only package-owned configuration/state according to documented policy;
6. package install does not overwrite locally managed secrets;
7. SuiteBridge/plugin ABI compatibility failure is detected rather than silently starting an incompatible runtime;
8. the packaged result passes the same real yaVDR Golden paths used by source installs.

Binding install boundary: [ADR-0037](../adr/ADR-0037-packaging-install-api-boundary.md).

---

## Execution relationship to numbered phases

These milestones are binding productization work but are intentionally not inserted as surprise numbered runtime phases.

```text
Phase 68 Legacy OSD
  -> Phase 69 Public API hardening
       -> stable independent-client boundary
       -> supported living-room client rollout
       -> release-grade Debian/Ubuntu packaging

Backend catalog/onboarding + permission-profile work
  -> may be implemented as explicit cross-cutting product work
  -> must preserve Phase 62/63/64 authority and fencing
  -> must not silently advance Phase 68/69
```

Any implementation still requires an explicit kickoff and current-main evidence before code changes.
