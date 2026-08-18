# VDR-Suite Current Project Status

## Status ownership

Exact operational phase state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context and must not become a second source for active branch heads, PR tips, transient CI run numbers or exact live `main` SHAs.

Before continuing work, always read `CURRENT.md`, the Strict Roadmap, the applicable ADRs and live GitHub state.

## Platform position

Latest completed numbered runtime phase: **Phase 64 - Timer Intent and Multi-Backend Orchestration**.

Current active numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.

Completed Phase-65 vertical: **65.A - Existing-Recording playback**.

Next Phase-65 product vertical: **65.B - Live-TV playback**.

Historical completed context remains relevant, including Phase 58 - Frontend and Live Parity, Phase 61 - Suite Metadata and Genre Platform, Phase 62 - Identity, RBAC and Accountability Foundation, and Phase 63 - Backend Agent and Secure Multi-Site Runtime.

## Phase 63 foundation

Phase 63 is complete. It established Agent enrollment/identity, protected transport, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit local-provider ownership/selection and the generic protected-write safety contract.

That foundation remains authoritative beneath completed Phase-64 orchestration and active Phase-65 media execution.

## Phase 64 completion

Phase 64 is complete and separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The completed engine provides durable Timer intent/assignment/binding identities, deterministic backend ownership, managed native Timer create/update/toggle/delete fulfillment, durable no-blind-retry semantics, authoritative readback/reconciliation and controlled reassignment/failover.

The final reassignment/failover block allows replacement only before native dispatch or after exact authoritative absence. It atomically supersedes the old exclusive owner, creates the replacement with a new assignment identity/epoch, persists durable handover evidence and rechecks current candidate authority. Stale or ambiguous evidence fails closed.

The exact completion candidate, CI and real yaVDR evidence are recorded in [Phase 64 Closeout](phase-64-closeout.md). Exact live `main` must be read from GitHub rather than copied into narrative status documents.

## Timer mutation safety position

The Phase-64 completion retains the protected-write rules established across Phases 62-64:

- actor/backend authorization and backend write mode remain authoritative;
- backend generation and provider instance/generation are fenced;
- assignment, binding, operation and expected-state fingerprints are checked where applicable;
- a possible native dispatch is durably represented before completion can be claimed;
- ambiguous delivery is `outcome_unknown` and reconciliation-only;
- CREATE/UPDATE/TOGGLE require authoritative PRESENT readback;
- DELETE requires complete-inventory authoritative ABSENCE readback;
- reassignment cannot use `dispatching`, `executed_unverified`, `outcome_unknown`, incomplete inventory or ambiguous/external drift as replacement authority;
- public SuiteBridge SVDRP help remains closed for private Timer write commands.

## Revised forward ordering

The strict numbered order is:

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [ACTIVE]
  -> Phase 66 Broadcast Companion Services: Teletext and HbbTV
  -> Phase 67 Legacy OSD Compatibility Bridge
  -> Phase 68 Public API and Client Compatibility Hardening
  -> Phase 69 Recommendation and Content Knowledge Graph
```

Completed history through Phase 64 is unchanged. Accepted ADR-0054 defines the Teletext/HbbTV architecture for Phase 66, but Phase 66 remains runtime-blocked until Phase 65 is complete and a separate Phase-66 kickoff is explicit.

## Active Phase 65 streaming architecture

Accepted ADR-0046 defines the server-side Streaming Gateway and MediaSession boundary. Accepted ADR-0053 defines the complementary client-playback/media-adaptation direction.

The active media direction is provider-private and transformation-minimal:

```text
private source
  -> explicitly owned provider
  -> ProviderStreamLease
  -> pass-through / remux / transcode as required
  -> Streaming Gateway / MediaSession
  -> client adapter
  -> platform playback engine
```

The product order remains Recording playback first, then Live TV, then truthful seek/growing-Recording behavior. Remux and transcode are evidence-driven compatibility escalation, not default architecture.

## Phase 65.A Recording playback completion

The existing-Recording playback vertical is accepted and closed for its bounded browser product scope.

Accepted capabilities include:

- authenticated/authorized MediaSession creation from stable Suite Recording identity;
- private local Recording source resolution without leaking provider-native paths to the browser;
- source probing and capability-based least-transformation selection;
- H.264 progressive copy paths;
- interlaced H.264 deinterlace/transcode handling;
- HEVC/Main10 UHD adaptation to browser-compatible H.264;
- AAC, AC3 and DTS audio handling with AAC stereo fallback where required;
- fMP4/HLS delivery through the Suite Gateway;
- startup/rebuffer forward buffering and stable four-second publication cadence;
- calibrated x264 workload policy;
- calibrated VAAPI UHD hardware transcoding with fail-closed UHD auto selection when no measured path reaches the 1.25x minimum;
- graceful `pagehide`, stop, ended and error cleanup;
- server-owned cleanup after an ungraceful client disappearance using the existing 300-second MediaAccessGrant idle boundary;
- deterministic worker/workspace and MediaSession/Route/Lease/Grant terminal cleanup;
- real browser picture + sound acceptance including mobile/VPN-sensitive cases.

The hard-disconnect acceptance proved that active playback is not reaped while access continues, and that an idle client disappearance ends the worker with terminal reason `media_access_idle_expired` and removes the workspace.

See [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md) for the durable evidence summary.

## Phase 65.B next direction

The next authorized product vertical is Live-TV playback:

```text
channel / EPG selection
  -> authorized MediaSession
  -> explicit live StreamProvider
  -> ProviderStreamLease
  -> Streaming Gateway
  -> first-party browser playback
  -> picture + sound
  -> channel change
  -> old route / receiver / lease released
```

Live TV must reuse the public MediaSession/Gateway boundary rather than expose Streamdev or other provider URLs directly to the browser. Channel replacement must be explicit and old provider/tuner resources must be released deterministically.

General seek/growing-Recording behavior remains later Phase-65 work.

## Broadcast Companion direction

Teletext and HbbTV are explicit planned television-domain capabilities rather than accidental Legacy-OSD backlog.

- Teletext is modeled as service/page/subpage data, not primarily as an OSD screenshot.
- HbbTV is modeled as broadcast application discovery plus an authorized isolated application session/runtime, not as raw URL/JavaScript/key plugin control.
- Phase-65 MediaSession rules remain authoritative when Suite-owned media is involved.
- Legacy OSD follows later for genuinely opaque native/plugin workflows.

## Broad Timer UI ordering

A broad polished Timer UI is not required to close the Phase-64 engine and is not inserted as a numbered phase.

It is a cross-cutting product milestone gated on:

```text
Phase 62 identity/RBAC [DONE]
+ Phase 64 Timer engine [DONE]
+ required account/backend access administration [OPEN]
```

The product UI must remain intent-first and preserve assignment, readback, reconciliation, failover and unknown-outcome semantics.

Phase 65 intentionally proceeds before the broad Timer UI is complete.

## Product acceptance

Component tests, CI, architecture guards and real-system safety checks remain mandatory. User-visible milestones additionally use [Golden User Journeys](../planning/golden-user-journeys.md).

The key vertical journeys cover:

- Live TV playback;
- Recording playback;
- record-one-programme orchestration;
- multi-backend scheduling without provider knowledge;
- fail-closed recovery;
- Teletext browsing;
- HbbTV application launch;
- one explicit Legacy OSD compatibility workflow;
- broad Timer UI operation as a cross-cutting milestone.

## Development rules

- Root-level `AGENTS.md` is binding.
- `CURRENT.md` is the sole repository copy of volatile operational phase status.
- Verify live `main`, exact PR head and exact-final-head CI before writes or status claims.
- Keep review/merge/retarget/close state changes behind explicit user approval.
- A slice is the smallest coherent safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessary long dependency stacks unless a real safety, concurrency, compatibility or acceptance boundary requires them.
- Provider availability never creates execution authority and active work never silently switches provider.
- Require real-system acceptance when an installed/runtime, native, media or broadcast-behaviour boundary changes.
- Broad Timer UI work must not bypass the account/backend access-management gate.
- Proposed ADR work does not authorize runtime implementation.
- Phase 65 is active; Phase 66 runtime remains blocked until Phase 65 closes and Phase 66 is explicitly started.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can perform the complete bounded edit safely. Read the complete file content required for the change, write a coherent commit on the intended branch and inspect the resulting diff before treating the update as correct.

Use local edits first only when the change requires:

- local build/test execution that cannot be represented by the connector;
- multi-file transformations that are materially safer in a checked-out worktree;
- binary/generated-file handling unavailable through the connector; or
- a workaround because the GitHub connector blocks a file operation.

GitHub-first does not weaken review safety: keep updates fast-forward-only, do not replace a complete file from a truncated fetch, and do not mark Draft PRs Ready or merge them without explicit approval.

## Current authorization boundary

Phase 65.A Recording playback is closed. The next authorized runtime change is Phase 65.B Live-TV playback:

1. resolve stable Suite Channel identity to an explicitly owned private live provider;
2. create authorized MediaSession/Route/Lease/Grant state without exposing provider URLs;
3. deliver live picture and sound through the Suite Gateway to the first-party browser;
4. keep provider/tuner ownership explicit and bounded;
5. make channel replacement explicit and release old route/provider/tuner resources deterministically;
6. preserve disconnect/revoke/expiry cleanup semantics;
7. pass exact-head CI and real yaVDR acceptance.

General seek/growing-Recording work follows according to the Strict Roadmap. Phase 66 remains out of scope.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Agent Workflow Rules](../../AGENTS.md)
