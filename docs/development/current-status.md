# VDR-Suite Current Project Status

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main baseline:
96fab8ad88eae9ea0d46adf4db50ccf8d750a19b

Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Merged Phase-64 foundation:
PR #150 - TimerIntent Domain Contract
PR #152 - TimerIntent Persistence and Repository Semantics

Current stacked Draft tip:
PR #168 - Add failure-aware RESTfulAPI native Timer inventory reader
branch: agent/phase64-restfulapi-native-timer-inventory-reader
head: a49a3b99167e8f1bbbbf220657d774aaf4038501
CI: VDR-Suite CI #7412 / run 31462922497 - PASS
runtime acceptance: not required; Slice-16 is not linked into an installed runtime target
```

Exact branch, PR head, mergeability and CI must be re-read from GitHub before every resumed workstream. Historical hashes remain evidence for their exact candidates only.

## Completed Phase 63 foundation

Phase 63 is complete. Its accepted foundation includes:

- controlled Backend Agent enrollment and technical identity;
- protected outbound transport, credential lifecycle, lease and backend-generation fencing;
- generation-/sequence-fenced read-only observation ingestion;
- explicit Channel observation from a selected local source;
- durable Agent command assignment, receipt, result and uncertain-outcome reconciliation;
- side-effect-free fenced SuiteBridge native execution with plugin-instance epoch and authoritative readback;
- explicit local-provider facts, ownership and immutable provider selection with no silent fallback;
- the generic protected-write safety contract used by later domain mutations.

Phase 63 did not itself create TimerIntent, TimerAssignment or NativeTimerBinding. Those belong to Phase 64.

## Phase 64 progress

### Merged foundation

PR #150 established the backend-neutral `TimerIntent` domain contract. PR #152 added Suite-owned persistence and repository-issued revision semantics.

### Active stacked Drafts

```text
Slice 3   PR #153 - TimerAssignment domain contract
Slice 4   PR #154 - TimerAssignment persistence repository
Slice 5   PR #155 - deterministic TimerAssignment planner
Slice 6   PR #158 - primary assignment scheduling handoff
Slice 7   PR #159 - assignment-set revision fence
Slice 8   PR #160 - replica assignment scheduling handoff
Slice 9   PR #161 - NativeTimerBinding domain contract
Slice 10  PR #162 - NativeTimerBinding persistence repository
Slice 11  PR #163 - VDR NativeTimerObservation mapper
Slice 12  PR #164 - safe present-readback application
Slice 13  PR #165 - native Timer expected PRESENT readback contract
Slice 14  PR #166 - operation-aware PRESENT readback verification
Slice 15  PR #167 - authoritative complete inventory / absence evidence
Slice 16  PR #168 - failure-aware RESTfulAPI Timer inventory reader
```

The stack is deliberately built in bounded slices so native Timer execution is not enabled before identity, concurrency, readback and uncertain-outcome semantics exist.

Draft PR #157 is a separate SQLite architecture-baseline repair. Draft PR #156 is a separate proposed client playback/media-adaptation architecture change and does not advance Phase-65 runtime.

## Current Timer guarantees

The current stack establishes:

- stable Suite identity distinct from backend-native Timer identity;
- repository-issued intent, assignment and binding revisions;
- one durable primary-owner fence and complete assignment-set concurrency fencing for replica decisions;
- deterministic backend selection from explicit authoritative evidence;
- backend-neutral copied native Timer observation values;
- safe present refresh that preserves unresolved drift/missing evidence;
- operation-aware expected-present verification using exact operation, revision, generation, time and normalized-state fences;
- authoritative absence evidence only from a proven complete Timer inventory;
- failure-aware RESTfulAPI inventory acquisition that preserves HTTP/parse failure as non-evidence.

Production native Timer mutation remains disabled. There is no Agent/SuiteBridge/RESTfulAPI/SVDRP Timer create/update/delete orchestration path enabled by the current stack.

## Next bounded slice

The immediate next Phase-64 slice after PR #168 is NativeTimerBinding absence application.

The service must:

1. load exact current durable binding state;
2. consume only valid complete inventory evidence for the exact backend and generation;
3. prove the queried native Timer is absent and the evidence is new enough;
4. persist `missingSince`/missing observation evidence without erasing the last known present representation or fingerprint;
5. use exact binding-revision optimistic concurrency with no hidden stale-evidence retry;
6. avoid guessing `external_delete` or failover eligibility from absence alone.

Later bounded work includes operation-aware delete verification, changed-state/external-drift reconciliation, assignment/binding transitions, controlled replacement/failover, protected native Timer create/update/delete, daemon orchestration and public Timer surfaces.

## Streaming and Timer UI ordering

The strict numbered runtime sequence is:

```text
Phase 64 Timer orchestration
  -> Phase 65 Streaming Gateway
  -> Phase 66 Legacy OSD bridge
  -> Phase 67 Public API/client hardening
```

The broad Timer UI is deliberately **not** required to close Phase 64. Broad Timer mutation controls remain gated on account/backend access management built on Phase 62.

Consequently it is valid and intended for Phase 65 Streaming to start before the broad Timer UI is finished. This is an execution/product-order decision, not a technical dependency of streaming on Timer functionality or vice versa.

## Security and authority rules

- Root-level `AGENTS.md` is binding for all repository work.
- Phase-62 identity/RBAC/accountability remains the user-facing trust foundation.
- Phase-63 Agent identity, backend generation, command delivery and explicit provider ownership remain the remote execution foundation.
- Provider availability never becomes implicit provider authority or fallback permission.
- Stale revision, generation, assignment-set or readback evidence fails closed.
- Unknown native mutation outcome is reconciled; it is not blindly retried.
- Secrets, cookies, CSRF values, Authorization headers, provider tokens and secret-bearing process environments are never normal output or repository content.
- No manual SQLite inspection is an acceptance requirement.

## Development rules

- Verify current `main`, exact PR head and final-head CI before writes or status claims.
- Keep all active Timer stack PRs Draft unless the user explicitly authorizes a state transition.
- Do not merge, rebase, force-push, rewrite published history or enable auto-merge without explicit approval.
- Use the smallest evidence-backed implementation slice and no unrelated refactors.
- Runtime acceptance is required when an installed runtime path or native VDR behaviour changes; contract/test-only slices do not invent a runtime acceptance requirement.
- A broad Timer UI must not bypass the account/backend access-management gate.

### Preferred edit path for new chats

Prefer direct GitHub repository operations when the connected tools can safely perform the bounded change. Use the local yaVDR checkout only for compilation, generated artifacts, focused local tests, installed-runtime facts or operations the connector cannot safely perform.

Never replace a repository file from a truncated fetch. Recheck the remote branch before writes and inspect the resulting commit/diff before treating the change as complete.

## Documentation synchronization note

The direct status entry points are:

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- this file.

The current-position sections in [Strict Roadmap](../planning/roadmap.md) and [Phase Map](../planning/phase-map.md) still lag the active Phase-64 stacked Drafts. Their phase-order and architecture contracts remain useful; their stale active-slice markers must not override exact GitHub state. A broader guarded planning-document synchronization remains separate.

## Exact next action

1. Treat PR #168 exact head `a49a3b99167e8f1bbbbf220657d774aaf4038501` and CI #7412 as the latest completed Slice-16 Draft checkpoint unless GitHub reports a newer head.
2. Start the next bounded Phase-64 slice from the exact current #168 head only after re-reading the stack.
3. Implement NativeTimerBinding absence application without native mutation, external-delete guessing or failover execution.
4. Preserve the stacked-Draft and explicit-approval boundaries.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Phase 64 TimerIntent Contract](phase-64-timer-intent-contract.md)
- [Phase 64 TimerIntent Repository](phase-64-timer-intent-repository.md)
- [Phase 64 TimerAssignment Contract](phase-64-timer-assignment-contract.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [ADR-0044 Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [Agent Workflow Rules](../../AGENTS.md)
