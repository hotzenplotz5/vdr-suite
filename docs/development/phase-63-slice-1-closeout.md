# Phase 63 Slice 1 — Backend Agent Enrollment and Lease Foundation Closeout

## Status

**Phase 63 Slice 1 is completed, accepted on the real yaVDR host and merged.**

PR #137, `Add backend agent enrollment and lease foundation`, was squash-merged into `main` as:

```text
a9620179a442155f0860ef3182ca39186ac46a57
```

The accepted source head was:

```text
bba51455552bab0f1a06c680369c508858b2384b
```

The merge commit carries the same accepted tree:

```text
575f49a197cda9ad02da4035b437ee1c32bed2d6
```

## Automated validation

The exact accepted source head passed VDR-Suite CI #7256, run ID `31001478896`, with all five required jobs successful:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`, including the complete fast suite and production daemon build;
- `packaging-regression-test`, including install staging, Agent state-directory ownership and administration binaries.

The repository-wide architecture checker still reports the seven pre-existing main-baseline SQLite allowlist gaps outside this slice. Slice 1 introduced no additional architecture-check failure.

## Real yaVDR acceptance

The guarded exact-head acceptance completed successfully:

```text
PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS
HEAD=bba51455552bab0f1a06c680369c508858b2384b
CONTROL_PLANE_URL=https://192.168.178.38/vdr-suite
FIRST_AGENT_ID=agt_20baf6df0defd62bc2da69f2515ab038
REPLACEMENT_AGENT_ID=agt_f4af54c8acc53bbc27bbb47a265e3d7d
CREDENTIAL_GENERATION=2
VDR_NATIVE_STATE_UNCHANGED=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=/var/backups/vdr-suite-phase63-20260805T114111Z-bba51455552b
```

The accepted path proved:

- controlled one-time enrollment into an existing Backend;
- persistent technical actor, device and credential identity;
- exact `vdr-suite-agent/1` protocol compatibility;
- Agent instance and backend-generation fencing;
- heartbeat and lease transitions through online, stale and offline;
- reconnect of the same Agent identity;
- bounded read-only capability publication;
- credential rotation to generation 2;
- rejection of a revoked Agent;
- distinct replacement enrollment with retained revoked history;
- unchanged VDR-native fingerprints;
- active VDR, daemon and replacement Agent services;
- secret-free retained evidence logs.

## Completed Slice-1 boundary

Slice 1 established only the secure Agent lifecycle foundation:

- enrollment and technical identity;
- protected outbound HTTPS transport;
- protocol, generation and process-instance fencing;
- heartbeat, lease and derived connection state;
- read-only capability publication;
- reconnect and credential lifecycle;
- revocation and replacement;
- protected local state, service packaging and administration;
- accountability and authorization reuse from Phase 62.

It deliberately did **not** implement:

- domain snapshot or change ingestion;
- durable command, receipt or result flow;
- VDR-native execution;
- provider ownership or selection;
- public provider URLs;
- TimerIntent orchestration;
- Streaming Gateway or OSD runtime;
- Phase 64-or-later work.

Agent lifecycle state remains separate from the existing direct-adapter `BackendNode.online` authority.

## Next bounded slice

Phase 63 remains incomplete. The next strict slice is:

```text
Phase 63 Slice 2 — Read-only Observation and Snapshot Ingestion Foundation
```

Its binding contract is [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md).

Slice 2 must establish generation- and sequence-fenced read-only observation ingestion before any command/result runtime is considered.

## Related documents

- [Current State](../CURRENT.md)
- [Current Project Status](current-status.md)
- [Phase 63 Backend Agent Foundation](phase-63-backend-agent-foundation.md)
- [Phase 63 Runtime Acceptance](phase-63-backend-agent-runtime-acceptance-runbook.md)
- [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
