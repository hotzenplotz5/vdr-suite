# Architecture Source Audit - 2026-07-15

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current State](../CURRENT.md)
- [Roadmap](../planning/roadmap.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [ADR Index](../adr/index.md)

---

## Purpose

This document records the completed source-based architecture audit that was used to define the post-Phase-60 VDR-Suite architecture direction.

It is an evidence snapshot, not a live roadmap and not a completed implementation claim.

The living list of remaining architecture and feature gaps is maintained in [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md).

---

## Audit Status

```text
Status: Completed
Audit date: 2026-07-15
Repository baseline: VDR-Suite through Phase 60.14k
Primary output: ADR-0038 through ADR-0041 and the strict Phase 60.15-68 roadmap
```

Broad ecosystem auditing is complete for architecture planning purposes. Future source audits should answer a concrete adapter, feature, migration or security question.

---

## Audited Upstream Baselines

| Component | Audited revision | Declared version or role |
| --- | --- | --- |
| VDR Core | `559743ce4603b8660bc0ad0fe77d1fb6145c7a13` | VDR 2.8.2 |
| epgsearch | `0118b4129c3601c04c7483f2fd04b604d4a2a980` | 2.4.6 |
| Live | `e582514ede475574842b44ca6792335ff141172d` | 2.3.1 |
| RESTfulAPI | `54c9124ca31d859f8af1704316cab65d1e398533` | 0.2.6.6 |
| Streamdev | `366a0ce9a8defb50f048983b4ff6c4da22fb19d0` | 0.6.5 |
| TVScraper | `2e63f40aff747f6dd8786aa8c2c7a4ef65cd5f08` | 1.2.15 |
| scraper2vdr | `d9f6cb454ebbc951af5d1a4aa7fcc31e772f3bca` | 1.0.11 |
| osd2web | `b2d61d9ecad3a213b5b8c9425d3eb83a21e51bcc` | 0.3.2 |
| epg2vdr | `c64172a4ef499f4398316031c66fd5bd88351dea` | 1.2.17 / DB API 8 |
| epgd | `388a693ffa7730f8ec7e279e604fbac21088455e` | 1.3.29 / DB API 8 |

The production reference server seen during the audit used VDR 2.7.9, epgsearch 2.4.6, RESTfulAPI 0.2.6.6, Streamdev 0.6.5 and TVScraper 1.2.15.

---

## Audit Method

For each component, the audit inspected:

- domain and identity semantics;
- lock and pointer lifetime behavior;
- mutation and error contracts;
- authentication, authorization and transport assumptions;
- multi-backend and multi-site suitability;
- event, snapshot and synchronization behavior;
- persistence and migration boundaries;
- reusable concepts versus implementation-specific liabilities.

The result was classified as:

```text
Adopt
Adapt
Reject
```

The target architecture was then compared with the actual VDR-Suite ADRs, roadmap and implementation state.

---

## Final Architecture Conclusion

VDR-Suite should remain a domain-first, agent-based multi-backend platform.

It should not become:

- a VDR fork;
- a thin public RESTfulAPI wrapper;
- a shared plugin-database client;
- an OSD menu mirror;
- a collection of frontend-specific backend calls.

The target boundary is:

```text
Clients
  -> Public VDR-Suite API
  -> Control Plane
  -> authenticated Agent protocol
  -> Backend Agent
  -> local adapters
  -> native VDR and plugins
```

VDR remains the native runtime authority. VDR-Suite owns cross-backend identity, policy, orchestration, durable jobs, normalized metadata, client contracts and audit.

---

## Component Conclusions

### VDR Core

Adopt native VDR semantics for timers, recordings, trash, restore, rename and runtime state.

Adapt them behind immutable VDR-Suite domain objects and short lock scopes.

Do not expose runtime-only recording identifiers, raw pointers or native paths as stable public identities.

### epgsearch

Adopt the rich automation concepts: search rules, history, repeat avoidance, conflict checking and remote timer awareness.

Adapt them into an automation-provider model that produces `TimerIntent` objects.

Do not allow epgsearch, VDR-Suite SearchTimer and other providers to maintain independent native timer ownership.

### Live

Use Live as a product-workflow and parity reference.

Do not reuse its global login state, MD5 credential model, GET mutations or direct native object ownership as VDR-Suite architecture.

### RESTfulAPI

Keep RESTfulAPI behind adapter boundaries as a compatibility transport.

It is not a public security gateway and must not define VDR-Suite identity, authorization, error or mutation contracts.

Unsafe or version-dependent operations remain capability-gated and default-blocked.

### Streamdev

Use Streamdev only as an internal media transport provider.

Public clients should receive authenticated, short-lived VDR-Suite media sessions rather than backend addresses or Streamdev URLs.

### TVScraper and scraper2vdr

Adopt provider-backed metadata, artwork and central catalog concepts.

VDR-Suite must own the normalized metadata database, provenance, confidence, entity identity and asset identity.

External plugin databases must not become the VDR-Suite public or internal domain contract.

### osd2web

Keep OSD access as an isolated legacy compatibility bridge only.

The primary Web, TV and native-client interfaces remain domain-first.

Any bridge requires explicit viewer and controller permissions, a controller lease, protocol sequencing, resynchronization and removal of arbitrary command execution.

### epg2vdr and epgd

Adopt stable backend identity, central EPG catalog, timer request and recording catalog concepts.

Reject direct shared-database coupling as the Agent protocol.

Replace implicit master election, global channel availability and first-claimer timer assignment with leases, backend-scoped capabilities, explicit assignment and reconciliation.

---

## Accepted Architecture Outputs

The first accepted package is:

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
```

The next contract package is:

```text
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
ADR-0046 - Streaming Gateway and Media Session Boundary
ADR-0047 - Legacy OSD Compatibility Bridge
ADR-0048 - Public API Versioning, Error and Compatibility Contract
ADR-0049 - Audit and Security Event Model
```

---

## Main Proven Architecture Invariants

1. Stable suite identity is separate from backend-native identity.
2. Backend identity is not derived from hostname or IP address.
3. Backend process generations and leases fence stale commands and results.
4. Capabilities and permissions are backend-scoped.
5. A read-only backend remains server-enforced.
6. Clients never communicate directly with VDR plugin ports or provider databases.
7. Backend Agents do not know the Control Plane database schema.
8. Snapshots and events carry backend ownership, revision and sequence information.
9. Mutations are authorized, revision-aware, idempotent, verified and audited.
10. Slow work uses durable jobs and compensation rather than long synchronous requests.
11. External I/O does not occur while native VDR locks are held.
12. Raw VDR pointers do not cross adapter boundaries.
13. Timer intent, backend assignment and native timer state are different objects.
14. Canonical programme identity is separate from backend event references.
15. Metadata and artwork remain available through suite-owned identities even when a provider is unavailable.
16. Streaming is exposed through VDR-Suite sessions, not internal transport URLs.
17. OSD is a compatibility bridge, not the primary application model.
18. API contracts are client-independent and versioned.
19. Security events and mutation audit are separate from normal diagnostics logging.
20. Additional plugin audits require a concrete implementation or risk question.

---

## Remaining Work

The audit itself is complete. Runtime implementation is not.

The remaining gaps, their current implementation status, their ADR dependency and their target roadmap phase are listed in:

- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)

---

## Back

- [Back to Development Index](index.md)
- [Back to Current State](../CURRENT.md)
- [Back to Documentation Index](../index.md)
