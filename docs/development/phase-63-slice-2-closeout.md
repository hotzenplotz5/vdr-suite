# Phase 63 Slice 2 — Read-only Observation and Snapshot Ingestion Closeout

## Status

**Phase 63 Slice 2 is completed, accepted on the real yaVDR host and merged.**

Slice 2 began with the generic read-only Observation and Snapshot Ingestion
contract and was completed through two bounded runtime domains:

```text
backend-health
channels
```

The final merged `main` baseline for this closeout is:

```text
main commit: 39ed86fc3a425697f738f8f555394d54e4e1a684
main tree: e03bb84951cef7ec5f6b2f338ba456116cd766a2
```

Phase 63 remains incomplete. Durable command delivery, native execution and
provider ownership/selection remain later, separately contracted slices.

## Contract foundation — PR #138

PR #138, `Define read-only agent observation ingestion contract`, established
the binding generic ingestion semantics.

```text
accepted source head: 0207c0cbc01f167139b5d6483680f9a280c05160
merge commit: 24b1d7938ddaa15834a8da6323a270761868f4ba
CI: VDR-Suite CI #7275
run ID: 31006387349
result: all five jobs successful
runtime change: none
```

The contract separated:

- Backend generation;
- Agent instance;
- observation domain;
- snapshot generation;
- producer sequence;
- resource revision;
- complete baseline;
- exact-next change delivery;
- equivalent replay;
- conflicting replay;
- explicit `resync-required`;
- atomic Suite-owned receipt/fact/cursor persistence.

## Backend-health runtime — PR #139

PR #139, `Add backend health observation ingestion runtime`, implemented the
first bounded domain.

```text
accepted source head: 8a143f0e7e0f04ccda53edc9721a6f601b92c679
accepted tree: 71a33d2864aa921a2a82a3c7c1f85b1ddfd7056b
merge commit: 37df59552fd6d2f739c580dc9b472416f0bf5a12
CI: VDR-Suite CI #7282
run ID: 31015366065
result: all five jobs successful
```

Real yaVDR acceptance:

```text
PHASE_63_BACKEND_HEALTH_INGESTION_UPGRADE_ACCEPTANCE=PASS
HEAD=8a143f0e7e0f04ccda53edc9721a6f601b92c679
AGENT_ID=agt_f4af54c8acc53bbc27bbb47a265e3d7d
OBSERVATION_CURSOR_RESTART_PERSISTED=yes
OBSERVATION_REPLAY=yes
OBSERVATION_GAP_RESYNC=yes
EXISTING_AGENT_IDENTITY_PRESERVED=yes
VDR_NATIVE_STATE_UNCHANGED=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=/var/backups/vdr-suite-phase63-health-20260805T152050Z-8a143f0e7e0f
```

The acceptance used no enrollment, revocation, replacement or manual SQLite
inspection.

## Channel contract — PR #140

PR #140, `Define read-only channel observation ingestion contract`, bound the
next VDR read domain to stable native Channel identity and complete snapshot
semantics before runtime implementation.

```text
accepted source head: d394988a3d123b73b33d0935a98630fcc5db1ff0
accepted tree: c4afb5d9c413401476e642c8799178a8e7676d33
merge commit: 2567407e6e1c6d098804f887875e7f3cbf9cba60
CI: VDR-Suite CI #7284
run ID: 31027021783
result: all five jobs successful
runtime change: none
```

The contract fixed:

- observation domain exactly `channels`;
- native identity `(backendId, channelId)`;
- explicit Channel fact fields;
- deterministic canonicalization;
- complete baseline and explicit removals;
- Agent-owned persistence separate from `VdrChannelCacheRepository`;
- one explicit later site-local source;
- no provider ownership/selection or native Channel mutation.

## Channel runtime — PR #141

PR #141, `Add read-only channel observation runtime`, selected `channels.conf`
as the explicit site-local source and implemented the accepted domain contract.

```text
accepted source head: 67585b5884653576ac07616cb1653d4ad8ca17ad
accepted tree: e03bb84951cef7ec5f6b2f338ba456116cd766a2
merge commit: 39ed86fc3a425697f738f8f555394d54e4e1a684
CI: VDR-Suite CI #7298
run ID: 31036980183
result: all five jobs successful
```

The first real-system attempt on superseded head
`723f673caffa1ab5139942d800e06a06d7cd3722` failed closed because the installed
candidate binaries were not proven to have been rebuilt from that exact
checkout. The failure restored the original configuration, removed the
temporary fixture and left VDR, daemon and Agent active. It exposed two harness
defects:

- byte equality between `.build` and `/usr/sbin` did not prove an exact-head
  rebuild;
- a failed wait inside command substitution was not propagated immediately.

The final runner rebuilds daemon, Agent, enrollment and administration binaries
from the exact clean checkout before comparison and terminates every failed wait
with one deterministic reason.

Final real yaVDR acceptance:

```text
PHASE_63_CHANNEL_OBSERVATION_UPGRADE_ACCEPTANCE=PASS
HEAD=67585b5884653576ac07616cb1653d4ad8ca17ad
AGENT_ID=agt_f4af54c8acc53bbc27bbb47a265e3d7d
INITIAL_CHANNEL_FACT_COUNT=343
CHANNEL_BASELINE=yes
CHANNEL_FIXTURE_TRANSITION=yes
CHANNEL_OBSERVATION_REPLAY=yes
CHANNEL_OBSERVATION_GAP_RESYNC=yes
CHANNEL_CURSOR_RESTART_PERSISTED=yes
CHANNEL_RECOVERY_AFTER_RESYNC=yes
EXISTING_AGENT_IDENTITY_PRESERVED=yes
CREDENTIAL_GENERATION_PRESERVED=yes
VDR_NATIVE_STATE_UNCHANGED=yes
VDR_READ_ONLY_REGRESSION=yes
ORIGINAL_CONFIGURATION_RESTORED=yes
VDR_ACTIVE=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=/root/vdr-suite-phase63-channel-acceptance-67585b58
```

The failed first-run evidence remains retained separately at:

```text
/root/vdr-suite-phase63-channel-acceptance-723f673c
```

The pre-install rollback binaries remain retained at:

```text
/root/vdr-suite-phase63-binary-backup-before-67585b58
```

## Completed Slice-2 boundary

Slice 2 now provides:

- authenticated Agent-only observation endpoints;
- Backend, Agent, Agent-instance and backend-generation fencing;
- independent snapshot generation, producer sequence and resource revision;
- complete baseline before changes;
- exact-next change acceptance;
- equivalent replay idempotency;
- conflicting replay rejection;
- explicit `resync-required`;
- restart-safe Agent lineage and pending-envelope retry;
- atomic immutable receipts, current cursor and domain facts;
- bounded strict JSON parsing;
- redacted administration readback;
- `backend-health` runtime;
- read-only `channels` runtime sourced explicitly from `channels.conf`;
- guarded exact-head, upgrade-safe real-system acceptance.

The accepted runtime remains read-only.

## Preserved boundaries

Slice 2 did not implement:

- command inbox, dispatch, receipts or results;
- a production Operation/Job/Attempt runtime;
- VDR-native execution;
- Recording, Timer, SearchTimer, Remote, configuration or metadata mutation;
- provider ownership or provider selection;
- public Agent/provider URLs;
- TimerIntent or Phase 64;
- Streaming Gateway or media sessions;
- OSD snapshots, controller lease or remote input;
- replacement of direct-adapter `BackendNode.online` authority;
- frontend migration to Agent-owned Channel facts.

VDR remains authoritative for native state. The Agent-owned Channel facts remain
separate from `VdrChannelCacheRepository` and `vdr_channel_cache`.

## Security and evidence position

- Existing Phase-62 identity, RBAC, read-only policy and accountability remain
  authoritative.
- Agent credentials remain technical identities and cannot act as browser users.
- Existing Agent ID and credential generation were preserved.
- Runtime and bootstrap secrets were not printed or committed.
- No manual SQLite inspection was used.
- VDR-native fingerprints remained unchanged.
- VDR, daemon and Agent were active after final acceptance.

## Next bounded slice

The next strict slice is:

```text
Phase 63 Slice 3 — Durable Agent Command Delivery
```

Its binding contract is
[Phase 63 Slice 3 — Durable Agent Command Delivery Contract](phase-63-command-delivery.md).

Slice 3 must establish durable assignment, Agent receipt, local execution
boundary, result outbox, replay, fencing and reconnect semantics before any
native command type or Phase-64 orchestration is considered.

## Related documents

- [Phase 63 Slice 3 — Durable Agent Command Delivery Contract](phase-63-command-delivery.md)
- [Phase 63 Slice 1 Closeout](phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md)
- [Phase 63 Channel Observation Ingestion](phase-63-channel-observation-ingestion.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
