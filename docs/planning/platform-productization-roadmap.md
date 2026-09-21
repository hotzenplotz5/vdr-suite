# Platform Productization Roadmap

## Navigation

- [Strict Roadmap](roadmap.md)
- [ADR Index](../adr/index.md)
- [ADR-0013 Permission Model](../adr/ADR-0013-permission-model.md)
- [ADR-0020 Multi-Source Federation Architecture](../adr/ADR-0020-multi-source-federation-architecture.md)
- [ADR-0037 Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)
- [ADR-0060 Federated VDR-Suite Sharing and Reciprocal Site Trust](../adr/ADR-0060-federated-vdr-suite-sharing-reciprocal-site-trust.md)
- [ADR-0061 Actor Permissions, Federation and Client Access](../adr/ADR-0061-actor-permissions-federation-client-access.md)
- [ADR-0062 First-Party Living-Room Output Client](../adr/ADR-0062-first-party-living-room-output-client.md)
- [Current State](../CURRENT.md)

---

## Purpose

This document makes four productization topics explicit without changing their long-standing architecture:

1. **Federated MultiBackend sharing between independent VDR-Suite installations**, including reciprocal but directional trust and granular rights;
2. the concrete sharing permission model for Recordings, Live TV, Timers and destructive/editing operations;
3. the first-party television/living-room output client;
4. release-grade Debian/Ubuntu packaging.

The first two items continue ADR-0013 and ADR-0020. They are not a new interpretation of MultiBackend.

ADR-0013 already states that a remote VDR-Suite instance is an Actor and gives the exact product example that Remote Suite B may see selected recordings from House A while Live TV and Timer creation are denied. ADR-0020 already allows a BackendNode to wrap a remote VDR-Suite instance.

The still-open gap is the production federation layer between **independent Control Planes**. ADR-0041 intentionally did not define that protocol.

These product milestones do **not** silently start Phase 68.

---

## 1. Federated MultiBackend / neighbor-house sharing

### Product model

```text
House A
VDR A + VDR-Suite A
        |
        | explicit Suite-to-Suite pairing/trust
        | directional grants A -> B and B -> A
        |
House B
VDR B + VDR-Suite B
```

Both installations remain autonomous.

House A decides what House B may do on A.
House B independently decides what House A may do on B.

Pairing itself grants nothing.

### Example

```text
House A grants House B:
  Recordings list/view       YES
  Recordings stream          YES
  Live TV                    YES
  Timer create               YES
  Timer modify/delete        NO
  Marks/cutting              YES
  Recording delete/purge     NO

House B grants House A:
  Recordings list/view       YES
  Recordings stream          YES
  Live TV                    NO
  Timer create               NO
  Marks/cutting              NO
```

The product must support asymmetric grants like this.

### Existing foundations reused

- ADR-0013 Actor/Permission architecture;
- ADR-0020 remote VDR-Suite as backend/source;
- Phase-62 actor identity/RBAC/accountability;
- Phase-63 Backend Agent and secure multi-site trust primitives;
- Phase-64 Timer orchestration;
- Phase-65 MediaSession/Gateway;
- ADR-0059 native Recording marks/cutting authority.

Backend Agent multi-site and independent VDR-Suite federation are related but not identical:

```text
Agent model:
one Control Plane owns/orchestrates a remote backend

Federation model:
two Control Planes remain independent and authorize each other as peers/actors
```

### Required product workflow

- create a one-time peer invitation/pairing request;
- approve it on the other VDR-Suite;
- establish revocable authenticated site identity;
- exchange only bounded capabilities/identity needed for federation;
- configure A->B and B->A grants separately;
- optionally approve delegated remote users;
- show remote permitted resources in normal backend-aware views;
- revoke/rotate peer trust without exposing raw VDR/plugin credentials.

### Media

Remote Recording/Live playback goes through the **owner site's** MediaSession/Gateway. A peer never receives permanent Streamdev/SuiteBridge/provider credentials.

### Mutations

Remote Timer, marks/cut, rename/move/delete operations execute through the **owner site's** normal protected mutation path and are authorized there.

### Local backend catalog

A durable local catalog remains useful implementation infrastructure for local backends, Agent-managed remote backends and registered federated peers. It is **not the definition of MultiBackend federation**.

Binding architecture: [ADR-0060](../adr/ADR-0060-federated-vdr-suite-sharing-reciprocal-site-trust.md).

---

## 2. Federation permissions

The permission model is operation- and resource-scoped, continuing ADR-0013.

At minimum the product must distinguish:

- Recording view/list;
- Recording stream/play;
- marks;
- cut;
- rename/move;
- trash/restore/delete/purge;
- Live TV;
- Timer view/create/modify/delete;
- SearchTimer/automation rights where exposed;
- later Legacy OSD view/control separately.

The owner may additionally restrict Recording folders, channels/channel groups and backend scope.

A frontend is not the security authority. Web, television, Kodi/mobile or another client only presents the effective grants; the owning server enforces them.

Pure clients are first-class permissioned consumers. A browser, VDR output/living-room frontend, Android app, TV app, Kodi/mobile client or other API client may authenticate/pair and receive scoped rights **without providing a VDR, BackendNode or reciprocal federation source**. Federation is additional capability, not a prerequisite for access.

Binding architecture: [ADR-0061](../adr/ADR-0061-actor-permissions-federation-client-access.md).


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

Federation + permissioned-client product work
  -> may be implemented as explicit cross-cutting product work
  -> must preserve Phase 62/63/64 authority and fencing
  -> must not silently advance Phase 68/69
```

Any implementation still requires an explicit kickoff and current-main evidence before code changes.
