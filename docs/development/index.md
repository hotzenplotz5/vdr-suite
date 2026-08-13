# Development Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Purpose

This index organizes development contracts, closeouts, runtime acceptance evidence and developer references.

It intentionally does **not** maintain a second list of current phase markers, active PRs, exact heads or CI checkpoints. Volatile operational truth belongs only in [Current State](../CURRENT.md).

## Current orientation

Read these first:

1. [Current State](../CURRENT.md) — exact operational checkpoint.
2. [New Chat Handoff](../NEW-CHAT-HANDOFF.md) — workflow entry point.
3. [Current Project Status](current-status.md) — stable narrative context.
4. [Current Architecture State](current-architecture-state.md) — implemented architecture by durable capability boundary.
5. [Strict Roadmap](../planning/roadmap.md) — binding forward order.
6. [Architecture Map](architecture-map.md) — developer reading order.

## Completed phase evidence

- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

Historical phase and slice closeouts may contain exact source heads, CI runs, daemon hashes and real-system evidence paths. Those values remain valid historical proof for the accepted candidate they describe and must not be copied into this current index.

### Phase 62 security and accountability

- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)

### Phase 63 Backend Agent and secure multi-site runtime

Development and closeout material includes the bounded contracts and acceptance evidence for:

- Agent enrollment, identity, credential lifecycle, generation/instance fencing and heartbeat/lease;
- read-only observation/snapshot ingestion and resynchronization;
- durable command delivery, receipts/results and reconnect handling;
- fenced native execution;
- explicit local provider ownership/selection;
- generic protected-write safety contracts.

Useful documents include:

- [Backend Agent Foundation](phase-63-backend-agent-foundation.md)
- [Observation and Snapshot Ingestion](phase-63-observation-ingestion.md)
- [Durable Command Delivery](phase-63-command-delivery.md)
- [Fenced Native Operation](phase-63-fenced-native-operation.md)
- [Local Provider Ownership](phase-63-local-provider-ownership.md)
- [Local Provider Selection Runtime](phase-63-local-provider-selection-runtime.md)
- [Protected Write Contract](phase-63-protected-write-contract.md)

For exact completed-phase acceptance boundaries, use the relevant closeouts and [Completed Phases](completed-phases.md), not this index.

### Phase 64 Timer orchestration work

Phase-64 development documents describe the bounded TimerIntent, TimerAssignment, NativeTimerBinding, scheduling, native observation/readback and protected mutation contracts being implemented under ADR-0044.

The exact currently authorized checkpoint and whether a successor slice is allowed are volatile project-state facts and therefore live only in [Current State](../CURRENT.md).

## Developer references

- [Developer Onboarding](developer-onboarding.md)
- [Architecture Map](architecture-map.md)
- [Build System State](build-system-state.md)
- [GitHub Actions Status Handoff](github-actions-status-handoff.md)
- [Web Client API Contract Snapshot](web-client-api-contract-snapshot.md)
- [Person API](person-api.md)

## Documentation placement rules

- Exact current state belongs only in `docs/CURRENT.md`.
- Stable narrative context belongs in `current-status.md` and `current-architecture-state.md` without exact active-head duplication.
- Stable architecture belongs in `docs/architecture/` and accepted ADRs.
- Future dependency order and phase gates belong in `docs/planning/`.
- Historical runtime hashes and exact accepted heads stay in their closeouts.
- Slice documents remain traceability records and must not silently authorize successor implementation after their phase/context has moved on.
- Acceptance of an ADR is not runtime completion.

## Workflow rule

Before implementation or status claims, re-read live GitHub state and [Current State](../CURRENT.md). Do not infer current work from an old closeout, archived roadmap, stale handoff or once-proposed successor name.

## Related documents

- [Planning Documentation](../planning/index.md)
- [Architecture Documentation](../architecture/index.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)

## Back

- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
