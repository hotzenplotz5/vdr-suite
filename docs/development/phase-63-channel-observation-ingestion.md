# Phase 63 — Read-only Channel Observation Ingestion Contract

## Status

**Binding contract for the next bounded read-only Phase-63 observation domain.**

This document defines the `channels` observation domain that may follow the
merged and accepted `backend-health` runtime. It is a contract-only slice.
It adds no Agent route, no payload parser, no channel source adapter, no database
schema, no daemon or Agent behaviour, no packaging change and no installed
runtime effect.

Merged foundation at contract creation:

```text
main commit: 37df59552fd6d2f739c580dc9b472416f0bf5a12
main tree: 71a33d2864aa921a2a82a3c7c1f85b1ddfd7056b
accepted runtime domain: backend-health
next bounded contract domain: channels
```

A separate Draft PR is required to implement this contract. That runtime PR must
select and prove one explicit site-local read source instead of guessing whether
channels come from `channels.conf`, RESTfulAPI, SVDRP, SuiteBridge or another
adapter.

## Goal

Allow an enrolled Backend Agent to publish one complete, bounded and
generation-/sequence-fenced view of the VDR channels visible at its Backend,
followed by exact-next changes, without making the Agent authoritative over VDR
or over existing direct-adapter channel state.

The result is a Suite-owned Agent observation lineage that can later support
explicit provider ownership and read-model selection. This contract does not
perform that later selection.

## Relationship to the generic ingestion contract

All requirements from
[Phase 63 Slice 2 — Read-only Observation and Snapshot Ingestion Foundation](phase-63-observation-ingestion.md)
remain binding, including:

- technical authentication through `vdr-suite-agent/1`;
- Backend, Agent, Agent-instance and backend-generation fencing;
- complete baseline before changes;
- independent `snapshotGeneration`, `producerSequence` and `resourceRevision`;
- exactly-next change sequencing;
- equivalent replay idempotency;
- conflicting replay rejection;
- explicit `resync-required` on gaps or missing baselines;
- atomic Suite-owned receipt, fact/read-model and cursor persistence;
- bounded diagnostics and accountability without secrets;
- no manual SQLite inspection for acceptance.

This document narrows those rules for the `channels` domain.

## Observation-domain identifier

The identifier is exactly:

```text
channels
```

The Agent may publish it only after `channels` is present in the accepted
read-only capability set for the current Agent instance and backend generation.

## Native identity

The stable resource identity is:

```text
(backendId, channelId)
```

`channelId` is the VDR-native channel identifier delivered by the selected
site-local read source. Channel number, display name, provider name and list
position are mutable attributes and must never be used as identity.

The later runtime must reject:

- an empty `channelId`;
- an identifier outside the bounded safe-string contract;
- duplicate `channelId` values within one complete snapshot;
- the same `channelId` in both `upserts` and `removedChannelIds`;
- any attempt to move one resource between Backend identities implicitly.

A change in native `channelId` is represented as removal of the old identity and
upsert of the new identity. It is never inferred from equal names or numbers.

## Channel fact

Each upserted channel contains exactly these bounded fields:

```text
channelId
channelNumber
name
provider
groupName
radio
encrypted
enabled
```

Semantics:

- `channelNumber` is presentation/order evidence, not identity;
- `name`, `provider` and `groupName` are bounded UTF-8 strings;
- `radio`, `encrypted` and `enabled` are booleans;
- no stream URL, provider URL, credential, filesystem path, command capability
  or private adapter configuration is part of the fact;
- fields not listed here are rejected rather than silently accepted.

The runtime PR must define explicit numeric and string bounds and a maximum
channel count before parsing or persistence. The complete request remains
subject to the generic Agent request-size and nesting bounds.

## Canonical representation and payload identity

For replay comparison and immutable evidence, canonicalization must be
deterministic:

- sort complete-snapshot channel items by `channelId`;
- sort `upserts` by `channelId`;
- sort `removedChannelIds` lexicographically;
- preserve every declared field with one unambiguous type;
- reject duplicate object keys, duplicate resource identities, unknown fields,
  invalid UTF-8 and non-canonical numeric overflow before persistence;
- derive `payloadIdentity` from the complete canonical domain payload, not from
  capture time, HTTP formatting or item order.

Equivalent semantic payloads therefore replay idempotently even when input item
order differs. A different canonical payload at the same fenced lineage key is a
conflicting replay.

## Complete snapshot

A `completeSnapshot`:

- starts a new strictly newer `snapshotGeneration`;
- uses `producerSequence = 1`;
- contains the complete bounded set of Agent-observed channel facts for the
  selected source at `capturedAt`;
- may be empty only when the selected source successfully and explicitly reports
  an authoritative empty channel set;
- atomically replaces the active **Agent-owned channels observation lineage**;
- records explicit removals relative to the previous Agent-owned lineage through
  snapshot replacement semantics;
- does not delete, overwrite or outrank existing direct-adapter channel state.

Failure or timeout of the local source is not an empty snapshot. It fails closed,
does not advance the cursor and retains the last accepted Agent-owned lineage.

## Change batch

A `changeBatch` is valid only after an accepted complete baseline for the same
Backend, Agent, Agent instance, backend generation and snapshot generation.

Its domain payload contains exactly:

```text
upserts
removedChannelIds
```

Rules:

- `upserts` uses the complete Channel fact shape;
- `removedChannelIds` contains explicit VDR-native channel identities;
- the two identity sets are disjoint;
- both sets are independently bounded;
- at least one set is non-empty;
- the batch advances only at the exact next `producerSequence`;
- a missing identity is not interpreted as deletion;
- deletion is accepted only through `removedChannelIds` or a newer complete
  snapshot;
- upsert of an existing identity replaces that Agent-owned fact atomically;
- removal of an unknown identity is rejected as a lineage conflict rather than
  silently ignored.

No partial application is permitted. Facts, immutable receipt and current cursor
commit in one Suite-owned transaction.

## Source acquisition boundary

The later runtime implementation must bind `channels` to one explicit
site-local read source and document why that source provides a complete snapshot
and stable VDR-native channel identity.

The implementation must not:

- auto-select a source based on what happens to respond first;
- fall back between RESTfulAPI, SVDRP, SuiteBridge, `channels.conf` or another
  source without a separate ordered provider-selection contract;
- publish or persist a private provider URL;
- read secrets into payloads, logs or evidence;
- require a new public Agent or provider endpoint;
- modify `vdr-plugin-suite-bridge` merely because it exists;
- treat an existing Suite cache as proof of local source authority.

Source failure, malformed source data or identity ambiguity returns a bounded
failure and leaves the previously accepted lineage and cursor unchanged.

## Authority and read-model boundary

VDR remains authoritative for VDR-native channels.

The existing direct-adapter channel cache and current client/frontend read paths
remain unchanged and authoritative for their established behaviour. In
particular, this contract does not replace or silently repurpose
`VdrChannelCacheRepository` / `vdr_channel_cache` as Agent-owned authority.

The runtime PR must use an explicitly Agent-owned Suite repository/read model or
an equivalent ownership-separated schema. Joining, preferring or replacing
direct-adapter facts requires a later provider ownership and provider selection
contract.

Agent channel observations:

- do not change `BackendNode.online`;
- do not reorder or edit VDR channels;
- do not change channel groups;
- do not create, delete, enable or disable native channels;
- do not become a command inbox or command result;
- do not authorize any browser, user or administrative action.

## Persistence and atomicity

The runtime implementation must atomically persist:

- immutable observation receipt and outcome;
- current fenced ingestion cursor;
- active Agent-owned channel facts for the accepted lineage;
- provenance linking every active fact to Backend, Agent, Agent instance,
  backend generation, snapshot generation, producer sequence, capture time and
  resource revision.

A transaction failure leaves receipt/facts/cursor in the documented
all-or-nothing state and never advances the cursor without the matching facts.

Repository classes own SQLite. HTTP handlers, Agent client transport and local
source adapters issue no direct SQL.

## Replay, gaps, restart and resynchronization

The generic rules apply independently to `channels`:

- equivalent replay is acknowledged without duplicate facts or cursor movement;
- conflicting replay is rejected;
- stale backend generation, Agent instance, snapshot generation or producer
  sequence is rejected;
- missing baseline or sequence gap returns `resync-required`;
- restart preserves the last committed cursor and active Agent-owned facts;
- Agent pending-envelope retry resends the byte-/semantic-equivalent envelope
  after an ambiguous transport result;
- resynchronization starts a newer complete snapshot and never guesses missing
  changes.

## Required automated coverage for the runtime PR

The separate runtime PR must cover at least:

- valid complete Channel snapshot acceptance and readback;
- deterministic canonicalization independent of input order;
- valid exact-next upsert and explicit removal;
- equivalent replay idempotency;
- conflicting replay rejection;
- duplicate identity and upsert/removal overlap rejection;
- missing-baseline and sequence-gap `resync-required`;
- stale Backend, Agent-instance and snapshot-generation fencing;
- undeclared `channels` capability rejection;
- malformed, oversized, overflow and unknown-field rejection;
- source failure not becoming an empty snapshot;
- unknown removal rejection;
- newer complete-snapshot replacement semantics;
- transaction rollback without cursor or fact advancement;
- restart-safe cursor, facts and replay behaviour;
- lost-response pending-envelope retry;
- direct-adapter channel cache and read paths unchanged;
- no channel command/mutation route;
- no direct SQLite outside repository classes;
- build, packaging, documentation and Make inventory.

## Real yaVDR acceptance direction

Because this contract-only slice changes no runtime, it requires no real-system
installation or yaVDR acceptance.

The later runtime PR must execute guarded exact-head acceptance that proves:

- the existing enrolled Agent identity is preserved;
- one explicit configured local channel source yields a complete baseline;
- one representative exact-next change or safely controlled fixture transition
  is observed without VDR-native mutation;
- equivalent replay, deliberate gap/resync and restart persistence work;
- Agent-owned readback matches the selected source;
- existing direct-adapter channel behaviour remains unchanged;
- VDR-native channel fingerprints remain unchanged;
- VDR, daemon and Agent services are active afterwards;
- installed binaries match the candidate build;
- retained evidence is root-only and secret-free.

No enrollment, revocation, Agent replacement, manual SQLite inspection or native
channel mutation is part of acceptance.

## Hard exclusions

This contract and its immediate runtime do not include:

- Timer, TimerIntent or Phase 64;
- EPG, recordings, SearchTimer or metadata ingestion;
- command inbox, dispatch, receipts or results;
- VDR channel movement, editing, creation, deletion or enablement;
- provider ownership or provider selection;
- public Agent/provider URLs;
- Streaming Gateway, media sessions or OSD;
- replacement of direct-adapter channel authority;
- frontend/client migration to Agent-owned channels;
- `BackendNode.online` authority changes.

## Exit criterion

This contract slice is complete when:

- the contract is present and CI-guarded;
- documentation and Make inventory pass on one exact Draft-PR head;
- the diff contains no runtime, schema, API, Agent-client, packaging or plugin
  implementation;
- the PR remains Draft until separate explicit approval.

Merging this contract authorizes only a separate bounded Draft runtime PR. It
does not itself authorize broader VDR domains, commands, provider ownership or
Phase 64.

## Related documents

- [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md)
- [Phase 63 Backend Agent Runtime Acceptance](phase-63-backend-agent-runtime-acceptance-runbook.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
