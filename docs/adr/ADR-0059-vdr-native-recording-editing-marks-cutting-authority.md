# ADR-0059: VDR-Native Recording Editing, Marks and Cutting Authority

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Post-Phase-66 Native Recording Editing](../development/post-phase66-native-recording-editing.md)
- [Post-Phase-66 Native Recording Editing Slices](../development/post-phase66-native-recording-editing-slices.md)
- [ADR-0014: Recording Identity Strategy](ADR-0014-recording-identity-strategy.md)
- [ADR-0024: Recording Action Transport Mapping](ADR-0024-recording-action-transport-mapping.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0056: Playback Presentation, Timeline, Continuity and Failure Semantics](ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)

---

## Status

**Accepted**

Date: 2026-09-04

This ADR establishes the stable authority and safety model for native VDR Recording marks and cutting. It authorizes only the bounded post-Phase-66 Recording-editing workstream described by the linked development documents. It does **not** reopen Phase 66, start Phase 67, renumber the strict roadmap or authorize unrelated Recording mutations.

---

## Context

VDR-Suite already has a mature Recording domain, authenticated frontend, Backend Agent execution boundary, provider/generation fencing, durable mutation semantics and a persistent Recording playback owner. It also already models `RecordingActionType::Cut`, `recording.action.cut` and `recording.permission.cut`.

The missing capability is not a new generic `CUT` enum. The missing capability is a trustworthy native owner path for:

- reading VDR Recording marks;
- editing those marks without creating a competing source of truth;
- preserving interoperability with normal VDR replay/OSD behavior;
- starting native VDR cutting through the VDR-owned Recording handler;
- observing the resulting native Recording state without inventing success or progress;
- routing every real mutation through the existing protected Control Plane -> Agent -> SuiteBridge boundary.

VDR itself already owns the relevant authoritative state and behavior:

```text
Recording
  -> cMarks / cMark
  -> native marks or marks.vdr file
  -> cRecording::IsInUse()
  -> RecordingsHandler
  -> native edited Recording
```

A naive implementation could instead put marks in the Suite database, expose Recording paths to the browser, write `marks` from HTTP code, invoke an external cutter, construct `cCutter` directly or rely on a transport acknowledgement as success. Each of those choices would create a second authority, bypass existing fencing or diverge from normal VDR behavior.

The architecture therefore needs one explicit decision before real marks mutation or cut dispatch is enabled.

---

## Decision

VDR remains the **canonical and exclusive authority** for native Recording marks and native cut lifecycle.

VDR-Suite provides a protected domain/API projection and orchestration layer over that authority. It does not replace it.

The canonical topology is:

```text
Recordings 2 / canonical Recording playback owner
  -> authenticated VDR-Suite Recording editing API
  -> Control Plane authorization and Recording identity
  -> revision / idempotency / durable operation boundary
  -> current Backend Agent and explicitly owned SuiteBridge provider
  -> typed native Recording editing command/read contract
  -> SuiteBridge inside the VDR process
  -> cRecording / cMarks / RecordingsHandler
  -> authoritative native readback
```

The topology is deliberately asymmetric:

- VDR owns native marks, index normalization, in-use state and cutting;
- SuiteBridge owns only bounded VDR-process-local translation and execution;
- the Agent owns local provider/generation/dispatch fencing;
- the Control Plane owns authorization, resource identity, durable operation state, idempotency, outcome classification and reconciliation;
- the frontend owns presentation and user intent only.

No layer below the Control Plane may acquire Recording-domain authority merely because it can reach VDR.

---

# Authority and identity rules

## 1. Native marks are the only marks authority

VDR-Suite must not create a proprietary or shadow marks database.

For supported VDR recordings the canonical state is the VDR-owned native marks representation associated with the Recording. VDR-Suite may cache a response transiently for rendering, but such a cache is never mutation authority and must not survive as an independent editable truth.

External native changes, including normal VDR replay/OSD changes and supported MARKAD changes, become visible through the same native read path.

## 2. Public clients never receive filesystem authority

The public/client identity remains a VDR-Suite Recording identity scoped by backend identity and authorization.

An absolute VDR Recording filename may exist as an internal backend-native identity, but clients must not be able to submit an arbitrary filesystem path and thereby select a mutation target.

SuiteBridge receives only a bounded typed identity token/opaque Recording key. It resolves that token against VDR's current Recording inventory before every native read or mutation.

Zero or multiple native matches fail closed.

## 3. Native frame position is semantic truth

The authoritative mark position is the native Recording frame index.

Timecode and seconds are presentation values derived from native Recording FPS/index semantics. A requested browser/playback second is an intent position, not proof that the final mark exists at that exact decimal second.

Native VDR alignment/normalization owns the final position. Successful mutation responses return authoritative post-normalization frame positions.

---

# Read model

The read path is intentionally safe enough to exist before mutation support.

For one resolved Recording the native marks read model exposes only bounded structured state, including:

- opaque Recording identity/key confirmation;
- native marks availability/state;
- frames per second and PES/TS facts required to interpret marks;
- in-use flags;
- ordered exact native mark frame positions;
- VDR-formatted timecodes and derived display seconds;
- optional mark comments;
- native sequence count where VDR can establish it;
- one opaque canonical `marksRevision` derived from the current marks state.

Malformed, oversized, identity-mismatched or semantically inconsistent native payloads are rejected. Read failure is not silently converted into an empty marks set.

The read contract must not expose arbitrary file contents or caller-controlled native paths.

---

# Mutation safety

## 1. ADR-0042 applies without exception

Every production marks mutation and cut-start operation is a real Recording mutation under ADR-0042.

It therefore requires the applicable protected-write envelope, including:

- authenticated/authorized actor or trusted caller;
- backend identity and active backend generation;
- current Agent/provider ownership and generation fences;
- stable Recording resource identity;
- expected current revision;
- durable operation identity;
- idempotency key / normalized request fingerprint;
- durable starting-before-dispatch semantics;
- authoritative readback or reconciliation;
- explicit `outcome_unknown` handling when dispatch may have occurred.

No browser-to-SuiteBridge shortcut and no best-effort direct native write is allowed.

## 2. Marks revision is optimistic concurrency authority

Every marks mutation carries the exact opaque `marksRevision` observed by the caller.

Immediately before native mutation, the owner re-reads canonical native marks state and rejects a revision mismatch without effect.

This protects the shared-authority race:

```text
Suite reads marks A
VDR OSD or MARKAD changes marks -> B
stale Suite mutation based on A
```

The stale Suite request must not overwrite B.

The marks revision represents marks-editing concurrency truth. Transient in-use state is a separate dispatch-time safety predicate and is not required to change the marks revision merely because playback starts or stops.

## 3. In-use mutations fail closed initially

The first production implementation rejects marks mutation and cut start when VDR reports the Recording in use.

This includes the VDR-native usage facts relevant to recording, replay and Recording-handler activity.

Supporting edits of growing/current recordings or concurrent replay is a separate future design problem and is not inferred from ordinary completed-Recording behavior.

## 4. Native write semantics stay inside VDR

SuiteBridge uses VDR-owned APIs and native Recording objects to apply marks changes. HTTP code, frontend code and Agent code do not open/unlink/write `marks` or `marks.vdr` directly.

Native add/move/replace operations permit VDR to align/sort positions using the Recording index semantics. Native delete/reset uses VDR-owned marks semantics rather than caller-selected filesystem deletion.

A successful transport acknowledgement is insufficient. The result must include or be followed by authoritative native readback showing the new canonical marks state and revision.

---

# Native cutting

## 1. RecordingsHandler is the cut lifecycle owner

VDR-Suite starts native cutting through the VDR Recording handler:

```text
RecordingsHandler.Add(ruCut, recording->FileName())
```

VDR-Suite does not construct `cCutter` as an alternate lifecycle owner and does not substitute ffmpeg, shell scripts or filesystem move/copy/delete behavior for native VDR cutting.

The handler remains responsible for native queueing/serialization, destination naming, result registration and incomplete-result cleanup according to the target VDR version.

## 2. Cut start has explicit prerequisites

Immediately before dispatch, the protected owner must establish at least:

- the Recording still resolves uniquely under the expected identity;
- backend/Agent/provider generations and ownership remain current;
- backend policy permits mutation and the actor has cut permission;
- expected native marks revision still matches;
- the Recording is not in use;
- canonical marks exist and form a usable native cut sequence;
- VDR does not already own a conflicting source/destination Recording-handler operation;
- destination/result collision policy is satisfied;
- required native capability is available.

Failure of any prerequisite blocks dispatch.

## 3. Original Recording is retained

Native cutting produces a new VDR Recording. VDR-Suite does not automatically delete, replace or rename the original after a successful cut.

Any later original-deletion workflow remains a separate protected mutation and is not implied by `Cut`.

## 4. Acknowledgement is not completion

A native cut-start acknowledgement proves only that the start request reached an accepted native boundary.

VDR-Suite reports only status that VDR can establish truthfully. It must not invent percentage progress.

If transport fails after dispatch may have begun, the durable operation becomes `outcome_unknown` or equivalent reconciliation-owned state. No blind retry is allowed until authoritative native evidence determines whether the first start took effect.

Edited-result discovery must use current VDR Recording state and native identity/naming semantics rather than guessing a path from the browser.

---

# VDR OSD and MARKAD coexistence

Normal VDR replay/OSD and VDR-Suite intentionally operate on the same canonical native marks authority.

Interoperability requirement:

```text
VDR OSD mutation
  -> native VDR marks
  -> next VDR-Suite read reflects it

VDR-Suite mutation
  -> native VDR marks
  -> normal VDR replay/OSD reflects it
```

MARKAD is optional and is never a second authority.

If an installed/supported MARKAD version is integrated, the preferred product flow is:

```text
explicit automatic mark detection
  -> MARKAD/native VDR marks
  -> normal VDR-Suite readback
  -> user review/edit
  -> separate explicit native VDR cut
```

No irreversible MARKAD auto-cut is introduced by this workstream. Before any manual MARKAD invocation is exposed, the exact installed version, overwrite/merge behavior and safe invocation semantics must be verified on the real target host.

Native manual marks and native cut remain functional when MARKAD is absent.

---

# Frontend ownership

Recording editing belongs inside the existing Recordings 2 detail flow and reuses the established persistent Recording playback owner.

The frontend may:

- display native marks and alternating cut sections;
- add a mark from the current canonical playback position;
- jump to a mark using the existing seek owner;
- request bounded delete/move/reset operations;
- request an explicit cut preview/confirmation/start;
- refresh after revision conflict or native state change.

It must not create:

- a second media player;
- a second MediaSession owner;
- a private competing Recording timeline;
- a direct SVDRP/SuiteBridge connection;
- filesystem authority;
- client-side generation or revision bypasses.

---

# Workstream slicing and sequencing

This architecture is implemented as a bounded **Post-Phase-66 Native Recording Editing** workstream before any separate Phase-67 kickoff.

The slices are workstream-local implementation/acceptance boundaries. They are deliberately **not** roadmap phase numbers and are not named `66.9`, `66.x` or `67.x`.

Required order:

```text
Slice 1 - Native Marks Read Model
  -> Slice 2 - Safe Native Marks Mutation
  -> Slice 3 - Native VDR Cut Execution
  -> Slice 4 - Recording Editing UX
  -> Slice 5 - OSD/MARKAD Coexistence and Real-System Acceptance
```

The detailed gates are owned by the linked slice document.

Hard sequencing rules:

- Slice 1 stays read-only.
- No production marks mutation is enabled before Slice 1 establishes the typed end-to-end read/revision contract.
- No native cut dispatch is enabled before safe marks mutation/revision behavior is proven.
- The frontend does not bypass unfinished server-side ownership merely to expose controls early.
- Real-system acceptance is performed on the exact final candidate before the workstream is declared complete.
- Phase 67 remains `NOT STARTED` until its independent explicit kickoff regardless of this workstream's progress.

---

# Consequences

Positive:

- one canonical marks authority remains visible to VDR, VDR-Suite and supported native tools;
- normal VDR editing semantics are preserved rather than reimplemented in browser/server code;
- stale OSD/Suite/MARKAD races become explicit optimistic-concurrency conflicts;
- native cutting remains owned by VDR's Recording lifecycle;
- remote/multi-site mutations reuse established Agent/provider/generation fences;
- unknown outcomes can be reconciled without duplicate cuts;
- the browser remains path- and provider-agnostic;
- future first-party clients can reuse the same domain contract.

Trade-offs:

- native editing depends on a compatible SuiteBridge/VDR build rather than RESTfulAPI alone;
- read and write models must preserve exact native frame semantics in addition to user-friendly seconds/timecodes;
- conservative in-use rejection limits concurrent editing in the first implementation;
- durable operation/readback logic makes cut start more involved than a one-shot native call;
- MARKAD integration cannot be generalized until the real installed version is audited.

---

# Non-goals

This ADR does not define or authorize:

- Phase 67 Teletext/HbbTV runtime work;
- reopening or renumbering Phase 66;
- a generic nonlinear video editor;
- arbitrary browser/server filesystem access;
- a proprietary marks store;
- direct `cCutter` lifecycle ownership when `RecordingsHandler` owns cutting;
- automatic original deletion/replacement;
- irreversible automatic MARKAD cutting;
- fabricated native cut progress;
- active/growing Recording editing;
- final Phase-69 public API compatibility/versioning work;
- changing existing RESTfulAPI cut support by inventing a transport that the backend does not own.

---

## Related decisions

- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0014: Recording Identity Strategy](ADR-0014-recording-identity-strategy.md)
- [ADR-0024: Recording Action Transport Mapping](ADR-0024-recording-action-transport-mapping.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0049: Audit and Security Event Model](ADR-0049-audit-security-event-model.md)
- [ADR-0056: Playback Presentation, Timeline, Continuity and Failure Semantics](ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Native Recording Editing](../development/post-phase66-native-recording-editing.md)
- [Back to Strict Roadmap](../planning/roadmap.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
