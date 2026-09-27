# Phase 69.D Closeout — Collections, Pagination and Partial Results

## Status

**COMPLETED**

```text
69.D=COMPLETED
next=69.E - Compatibility and deprecation policy
```

Phase 69 remains active. This closeout does not complete Phase 69, does not
promote every pre-v1 collection into the stable public contract and does not
define alias retirement or sunset policy ahead of 69.E.

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0020 Multi-Source Federation Architecture](../adr/ADR-0020-multi-source-federation-architecture.md)
- [ADR-0013 Permission Model](../adr/ADR-0013-permission-model.md)
- [Phase 69 Public API Kickoff and Runtime Progress](phase-69-public-api-kickoff.md)

## Completion decision

The live post-PR-364 audit found no remaining bounded 69.D runtime gap. Two
accepted collections deliberately prove the two different contract classes
required by ADR-0048:

| 69.D requirement | Accepted evidence |
| --- | --- |
| Standard public collection envelope | PR #363 `GET /api/v1/timer-assignments?backend=...` establishes `items`, `page`, `meta` and `links`. |
| Stable ordering | TimerAssignments use `timerAssignmentId ASC`; Channels use `backendId ASC, channelId ASC`. |
| Bounded pagination | Both collections use opaque keyset cursors with default limit 50, maximum 100 and no OFFSET contract. |
| Cursor scope safety | TimerAssignment cursors bind backend scope; Channel cursors bind the normalized requested backend-source set and reject changed scope. |
| Multi-backend partial results | PR #364 `GET /api/v1/channels` reports explicit per-source state and `meta.partial=true` when at least one authorized source succeeds and another is unavailable. |
| Explicit source failure | A failed Channel source is never silently omitted; all requested sources unavailable returns `503 backend_unavailable`. The single-source TimerAssignment collection also fails explicitly rather than returning a fake empty success. |
| Authorization across traversal | Every requested Channel backend is independently authorized and every page is re-authorized; TimerAssignment reads retain backend-scoped authorization. |
| Cache/revision truthfulness | Neither collection invents a collection ETag without an owning durable set/federation revision. |

No third runtime collection is required merely to re-prove these mechanics.
Future domain promotion remains a separate public-v1 stabilization decision.

## Accepted implementation chain

### PR #363 — first stable collection

```text
PR #363
title         = Add first Phase 69.D public collection
accepted head = 7c8e97b4b6e494a123a368395695459946630faa
merge/main    = 45ef8e3665082c2b1bb4e3e7f54c67e1e18833d6
CI #9291      = 36301143784 / SUCCESS (6/6)
```

This slice established the standard single-source collection envelope, stable
TimerAssignment identity/order, bounded keyset pagination, explicit repository
failure and the rule that no collection ETag exists without a durable set
revision owner.

### PR #364 — federated partial-result collection

```text
PR #364
title         = Add federated Phase 69.D Channel collection
accepted head = f2c7ada38b80ba55e8d216401ed744b6fc3b02ba
merge/main    = d39b1f6f52d2bc83458cabb7fb71bf4f8dac4890
CI #9295      = 36310001132 / SUCCESS (6/6)
```

This slice deliberately used Channels because stable `(backendId, channelId)`
identity, BackendRegistryService, VdrSnapshotReadService and
`channels.view@backend` already existed. It added no new repository, polling
loop, provider selector, lifecycle owner or fallback route.

The contract requires explicit repeated `backendId` scopes, caps requested
sources at 16, independently authorizes every source, uses scope-bound opaque
keyset traversal, exposes explicit `meta.sources`, returns partial success only
when at least one source is useful and returns `503 backend_unavailable` when
all requested sources fail.

## Hosted acceptance boundary

Both accepted 69.D slices are read-only public API contracts over existing read
authorities. Neither changes native VDR effect, installed media behavior,
SuiteBridge transport, Home, Recording metadata fanout, LiveTV or Live Preview.
Therefore the final 69.D closeout requires exact-head hosted CI and architecture
guards, not a repeated real-yaVDR native-effect gate.

## 69.D acceptance gate

| Gate | Result |
| --- | --- |
| Standard v1 collection envelope is represented | PASS — PR #363 |
| Stable deterministic ordering is represented | PASS — PR #363 / #364 |
| Bounded keyset pagination and opaque cursors are represented | PASS — PR #363 / #364 |
| OFFSET is not the stable public traversal contract | PASS |
| Multi-backend partial-result semantics are represented by a real federated source set | PASS — PR #364 |
| Failed sources are explicit and never silently omitted | PASS — PR #364 |
| All-source failure is not serialized as fake empty success | PASS — PR #364 |
| Source authorization remains backend-scoped and rechecked | PASS — PR #364 |
| No collection-wide ETag is fabricated without a revision owner | PASS |
| No pre-v1 collection was silently promoted by analogy | PASS |
| Home / Recording fanout / LiveTV / Timer mutation ownership unchanged | PASS |
| Exact accepted-head hosted CI for both runtime slices | PASS — 6/6 each |

## Retained boundaries

69.D does **not**:

- define additive versus breaking schema policy;
- define deprecation/sunset dates or alias retirement;
- complete versioned capability negotiation;
- complete the Phase-69 compatibility matrix;
- migrate all legacy collections into `/api/v1`;
- authorize public mutations merely because a read collection exists;
- reopen Timer CREATE/DELETE/MODIFY, ADR-0064 or Phase-64 Timer orchestration;
- change Home, Recording metadata/discovery, LiveTV/Live Preview or EPG
  aggregation ownership.

Those compatibility/deprecation concerns belong to 69.E. First-/third-party
client migration and wrapper hardening remain 69.F.

## Next bounded runtime slice

```text
69.D - Collections, pagination and partial results [COMPLETED]
  -> 69.E - Compatibility and deprecation policy [ACTIVE]
```

69.E must define the policy before retiring aliases: additive versus breaking
schema rules, versioned capability negotiation, deprecation/sunset metadata,
alias retirement rules and compatibility contract tests. Existing pre-v1 routes
remain classified inventory until that policy explicitly governs them.
