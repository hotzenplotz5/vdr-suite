# Post-Phase-66 Native Recording Editing — Workstream Slices

## Status and classification

**Active bounded post-Phase-66 workstream. Phase 66 remains completed. Phase 67 remains not started and requires its own explicit kickoff.**

This document decomposes the accepted [ADR-0059](../adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md) into coherent implementation and acceptance boundaries.

These are **workstream-local slices**, not strict-roadmap phase numbers. They must not be renamed to `66.9`, `66.x`, `67.x` or another invented numbered runtime phase.

The authoritative architecture/scope audit remains [Post-Phase-66 Native Recording Editing](post-phase66-native-recording-editing.md).

## Sequencing

```text
Slice 1 - Native Marks Read Model
  -> Slice 2 - Safe Native Marks Mutation
  -> Slice 3 - Native VDR Cut Execution
  -> Slice 4 - Recording Editing UX
  -> Slice 5 - OSD/MARKAD Coexistence and Real-System Acceptance
```

A slice is complete only when its coherent contract, automated regression evidence and any required real-system evidence are complete. A green intermediate commit does not automatically complete the next slice.

---

## Slice 1 — Native Marks Read Model

Status: **Active / partially implemented. Mutation remains closed.**

Goal: establish one typed, bounded, fail-closed read path from VDR native marks through SuiteBridge and the Backend Agent into VDR-Suite, then expose it through the authenticated Recording detail boundary without granting filesystem or mutation authority.

Required scope:

- SuiteBridge resolves an opaque Recording key against current VDR Recordings;
- native `cMarks` state is projected as bounded structured JSON;
- exact native frame positions remain semantic truth;
- VDR timecode and display seconds are derived values;
- marks state distinguishes no marks, canonical marks, missing Recording and unreadable/invalid native state;
- `cRecording::IsInUse()` facts are exposed truthfully for later safety policy;
- one opaque deterministic `marksRevision` represents canonical marks-editing state;
- Agent transport sends only the typed read command and rejects invalid keys before network dispatch;
- Core resolver strictly validates schema, identity, bounds, ordering and derived values;
- SuiteBridge discovery advertises a dedicated read capability only when implemented;
- daemon/runtime wiring owns one typed marks resolver for the current Backend Agent transport;
- authenticated Recording-detail API resolves the current Recording/backend identity before native read;
- no public caller supplies an absolute VDR Recording path;
- Recordings 2 can display the read model without introducing editing controls that imply mutation support.

Current evidence retained for this slice:

```text
published_suitebridge_candidate=237eff5c33d890730343036d5e125ba72ec16142
hosted_ci=VDR-Suite CI #8624 / run 33847607940 / PASS
real_yavdr_contract_build=PASS
real_yavdr_checked_candidate=237eff5c33d890730343036d5e125ba72ec16142
```

That evidence proves only the published SuiteBridge read-contract substep. Slice 1 remains open until the complete authenticated end-to-end read path, regression tests and required real runtime readback are proven on one exact candidate.

Exit gate:

- all read-contract tests pass;
- invalid/oversized/identity-mismatched payloads fail closed;
- Agent/Core/runtime/API route is wired and tested;
- no mutation command is reachable;
- real yaVDR readback shows existing native marks through the shipped candidate;
- normal Recording listing/details/playback regressions remain green.

---

## Slice 2 — Safe Native Marks Mutation

Status: **Not started.**

Goal: permit bounded add/delete/move/reset/replace operations against the same native VDR marks authority while preserving optimistic concurrency, protected-write fencing and authoritative readback.

Required scope:

- typed Recording-marks mutation schema/capability distinct from read capability;
- authenticated permission and backend read-only policy enforcement;
- current backend generation, Agent/provider identity/generation and ownership fences;
- durable operation + idempotency/request-fingerprint semantics;
- mandatory exact expected `marksRevision`;
- immediate pre-dispatch native read/revision comparison;
- in-use Recording rejection in the first implementation;
- VDR-owned mark add/delete/move/reset/replace semantics;
- native alignment/sort rather than browser-defined final position;
- exact authoritative post-mutation readback and new revision;
- stale OSD/MARKAD/Suite revision conflict rejects with no effect;
- possible-dispatch uncertainty never becomes blind mutation retry.

Exit gate:

- add/delete/move/reset/replace contract tests pass;
- stale revision and wrong identity/generation/provider tests prove no effect;
- duplicate delivery is idempotent or safely returns existing operation state;
- real yaVDR Suite-created mark is visible in normal VDR replay/OSD;
- real yaVDR OSD-created/changed mark is visible to VDR-Suite;
- no parallel marks persistence exists.

---

## Slice 3 — Native VDR Cut Execution

Status: **Not started.**

Goal: start and reconcile native cutting through VDR's `RecordingsHandler` using already established marks/revision safety.

Required scope:

- explicit cut preview/precondition evaluation;
- current Recording identity and protected backend/Agent/provider fences;
- expected `marksRevision` revalidation immediately before dispatch;
- in-use Recording rejection;
- VDR-native usable-sequence validation;
- conflict/collision check for existing Recording-handler work/result destination;
- dispatch through `RecordingsHandler.Add(ruCut, recording->FileName())`;
- no direct `cCutter`, ffmpeg, shell or filesystem substitute;
- durable starting-before-dispatch boundary;
- duplicate-start protection;
- transport uncertainty classified as `outcome_unknown`/reconciliation-owned;
- truthful pending/active/final state only where native evidence supports it;
- edited-result discovery from current VDR Recording state;
- original Recording remains untouched.

Exit gate:

- no-marks, invalid-sequence and in-use cases reject before dispatch;
- duplicate/replay behavior cannot create a second native cut;
- possible-dispatch timeout cannot cause blind redispatch;
- authoritative result discovery is tested;
- real yaVDR dedicated test Recording produces one native edited Recording and retains the original.

---

## Slice 4 — Recording Editing UX

Status: **Not started.**

Goal: expose the completed server-side read/mutation/cut capabilities inside the existing Recordings 2 detail flow without creating a second playback/timeline owner.

Required scope:

- compact `Schnitt / Schnittmarken` section;
- exact native mark list/timecodes;
- clear alternating keep/remove sequence visualization;
- add mark from the current canonical Recording playback position;
- jump to mark through the existing persistent playback owner/seek semantics;
- delete/move/reset controls bound to current revision;
- refresh/retry UX for revision conflicts rather than silent overwrite;
- explicit cut preview/confirmation/start;
- busy/in-use/permission/capability/unknown-outcome state shown truthfully;
- no direct SuiteBridge, SVDRP, provider or filesystem call from browser code;
- no second player, MediaSession owner or private timeline.

Exit gate:

- frontend regression tests prove one playback owner;
- current-position and jump-to-mark use canonical playback state;
- controls are hidden/disabled only as UX in addition to server-side enforcement, never instead of it;
- desktop/mobile Recording detail remains usable;
- real browser acceptance covers mark edit and cut confirmation against the shipped server candidate.

---

## Slice 5 — OSD/MARKAD Coexistence and Real-System Acceptance

Status: **Not started.**

Goal: prove the complete workstream on the production yaVDR target, including native OSD coexistence and optional MARKAD behavior where the installed version can be safely established.

Mandatory real-system acceptance:

1. record exact VDR version/API, candidate commit, SuiteBridge object hash and daemon/Agent candidate identity;
2. select a dedicated non-critical test Recording;
3. show existing native marks identically through VDR-Suite;
4. create/edit a mark in VDR-Suite and verify normal VDR replay/OSD sees it;
5. create/edit a mark in normal VDR and verify VDR-Suite sees it after refresh;
6. prove stale `marksRevision` blocks a conflicting Suite mutation;
7. start one native cut from VDR-Suite;
8. verify original Recording remains intact;
9. verify the edited Recording is registered/named according to native VDR behavior;
10. verify no VDR/daemon/Agent crash and no Recording corruption;
11. inspect installed MARKAD presence/version and record supported behavior as PASS or N/A without enabling irreversible auto-cut.

Workstream completion gate:

- all five slice contracts are implemented;
- complete hosted CI is green on one exact final candidate;
- packaging/install staging is green;
- exact yaVDR native build/runtime acceptance is PASS;
- durable acceptance evidence is recorded;
- only then may volatile status/closeout documents be updated and the Draft PR be considered for Ready-for-review state;
- PR merge remains a separate human action and is never performed by this workstream automation.

---

## Deferred/non-goals

The workstream intentionally does not include:

- Phase 67 Teletext/HbbTV;
- Phase 68 Legacy OSD compatibility bridge work;
- Phase 69 final public API hardening/versioning;
- a generic video editor;
- active/growing Recording editing;
- proprietary marks persistence;
- automatic deletion/replacement of the original Recording;
- irreversible MARKAD auto-cut;
- fabricated cut percentage/progress;
- direct browser/provider/SVDRP/filesystem authority.
