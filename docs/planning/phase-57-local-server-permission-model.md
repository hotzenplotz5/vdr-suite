# Phase 57 Local Server Permission Model

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)

---

## Purpose

This document fixes the first Phase 57 implementation boundary before code changes.

Phase 57 starts with a local or private VDR-Suite server architecture. The word local means the server is reachable inside a local network or private network. It does not mean that the implementation is only a local repository workflow.

The goal is to make one central VDR-Suite server manage multiple VDR backends and enforce backend-specific access modes.

---

## Scope

In scope for Phase 57.1:

- one central VDR-Suite server in a local or private network
- multiple VDR backends reachable by that server
- backend identity
- backend display name and optional location metadata
- backend access mode
- read-only secondary backend mode
- read-write primary backend mode
- server-side write blocking for read-only backends
- frontend-ready API state for backend selection and permissions

Out of scope for Phase 57.1:

- public Internet access
- reverse proxy deployment
- HTTPS deployment design
- VPN or tunnel deployment
- OIDC, LDAP, passkeys or external identity provider integration
- full Web UI implementation
- streaming or recstream implementation

---

## Target Local Server Topology

```text
Browser, CLI or later Web frontend in LAN
  -> central VDR-Suite server
     -> VDR backend: living-room
     -> VDR backend: remote-house
```

The central VDR-Suite server runs the daemon, REST API, backend registry and policy enforcement.

The VDR backends stay normal VDR systems with the required backend interfaces such as RESTfulAPI, SVDRP or future adapter sources.

Clients must not rely on talking directly to the VDR backend interfaces.

---

## Local Server Responsibility

The central VDR-Suite server is responsible for:

- knowing configured backends
- resolving stable backend identifiers
- exposing backend selection state to API consumers
- exposing backend access state to API consumers
- enforcing read-only and read-write policy before dispatching write operations
- keeping backend-specific transport details behind adapter boundaries

VDR backends are not responsible for VDR-Suite user or backend policy.

---

## Existing Architecture Foundations

The existing code already contains relevant pieces that Phase 57.1 must reuse before introducing new types.

Current foundations:

- `BackendNode` already models backend identity, type, connection, capabilities, enabled state and online state.
- `BackendRegistry` already supports adding, looking up, listing and updating backend capabilities.
- `BackendRegistryService` already wraps registry access and exposes the default backend.
- `BackendRegistryJsonSerializer` and `BackendRegistryController` already expose backend lists as JSON.
- `VdrCapabilitySet` already exists and contains a snapshot read-only preset.
- Recording actions already have a backend policy model with read-only and mutation policies.
- Recording action execution already has a policy-aware path that blocks execution through safety policy.
- SearchTimer workflow already has backend write allowlist and backend write permission gates.
- Timer actions already carry a backend ID and dispatch through an adapter registry, but need central read-only policy enforcement.

---

## Access Mode Model

Phase 57.1 should introduce the smallest local server access model needed for backend policy.

Required access states:

```text
read-only
read-write
```

Possible later extension:

```text
admin
unavailable
maintenance
```

The first cut must not require a full user database.

The access mode belongs to the local VDR-Suite server's view of a backend.

Example:

```text
living-room:
  access: read-write

remote-house:
  access: read-only
```

---

## Frontend-Ready API Shape

The API should expose access state so a later frontend can show correct controls without guessing.

Example target shape:

```json
{
  "backends": [
    {
      "backendId": "living-room",
      "backendName": "Living Room VDR",
      "backendType": "vdr",
      "accessMode": "read-write",
      "readOnly": false,
      "enabled": true,
      "online": true
    },
    {
      "backendId": "remote-house",
      "backendName": "Remote House VDR",
      "backendType": "vdr",
      "accessMode": "read-only",
      "readOnly": true,
      "enabled": true,
      "online": true
    }
  ]
}
```

The frontend may hide or disable write actions based on this state.

The server must still enforce the policy when the API is called directly.

---

## Server-Side Enforcement Rule

Frontend permission display is not a security boundary.

VDR-Suite must block write operations on a read-only backend before dispatching to backend adapters.

Minimum write surfaces to guard:

- recording actions that mutate recordings
- timer create, update, delete or toggle operations
- SearchTimer write execution
- any future destructive or mutating backend action

Read operations remain allowed when the backend is enabled and readable.

---

## Phase 57.1 Implementation Direction

Recommended sequence:

1. Extend backend identity with local access metadata.
2. Extend backend JSON serialization and controller tests.
3. Add or reuse a central backend access policy helper.
4. Wire read-only policy into recording actions only if existing recording policy cannot already derive it from registry state.
5. Add central read-only policy enforcement for timer actions.
6. Connect SearchTimer write gates to the central backend access state without weakening the existing explicit allowlist and permission gates.
7. Keep all backend-specific implementation behind existing adapter boundaries.

---

## First Code Target

Phase 57.1 first code target:

```text
Backend Identity + Local Server Permission Model
```

Expected concrete output:

- `BackendNode` carries local access state.
- backend registry JSON exposes access mode and read-only state.
- controller tests prove the API exposes read-only and read-write backends.
- focused backend registry tests stay green.
- no public Internet or Web UI scope is added.

---

## Validation

For documentation-only changes:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

For Phase 57.1 code changes, also run the focused backend tests selected by the implementation diff, at minimum:

```bash
make test-vdr
make test-docs
make test-phase
make test-phase-map-coverage
```

Additional recording or timer tests must be added when policy enforcement touches those areas.

---

## Non-Goals

Phase 57.1 does not define how users reach the system from the Internet.

Remote access belongs to a later secure deployment phase after local backend permissions are working.

Phase 57.1 also does not implement the Web frontend. It only prepares frontend-visible API state.

---

## Back

- [Back to Planning Index](index.md)
- [Back to Roadmap](roadmap.md)
- [Back to Phase Map](phase-map.md)
- [Back to Current State](../CURRENT.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
