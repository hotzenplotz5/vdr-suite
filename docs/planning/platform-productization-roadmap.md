# Platform Productization Roadmap

## Navigation

- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Security and Identity Foundation](../architecture/security-identity-foundation.md)
- [ADR Index](../adr/index.md)
- [ADR-0013 Permission Model](../adr/ADR-0013-permission-model.md)
- [ADR-0020 Multi-Source Federation Architecture](../adr/ADR-0020-multi-source-federation-architecture.md)
- [ADR-0037 Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)
- [ADR-0041 Authentication, Agent Trust and Multi-Site Transport](../adr/ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049 Audit and Security Event Model](../adr/ADR-0049-audit-security-event-model.md)
- [ADR-0060 Federated VDR-Suite Sharing and Reciprocal Site Trust](../adr/ADR-0060-federated-vdr-suite-sharing-reciprocal-site-trust.md)
- [ADR-0061 Actor Permissions, Federation and Client Access](../adr/ADR-0061-actor-permissions-federation-client-access.md)
- [ADR-0062 First-Party Living-Room Output Client](../adr/ADR-0062-first-party-living-room-output-client.md)
- [Current State](../CURRENT.md)

---

## Purpose

This document is the binding cross-cutting productization plan after Phase 69.

Phase 69 completed the stable independent-client boundary. Productization now turns that boundary into a modern, client-independent media platform without reopening completed phases or inventing a second roadmap.

VDR-Suite remains VDR-centered, but it is not defined as a modern VDR Web frontend. The target product is a Suite-owned platform whose browser, TV, Android, Kodi, desktop and later clients consume the same stable public contract.

Plex and Jellyfin are useful product/reife comparisons for breadth, administration, multi-user behavior and client quality. They are not architecture templates and their internal ownership or protocol choices are not copied by default.

## Platform product direction

~~~text
TV / Android / Kodi / Desktop / Web / later clients
                         |
                         v
                    stable /api/v1
                         |
                         v
                   Control Plane
                         |
       +-----------------+------------------+
       |                 |                  |
Identity / Users /   Library / Metadata   MediaSession /
Profiles / Grants    / History            Streaming /
                                          Transcoding
                         |
                         v
                Backend orchestration
                         |
                         v
                Backend Agents / VDR
~~~

Binding rules:

- the existing Web frontend remains a full browser client, reference client and important administration surface;
- it must remain stable, fast and functional, but productization does not require a framework rewrite before platform work;
- no first-party or third-party client may establish private VDR, SuiteBridge, RESTfulAPI, Agent or provider shapes as public contracts;
- a missing general client capability is evaluated first as a platform/API capability, not solved as a proprietary client bypass;
- platform-appropriate mature player engines are preferred over a universal Suite-owned decoder/player core;
- provider, Agent and VDR-local credentials or URLs never become permanent client credentials or public media contracts.

## Existing identity foundation — reuse, do not reinvent

Phase 62 already provides a real persistent security foundation:

- Actor identity and actor type;
- Device identity;
- Session identity;
- Credential identity and verifier storage;
- browser-session issuance, revocation, expiry and CSRF;
- exact actor permission grants and backend scope;
- fixed role expansion;
- server-side authorization;
- accountability evidence.

The production repositories already persist security actors, devices, sessions, credentials, credential verifiers, browser-session credentials and actor permission grants. Managed Basic and browser sessions resolve into the same persistent security context. Legacy Basic remains an explicitly transitional compatibility mode.

This is the security-principal foundation for productization. It is not yet the complete household/user/profile product model.

### Identity concepts that must remain distinct

~~~text
Actor
  security principal used for authentication, authorization and accountability

Human User / Account
  human-owned account and administrative lifecycle

Profile
  household/media persona with personal media and UI state

Credential
  secret/verifier/certificate binding used to authenticate an identity

Session
  bounded authenticated runtime context

Device / Client
  revocable client/device identity and policy context

Permission / Grant
  allowed operation over an explicit resource scope

Backend / Library / Content Scope
  resources to which a grant applies

Capability
  technical ability of a backend or client; never authorization
~~~

An Actor is not synonymous with a Human User or Profile. Actors may also represent API clients, services, remote VDR-Suite peers or other technical identities.

No second identity authority may be created beside the Phase-62 repositories and authorization path. Productization must extend or explicitly map the existing authority.

## Binding productization sequence

These steps are cross-cutting product work. They do not renumber the strict numbered phase sequence.

### P0 — Platform Direction / Documentation Alignment

Status: **this documentation slice**.

- reflect Phase-69 completion in current planning authorities;
- make backend-first/client-platform direction explicit;
- keep Plex/Jellyfin as product/reife comparisons only;
- define Web as browser/admin/reference client;
- make stable /api/v1 the supported independent-client boundary.

### P1 — Identity Model Audit

Before product code, inventory live main and prove the gap between the existing security Actor model and a human household/multiuser model.

Inventory at minimum:

- Actor and actor types;
- Human User/Account representation, if any;
- Credential;
- Device;
- Session;
- Role;
- Grant;
- Backend scope;
- persistence ownership;
- lifecycle, revocation and recovery rules;
- later changes made after Phase 62.

The first implementation slice must be chosen only after this audit. It must not create a parallel identity store or treat Actor as a synonym for User.

### P2 — Account Administration Read Model

Expose stable administrative read resources for the existing authority first, when that is the smallest coherent boundary:

- human accounts and supported actor identities;
- roles;
- grants;
- backend access;
- credential metadata without secrets;
- sessions;
- devices.

Read-only is preferred for the first slice if mutation lifecycle, revision or bootstrap decisions are not yet complete.

### P3 — Account / Grant Administration

Productize the existing Roadmap milestone **Account and Backend Access Administration**:

- create, update, disable and recover supported human accounts;
- grant/revoke roles and permissions;
- manage backend scopes;
- rotate/revoke supported credentials;
- use revision-safe and fully accountable mutations;
- never return stored password material or reusable verifier secrets.

This step also owns the product bootstrap/recovery contract described below. Generic user administration belongs to the persistent Suite identity store, not to /etc/vdr-suite.

### P4 — Household / Profile Model

Add a media persona layer separate from security Actors.

Example profiles may include Holger, Partner, Kind and Gast. Managed/child profiles may be supported, but exact parental policy is a later bounded decision.

A Profile may be selected after authentication without silently becoming an independent authentication authority.

### P5 — Per-Profile Media State

Move personal media state behind Profile identity before personalized recommendations.

At minimum:

- Continue Watching;
- playback position;
- Recently Watched / History.

Later extensions may include Favorites, preferences and recommendation input.

Required truth:

~~~text
Profile Holger:
  Recording A = 38 minutes

Profile B:
  Recording A = 4 minutes
~~~

Global shared playback/history state is not sufficient for personalized Phase-70 recommendation work.

**Phase 70 personalized recommendation implementation is blocked until P5 has a stable profile-scoped media-state contract.**

### P6 — Device & Session Management

Make client/device state visible and revocable for devices such as:

- living-room TV;
- VIDAA TV;
- Android TV;
- Kodi;
- smartphone;
- browser;
- desktop client.

Device trust, session state, User/Profile selection and permissions remain distinct.

Device capability may constrain media delivery but never grants authorization.

### P7 — Scoped Content Access

Extend existing backend scope only after the core account/profile model is stable.

Potential scopes include:

- Library scope;
- Recording folder/collection scope;
- Channel group scope;
- other explicit content scopes proven by a real product requirement.

Example child profile policy may allow a children library and streaming while denying Recording deletion and Timer mutation.

### P8 — Remote Access Contract

Remote access uses the same platform boundary as local access:

- authenticated identity;
- Device trust;
- User/Profile context;
- permission and content policy;
- MediaSession;
- short-lived MediaAccessGrant.

Remote access must not be implemented as a permanent Streamdev/VDR/provider URL, exposed private plugin port or provider-specific client hack.

Independent VDR-Suite federation remains the separate reciprocal-site model from ADR-0060/0061 and reuses the same owner-side authorization principles.

### P9 — Client Contract / SDK Layer

Publish and maintain stable /api/v1 models and client semantics for:

- TV;
- Android;
- Kodi;
- desktop;
- Web;
- later supported clients.

SDK/helpers may normalize public API usage, errors, pagination, revisions, sessions and MediaSession semantics. They must not encode private backend knowledge.

### P10 — First-party Client Rollout

Roll out high-quality clients only on the stable platform boundary.

Kodi target:

~~~text
Kodi UI / Kodi Player
        |
        v
VDR-Suite Add-on
        |
        v
      /api/v1
        |
        v
   MediaSession
~~~

TV, VIDAA, Android and desktop clients follow the same ownership rule. Platform-native or mature player engines remain behind Suite playback semantics.

## Admin bootstrap and installation direction

The current Legacy Basic and Managed Basic mechanisms are compatibility/foundation mechanisms, not the final human-account product workflow.

Target direction for fresh production installs:

- no default administrator credential;
- no permanent product default such as admin / vdr-suite;
- /etc/vdr-suite contains system/deployment configuration such as Security Mode, listen address, TLS, database path, session policy and external identity-provider configuration;
- normal human accounts, credentials, grants, devices and sessions live in the persistent Suite identity store;
- password-based local accounts store only modern salted one-way verifiers, never plaintext or reversible passwords.

First administrator bootstrap target:

~~~text
apt install vdr-suite
        |
        v
unclaimed server
        |
        v
root/operator starts one-time bootstrap
        |
        v
short-lived setup code
        |
        v
Web setup
        |
        v
first administrator account
        |
        v
bootstrap credential permanently invalidated
~~~

Source installation and Debian/Ubuntu packages must use the same Suite identity/bootstrap lifecycle. Packaging may invoke or document the bootstrap but must not create a package-specific user database.

Local root/operator recovery should create a short-lived, auditable recovery flow. Manual SQLite editing is not a product recovery workflow.

The exact bootstrap credential format, expiry, endpoint and recovery mutation contract require their own bounded implementation decision in P3; this roadmap defines ownership and safety direction, not a hidden second authentication protocol.

## TV / app pairing direction

First-party living-room and device clients should support a bounded approval flow:

~~~text
TV app first start
   |
   v
server discovery / selection
   |
   v
short-lived pairing request
   |
   v
QR code + human code
   |
   v
trusted browser / phone approval
   |
   v
server registers device
   |
   v
TV receives revocable device credential
~~~

Rules:

- the QR code does not contain a permanent credential;
- pairing never grants administrator rights by itself;
- Device Trust, User Identity, Profile and Permission are separate;
- a device may narrow effective policy but never broaden the User/Profile grants;
- sensitive destructive/admin actions may later require PIN or re-authentication.

Conceptually:

~~~text
effective authorization
  = user/actor permission
  INTERSECT device policy
  INTERSECT backend/content policy
  INTERSECT capability availability
~~~

Capability here means technical ability such as H.264, HEVC, AV1, AC3, resolution or HDR. Capability is never permission.

A household TV may present a post-authentication profile picker such as:

~~~text
Who is watching?

[ Holger ] [ Partner ] [ Kind ]
~~~

Personal devices may bind directly to an allowed default User/Profile when policy permits.

The exact device-pairing credential schema and approval lifecycle must be finalized in P6 against the existing Phase-62 Device/Credential/Session authority.

## Federation / neighbor-house sharing

The existing federation direction remains valid and separate from ordinary remote client access.

### Product model

~~~text
House A
VDR A + VDR-Suite A
        |
        | explicit Suite-to-Suite pairing/trust
        | directional grants A -> B and B -> A
        |
House B
VDR B + VDR-Suite B
~~~

Both installations remain autonomous. Pairing itself grants nothing.

Required workflow:

- create a one-time peer invitation/pairing request;
- approve it on the other VDR-Suite;
- establish revocable authenticated site identity;
- configure A->B and B->A grants separately;
- optionally approve delegated remote users;
- expose only owner-authorized Suite resources;
- revoke/rotate peer trust without exposing raw VDR/plugin credentials.

Remote Recording/Live playback goes through the owner site's MediaSession/Gateway. Protected mutations execute through the owner site's normal authorization, revision, idempotency, accountability and reconciliation paths.

Binding architecture: [ADR-0060](../adr/ADR-0060-federated-vdr-suite-sharing-reciprocal-site-trust.md) and [ADR-0061](../adr/ADR-0061-actor-permissions-federation-client-access.md).

## First-party living-room client

The browser remains a supported first-party client, but it is not the only product presentation target.

A VDR output plugin may be used as a thin integration/hosting boundary, never as a second Control Plane, media authority or private-provider client.

The first Linux/yaVDR implementation continues to target mature hardware-accelerated playback on the supported Intel reference system, with platform-specific engines hidden behind MediaPlaybackContract semantics.

Binding architecture: [ADR-0062](../adr/ADR-0062-first-party-living-room-output-client.md).

## Debian/Ubuntu release packaging

ADR-0037 remains the install-layout authority. Phase 69 has now satisfied the public-contract gate that previously blocked release-grade packaging.

Required package work includes:

- canonical Debian packaging metadata;
- reproducible package build;
- dependencies and architecture declarations;
- systemd lifecycle;
- conffile policy under /etc/vdr-suite;
- state/cache/log ownership;
- schema migration and upgrade behavior;
- remove versus purge semantics;
- SuiteBridge/Agent/Web/client artifact ownership;
- fresh-install and upgrade acceptance on supported Debian/Ubuntu/yaVDR targets.

Package boundaries must not introduce a second account database or secret store. The package participates in the same bootstrap/recovery and persistent identity lifecycle described above.

Binding install boundary: [ADR-0037](../adr/ADR-0037-packaging-install-api-boundary.md).

## Execution relationship to numbered phases

~~~text
Phase 69 Public API hardening [COMPLETED]
  -> P0 documentation/platform direction
  -> P1 Identity Model Audit
  -> P2 Account Administration Read Model
  -> P3 Account / Grant Administration
  -> P4 Household / Profile Model
  -> P5 Per-Profile Media State
  -> P6 Device & Session Management
  -> P7 Scoped Content Access
  -> P8 Remote Access Contract
  -> P9 Client Contract / SDK Layer
  -> P10 First-party Client Rollout

Phase 70 Recommendation / Content Knowledge Graph
  -> remains the next strict numbered phase
  -> is not started by this roadmap
  -> personalized recommendation work requires P5 first
~~~

Adjacent cross-cutting milestones remain valid where they do not conflict with this sequence: Broad Timer Product UI, audit/security/operations surfaces, Legacy Basic retirement, federation and release packaging.

Every implementation slice still requires a live-main audit and its own bounded authorization. Historical Phase documents remain historical evidence and are not rewritten by this plan.
