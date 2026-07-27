# ADR-0051: Client Platform and Native Android Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Documentation](../architecture/index.md)
- [ADR Index](index.md)
- [Android, Android TV and Client API Feasibility Study](../architecture/android-client-api-feasibility-study.md)
- [Client Capability, API Candidate and Gap Matrix](../planning/client-capability-api-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Status

Proposed

Date: 2026-07-27

---

## Context

VDR-Suite currently provides a browser frontend that owns desktop, mobile-browser and TV-browser presentation. It consumes Suite HTTP routes through a DOM-free JavaScript wrapper, while individual frontend modules still contain browser-specific state, navigation, rendering and compatibility behavior.

The current browser runtime provides structured VDR state, including channel and programme information, EPG data, recordings, timers, search, Genres, remote-control actions and LiveOverlay state. It does not provide a Suite-owned video player, MediaSession contract or Streaming Gateway. The existing LiveOverlay and SSE paths are control/read-model facilities, not a media transport.

Future clients may include:

- Android smartphones and tablets;
- Android TV and Google TV devices;
- the existing browser frontend and TV browsers;
- a progressively enhanced PWA;
- possible iOS and desktop clients;
- third-party and automation clients.

A durable architecture decision is required before prototypes accidentally turn current unversioned browser routes, JavaScript behavior, provider URLs or UI details into permanent public contracts.

The detailed evidence, route tracing, weighted option comparison and PoC boundaries are documented in the linked feasibility study and client/API gap matrix.

---

## Decision

### Production Android direction

The long-term production client for Android smartphones and tablets will be a native Android application built with:

- Kotlin;
- Jetpack Compose for application UI;
- Kotlin Coroutines and Flow for asynchronous state;
- AndroidX lifecycle-aware state collection;
- Android Media3 for playback and media-session integration once the Suite Media Plane exists.

The architecture must follow a unidirectional state flow with explicit domain models, repositories, use cases where justified and lifecycle-aware presentation state. Android framework types must not leak into the shared domain or protocol model unless the responsibility is inherently platform-specific.

This decision selects an architecture direction. It does not freeze a particular library version in the ADR.

### Android Mobile and Android TV

Android Mobile and Android TV will share a non-UI client core where the domain and protocol semantics are genuinely identical.

Expected shared areas include:

- canonical client models;
- public API transport and serialization;
- authentication and device identity;
- capability discovery;
- cache and synchronization policies;
- operation, error and revision models;
- media-session abstractions;
- test fixtures and contract tests;
- design tokens that are independent of input modality.

Touch and ten-foot/D-pad interfaces will remain separate presentation surfaces.

Android TV must not be implemented as a stretched mobile layout. It requires explicit focus movement, D-pad navigation, back-stack behavior, remote-friendly controls, overscan-safe composition, readable distance typography and TV-specific playback controls.

### Browser and PWA strategy

The existing browser frontend remains a first-class client.

Progressive Web App capabilities may be added as a parallel browser enhancement where they improve installation, launch, caching, connectivity handling and device integration without weakening server-side security or API boundaries.

PWA support is complementary to the native Android product. It is not the sole Android strategy and does not remove the need for native Android TV interaction and media integration.

### WebView and Trusted Web Activity

A WebView or Trusted Web Activity shell may be used only as a clearly temporary transition, demonstration or constrained PoC approach.

Such a shell should preferably be read-only. It must not establish a permanent privileged JavaScript bridge, bypass browser security controls, expose bearer credentials to arbitrary page content or make internal frontend routes into a public API by accident.

WebView/TWA is not the production target architecture for the full Android or Android TV application.

### Hybrid application frameworks

A hybrid framework remains technically possible, but it is not the preferred production direction.

Adopting one later requires a new decision based on demonstrated benefits that outweigh additional bridge, media, lifecycle, focus, security and long-term maintenance complexity. The current decision must not be interpreted as approval for an implicit hybrid rewrite of the browser frontend.

### Client contract layers

The following are distinct contracts and must remain distinct:

1. **Public HTTP API** — versioned, authenticated and documented server contract for independent clients.
2. **Browser JavaScript wrapper** — convenience and compatibility layer used by the current web frontend.
3. **Android client library** — typed Kotlin transport/model layer built on the public HTTP API.
4. **Agent protocol** — trusted Control Plane to backend-Agent communication.
5. **Media Plane** — playback session, authorization and stream-delivery contract.
6. **Plugin and provider schemas** — internal or provider-facing integration contracts.
7. **UI state** — client-local navigation, focus, dialog, scroll, pressed and rendering state.

No layer becomes another merely because route names or data fields currently overlap.

### Public API versioning

Current unversioned `/api/...` routes are transition contracts used by the existing Suite runtime. They are not automatically declared stable public API.

Independent production clients must ultimately consume the versioned public API defined under the Phase 67 compatibility contract, expected to use `/api/v1` semantics in accordance with ADR-0048.

Until that contract exists, exploratory clients must isolate current route strings and response quirks behind a compatibility adapter. Application screens and domain logic must not depend directly on those transition details.

### Authentication and mutation safety

A production client must not treat current network reachability as authentication.

Production authentication, authorization, device identity, account/session lifecycle, revocation and permission enforcement depend on the Phase 62 security model.

All mutations must use the Suite mutation contract, including as applicable:

- explicit authorization scopes;
- revision or precondition checks;
- idempotency keys;
- stable operation identifiers;
- conflict and retry semantics;
- auditable actor and device identity;
- clear pending, accepted, completed, failed and indeterminate states.

Client-side button disabling is user-interface behavior, not the mutation-safety contract.

### Media architecture

Structured LiveOverlay data is not video playback.

Production playback must use the Suite Media Plane defined by ADR-0046 and delivered through Phase 65. The product contract must provide authorized media sessions, stable media metadata, lifecycle and expiry rules, supported stream formats, failure semantics and a Streaming Gateway boundary.

Clients must not permanently construct or expose direct provider or Streamdev URLs as their public playback contract.

Media3 is the selected Android playback integration layer once Phase 65 supplies the server-side media-session contract. It does not replace that contract.

### Read-only proof of concept

A read-only Android proof of concept is allowed before Phases 62, 65 and 67 when it observes all of these constraints:

- no timer, recording, remote-control or configuration mutations;
- no production credential promise;
- current routes are isolated behind a disposable compatibility adapter;
- no claim that the adapter is the stable public SDK;
- no direct provider URL becomes product architecture;
- test media may be used to validate Media3 independently of the Suite Media Plane;
- screens clearly distinguish repository/runtime truth from mocked or unavailable capabilities.

The PoC may validate navigation, state flow, adaptive mobile layouts, Android TV focus behavior, serialization and read-model usability. It must not be presented as completed production integration.

### Roadmap position

This decision creates no new numbered runtime phase and does not reorder the strict roadmap.

The relevant gates remain:

- Phase 62: authentication, authorization, permissions and production mutation safety;
- Phase 63: remote sites and secure Agent/control-plane operation;
- Phase 64: timer automation and orchestration capabilities;
- Phase 65: Streaming Gateway and Media Sessions;
- Phase 66: legacy OSD compatibility where still required;
- Phase 67: stable public API, compatibility policy and independent-client/SDK contract.

Phase 62 remains the next strict runtime phase.

---

## Consequences

Positive:

- Android receives a platform-native product direction rather than an indefinite browser shell.
- Android Mobile and Android TV can share protocol and domain work without forcing a common interaction model.
- The browser remains supported and can improve independently through PWA capabilities.
- Current unversioned routes remain usable for the existing frontend without being prematurely frozen as public API.
- Authentication, mutation safety, media delivery and public API stability retain their existing roadmap owners.
- Media playback is separated from LiveOverlay and change-notification state.
- Third-party, desktop and future iOS clients can target the same public contract without depending on Android implementation details.
- A constrained PoC can start early without misrepresenting product readiness.

Trade-offs:

- Native Mobile and TV presentation surfaces require more UI work than one responsive browser view.
- Shared-core boundaries require discipline to prevent Android or browser concerns from leaking into protocol/domain code.
- Productive mutations and playback cannot be completed solely inside an app repository before their server phases exist.
- Transition adapters may need replacement when `/api/v1` is finalized.
- Browser, Android and future clients need contract tests against the same public API semantics.

Required follow-up:

- keep the feasibility study and API gap matrix synchronized with material architecture changes;
- define Phase 62 authentication and device/session flows suitable for native and browser clients;
- define Phase 65 MediaSession and Streaming Gateway contracts before production playback work;
- define Phase 67 schemas, errors, compatibility policy and SDK generation/maintenance rules;
- convert this ADR from Proposed to Accepted only after architecture review approves the decision.

---

## Rejected Alternatives

### WebView as the permanent Android application

Rejected because it preserves browser constraints, adds WebView security and lifecycle concerns, provides weak Android TV focus/media integration and risks exposing internal frontend behavior as a client contract.

### PWA as the only Android and Android TV product

Rejected because PWA support is valuable for browser clients but does not provide the preferred long-term TV input, media-session, platform lifecycle and native integration model.

### One shared responsive UI for Mobile and TV

Rejected because touch and D-pad/focus interaction are different product surfaces. Shared domain logic does not imply shared screen composition or navigation.

### Freeze current unversioned routes as the public API

Rejected because current routes include transition behavior, frontend-oriented compatibility semantics and incomplete authentication, mutation, media and compatibility guarantees.

### Use direct Streamdev or provider URLs as the playback API

Rejected because provider URLs are not a stable, authorized Suite Media Plane and would leak backend topology and provider-specific behavior into every client.

### Put Agent protocol or UI state into the public API

Rejected because trusted backend control communication and local presentation state have different security, lifecycle and compatibility responsibilities from independent-client APIs.

### Delay all client work until Phase 67

Rejected because read-only prototyping can validate models, navigation, adaptive layouts and TV focus behavior early when it remains explicitly disposable at the transport boundary and does not claim production readiness.

---

## Related Decisions

- [ADR-0003: REST API as External Interface](ADR-0003-rest-api.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0019: SSE Event Stream Transport Strategy](ADR-0019-sse-event-stream-transport-strategy.md)
- [ADR-0030: Domain-First UI Over OSD Proxy](ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](ADR-0047-legacy-osd-compatibility-bridge.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049: Audit and Security Event Model](ADR-0049-audit-security-event-model.md)

---

## Supporting Documents

- [Android, Android TV and Client API Feasibility Study](../architecture/android-client-api-feasibility-study.md)
- [Client Capability, API Candidate and Gap Matrix](../planning/client-capability-api-gap-matrix.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Architecture Documentation](../architecture/index.md)
- [Back to Documentation Index](../index.md)
