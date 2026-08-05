# Phase 63 Slice 2 — Read-only Observation and Snapshot Ingestion Foundation

## Status

**Binding contract for the next bounded Phase-63 runtime slice.**

This slice starts only after the accepted and merged [Phase 63 Slice-1 closeout](phase-63-slice-1-closeout.md). It extends the existing `vdr-suite-agent/1` lifecycle with generation- and sequence-fenced read-only observation ingestion. It does not add a command channel or any VDR-native mutation.

## Goal

Allow an enrolled Backend Agent to publish bounded immutable observations and complete domain snapshots to the Control Plane so that remote-site state can be persisted in Suite-owned read models without exposing private providers or weakening existing direct-adapter authority.

The smallest accepted result is a secure ingestion foundation that can:

- authenticate the Agent through its existing technical credential;
- bind every accepted observation to the current Backend, Agent, Agent process instance and backend generation;
- distinguish complete snapshots from later changes;
- enforce monotone producer sequence and snapshot generation;
- reject stale, replay-conflicting or gapped input deterministically;
- request explicit resynchronization instead of guessing continuity;
- persist accepted observation facts and ingestion cursor atomically;
- retain provenance and receipt evidence without storing secrets;
- expose no new public client API and execute no VDR operation.

## Trust and authority boundary

- VDR remains authoritative for VDR-native state.
- The Backend Agent is a bounded site-local observation producer, not a global policy authority.
- The Control Plane validates, persists and derives Suite read models.
- Existing local/direct adapters remain authoritative for their current runtime paths until provider ownership and selection receive a separate binding contract.
- An Agent observation does not silently overwrite a direct-adapter fact with higher authority.
- Provider/native facts retain producer, backend, generation, sequence, domain and capture-time provenance.

## Protocol envelope

Every observation request is authenticated with the existing Agent credential and carries a bounded envelope equivalent to:

```text
protocolVersion = vdr-suite-agent/1
backendId
agentId
agentInstanceId
backendGeneration
observationDomain
snapshotGeneration
producerSequence
kind = completeSnapshot | changeBatch
capturedAt
resourceRevision
payload
```

The exact serialized JSON is implementation-owned, but these semantics are mandatory and independent:

- `backendGeneration` fences replacement or resynchronized backend state;
- `agentInstanceId` fences stale Agent processes within the accepted backend generation;
- `snapshotGeneration` identifies one complete snapshot lineage for one observation domain;
- `producerSequence` is monotone within that domain and snapshot generation;
- `resourceRevision` is domain/resource revision evidence and is not interchangeable with producer sequence;
- `completeSnapshot` establishes a complete bounded baseline;
- `changeBatch` advances only from an accepted baseline without a gap.

## Initial observation domains

The runtime implementation must begin with the smallest domain that proves the protocol and persistence boundary. The default first domain is:

```text
backend-health
```

A later implementation commit may add one bounded VDR read domain only when its native identity and complete-snapshot semantics are explicit. Adding recordings, timers, EPG or channels is not automatically part of this contract PR.

Observation-domain identifiers must be filename-/log-safe bounded ASCII identifiers and must be present in the Agent's accepted read-only capability set.

## Acceptance rules

An observation is accepted only when all of the following hold:

1. The request resolves to an active, non-revoked Agent credential.
2. `backendId` and `agentId` match the authenticated technical identity.
3. `protocolVersion` equals `vdr-suite-agent/1`.
4. `agentInstanceId` equals the currently connected accepted instance.
5. `backendGeneration` equals the current accepted generation.
6. The Agent lease is current enough for ingestion according to one explicit server policy.
7. `observationDomain` is declared in the accepted read-only capabilities.
8. Payload, item count, string length, nesting and request size are bounded before persistence.
9. A `completeSnapshot` has a strictly valid snapshot generation and initial sequence.
10. A `changeBatch` references the currently accepted snapshot generation and exactly the next producer sequence.
11. The complete persistence transaction can commit the immutable receipt/fact and ingestion cursor together.

## Replay, gaps and resynchronization

The server must classify duplicate and discontinuous delivery explicitly:

- byte-/semantic-equivalent replay of an already accepted `(domain, snapshotGeneration, producerSequence)` is idempotently acknowledged;
- conflicting replay of the same key is rejected and recorded;
- a lower generation or lower sequence is stale and rejected;
- a sequence gap is rejected with `resync-required`;
- a change before an accepted complete snapshot is rejected with `resync-required`;
- a newer complete snapshot may replace the active lineage only through one atomic generation transition;
- no missing change is synthesized and no continuity is guessed from timestamps.

Reconnect must preserve the last committed cursor. Process restart, network retry and duplicate delivery must not create duplicate active facts or advance the cursor twice.

## Persistence model

The implementation must use Suite-owned repositories and transactions. At minimum it needs:

- one immutable observation receipt/fact record carrying provenance and bounded payload identity;
- one current ingestion cursor per `(backendId, observationDomain)`;
- active snapshot-generation and last producer-sequence state;
- accepted Agent ID, Agent instance ID and backend generation evidence;
- accepted/rejected outcome reason suitable for accountability without payload secrets;
- deterministic lookup for idempotent replay and conflicting replay detection.

Raw credentials, Authorization headers, enrollment tokens, credential secrets and secret-bearing process environment are never persisted in observation tables or normal logs.

The repository layer owns SQLite. HTTP handlers and Agent client code must not issue direct SQLite statements.

## HTTP and client boundary

The Control Plane may add authenticated Agent-only endpoints under:

```text
/api/agent/v1/observations/...
```

Requirements:

- HTTPS verification remains mandatory;
- redirects, proxy inheritance and netrc remain disabled in the Agent transport;
- request and response sizes are bounded;
- errors use stable machine-readable reason codes;
- no browser session, CSRF or user credential is accepted as Agent authentication;
- no observation endpoint is exposed as a public provider URL;
- response bodies contain no private provider address or credential material.

## Accountability and diagnostics

Accepted, replayed, rejected and resync-required outcomes must retain bounded evidence linking:

- request and correlation IDs;
- authenticated Agent actor/device/credential identity;
- backend ID;
- observation domain;
- backend generation;
- Agent instance ID;
- snapshot generation and producer sequence;
- outcome reason.

Normal diagnostics must be useful without printing payloads that may contain titles, paths or provider-native details. Any retained runtime acceptance evidence must pass the deterministic secret scanner introduced in Slice 1.

## Runtime and failure behaviour

- Ingestion failure must not stop VDR.
- Ingestion failure must not disable existing direct adapters.
- Database failure fails the observation closed and does not advance the cursor.
- Invalid payloads do not partially update a read model.
- Stale Agent processes cannot complete current-generation ingestion.
- Revocation immediately prevents further accepted observations.
- Lease expiry and reconnect behaviour remain compatible with Slice 1.
- Observation backoff is bounded and does not spin.

## Hard exclusions

This slice does **not** implement:

- command inbox, command dispatch, receipts or results;
- Timer, Recording, SearchTimer, Remote or configuration mutation;
- VDR-native execution of any kind;
- universal Operation/Job/Outbox infrastructure;
- provider ownership or provider selection;
- public Agent/provider URLs;
- Streaming Gateway or media sessions;
- OSD snapshots, controller lease or remote input;
- TimerIntent orchestration or Phase 64;
- replacement of existing direct-adapter `BackendNode.online` authority;
- broad migration of all current local snapshot caches to Agent delivery.

## Required automated coverage

The implementation PR must cover at least:

- valid complete baseline acceptance;
- valid next-sequence change acceptance;
- equivalent replay idempotency;
- conflicting replay rejection;
- stale backend generation rejection;
- stale Agent instance rejection;
- undeclared observation-domain rejection;
- change-before-baseline rejection;
- sequence-gap `resync-required`;
- newer complete-snapshot lineage transition;
- transaction rollback without cursor advance;
- restart-safe cursor and replay behaviour;
- revoked-Agent denial;
- malformed, oversized and overflow payload rejection;
- repository/HTTP SQLite boundary;
- no command or mutation route introduced;
- build, packaging, documentation and Make inventory.

## Real-system acceptance direction

Draft PR #139 provides a guarded exact-head, upgrade-safe yaVDR acceptance path that proves:

- existing Slice-1 Agent stays online;
- one bounded read-only observation domain establishes a complete baseline;
- one next-sequence update is accepted;
- duplicate replay is idempotent;
- a deliberate gap returns `resync-required` without corrupting the cursor;
- restart preserves the committed cursor;
- the existing accepted Agent identity is preserved across daemon/Agent restart and rejected-gap recovery;
- VDR-native fingerprints remain unchanged;
- VDR, daemon and Agent services remain active;
- evidence is root-only and secret-free.

No manual SQLite inspection is part of acceptance. The redacted administration tool and guarded harness expose the required cursor state while a root-only helper reads protected identity material internally without printing it.

## Exit criterion

Slice 2 is complete only when one exact implementation head has:

- all required CI jobs green;
- exact contract and architecture checks green;
- real yaVDR acceptance for the implemented observation domain;
- no VDR-native mutation;
- no command/result flow;
- no secret-bearing evidence;
- explicit user approval before Ready or merge.

## Related documents

- [Phase 63 Slice-1 Closeout](phase-63-slice-1-closeout.md)
- [Phase 63 Backend Agent Foundation](phase-63-backend-agent-foundation.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
