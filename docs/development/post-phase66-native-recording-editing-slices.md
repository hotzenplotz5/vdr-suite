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
       -> Slice 2A - Native Marks Read Authority
       -> Slice 2B - Safe Mutation Dispatch
       -> Slice 2C - Verification & Idempotency
  -> Slice 3 - Native VDR Cut Execution
  -> Slice 4 - Recording Editing UX
  -> Slice 5 - OSD/MARKAD Coexistence and Real-System Acceptance
```

The 2A/2B/2C labels are an **internal decomposition of Slice 2 only**. They do not create new roadmap phases and do not authorize Slice 3 work early.

A slice is complete only when its coherent contract, automated regression evidence and any required real-system evidence are complete. A green intermediate commit does not automatically complete the next slice.

---

## Slice 1 — Native Marks Read Model

Status: **Implemented as the read foundation; retained as a required regression boundary for Slice 2.**

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
- Recordings 2 can display the read model without introducing editing controls that imply unsupported authority.

Retained historical evidence:

```text
published_suitebridge_candidate=237eff5c33d890730343036d5e125ba72ec16142
hosted_ci=VDR-Suite CI #8624 / run 33847607940 / PASS
real_yavdr_contract_build=PASS
real_yavdr_checked_candidate=237eff5c33d890730343036d5e125ba72ec16142
```

The Slice-2 implementation and CI continue to execute the Slice-1 read-contract tests. Native marks reads remain authoritative and are not replaced by a mutation-side cache or parallel database.

---

## Slice 2 — Safe Native Marks Mutation

Status: **Active. Slice 2A and 2B implemented. Slice 2C implementation is in verification/final-candidate preparation; real yaVDR Slice-2C acceptance remains required. Slice 3 remains closed.**

Goal: permit bounded add/delete/move/reset/replace operations against the same native VDR marks authority while preserving optimistic concurrency, protected-write fencing, durable replay safety and authoritative readback.

### Internal Slice 2 decomposition

#### Slice 2A — Native Marks Read Authority

Implemented foundation retained by every mutation:

- VDR/SuiteBridge remains the sole canonical marks owner;
- public Recording identity is resolved to the current native Recording before read/mutation;
- fresh native marks state and exact `marksRevision` are available to the Control Plane;
- callers cannot supply filesystem path authority;
- malformed/unreadable/identity-mismatched native state fails closed.

#### Slice 2B — Safe Mutation Dispatch

Implemented dispatch boundary:

- typed `recording.marks.modify` command/capability and bounded Add/Delete/Move/Reset/Replace payloads;
- authenticated protected-write policy and backend read-only enforcement;
- backend generation, Agent instance, SuiteBridge provider ownership/generation/capability fences;
- durable assignment keyed by `operationId`, `operationRevision` and request fingerprint;
- exact `expectedMarksRevision` checked before mutation;
- fresh durable local `starting` state before possible native dispatch;
- possible-dispatch uncertainty persists as `outcome_unknown` / `reconcile_only` rather than blind retry;
- SuiteBridge performs only the bounded VDR-process-local marks mutation; no browser/filesystem/raw-SQLite/shell write path exists.

Retained real yaVDR Slice-2B build gate:

```text
GATE_RUNTIME_BUILD_MARKERS=PASS
CI_RUN=33976798597
NO_RUNTIME_INSTALL=true
NO_POST=true
NO_ADD=true
NO_DELETE=true
NO_MANUAL_NMARKS=true
NO_RAW_SQLITE_WRITE=true
NO_CUT=true
SLICE2B_CANDIDATE_BUILD=PASS
```

That gate proves buildability only. It is not Slice-2C runtime acceptance and did not execute a real marks mutation.

#### Slice 2C — Verification & Idempotency

Current required contract:

- command acceptance/transport success is never sufficient for mutation success;
- after successful native mutation SuiteBridge performs a fresh native marks read, verifies the intended normalized frame set and binds the exact resulting canonical `marksRevision` to the operation evidence;
- Agent completion still persists the possible-dispatch result as unverified `outcome_unknown` / `reconcile_only` evidence;
- daemon reconciliation performs another fresh canonical marks read and verifies **the exact bound post-mutation revision**, not merely "revision changed";
- an unrelated concurrent OSD/MARKAD revision change therefore cannot falsely verify a Suite mutation;
- a verified result is durable and an identical replay may return that persisted verified outcome without redispatch;
- same `operationId` with changed revision, Recording or mutation payload conflicts and fails closed;
- stale `expectedMarksRevision` never creates a new mutation; the replay probe may only return a previously known operation result;
- timeout, lost response and restart paths never turn possible dispatch into "write again and hope";
- malformed operation tokens fail before any dispatcher is reached;
- backend/Agent/provider/capability drift prevents verification or dispatch;
- no reconciliation path calls the mutation assignment/replay writer to redispatch work.

A defect was found while implementing 2C in the pre-2C behavior: reconciliation accepted any fresh canonical revision different from `expectedMarksRevision`. A concurrent native marks change could therefore have been mistaken for successful verification. That behavior is insufficient for Slice 2C and is the reason a new exact runtime candidate is required after final CI; the historical accepted SHA below is not silently redefined.

Historical real Acceptance reference retained unchanged:

```text
previous_real_acceptance_candidate=066028de07c406a7e5a96dba8260eac23d21ffa9
```

Hosted implementation evidence before the final documentation/candidate commit:

```text
slice2c_product_head=389fd46cdd500e7461065e62a4fa5938bf544c50
slice2c_guard_alignment_head=eceec6d0f5d7303e230a3b4052e7109d738c4d94
hosted_ci_run=33980296728
hosted_ci_result=PASS
jobs=docs-check,frontend-regression-test,fast-regression-test,make-test-audit,architecture-check,packaging-regression-test
```

The final real-acceptance candidate must be a later exact SHA containing this documentation plus the complete explicit regression set. It is frozen only after its own complete relevant CI is green.

### Slice 2 automated exit evidence

The final candidate must keep deterministic regressions green for at least:

- Add -> exact post-state verification PASS;
- Delete -> exact post-state verification PASS;
- Move -> exact post-state verification PASS;
- Reset -> exact post-state verification PASS;
- Replace -> exact post-state verification PASS;
- identical assignment replay -> same durable command/operation, no second mutation;
- same `operationId` with changed expected revision/Recording/payload -> conflict;
- stale `expectedMarksRevision` -> no fresh mutation;
- successful mutation with lost HTTP response -> retry returns/reconciles the already verified operation without a second dispatch;
- exact post-revision mismatch / unrelated revision drift -> no false success;
- `outcome_unknown` remains reconciliation-owned;
- provider/Agent/backend generation/capability fences;
- malformed/invalid operation tokens;
- Slice-2A read-path malformed/identity/capability failures;
- architecture guards for no raw SQLite, direct marks-file, shell or browser bypass;
- existing Slice-1/2A/2B contracts.

### Controlled real yaVDR Slice-2C acceptance

The real acceptance is marks-only. **Do not execute a Recording cut. Do not use manual `NMARKS`, direct marks-file writes, raw SQLite writes or shell-based marks mutation.**

Before any mutation:

1. checkout the frozen candidate and prove `git rev-parse HEAD` equals the exact announced SHA;
2. build the candidate and record hashes for daemon, Agent binaries and SuiteBridge object;
3. install only that verified build through the normal runtime/package path;
4. record VDR, SuiteBridge, daemon and Agent service/capability/provider-ownership state;
5. choose one non-critical, inactive test Recording;
6. read its marks through the public authenticated VDR-Suite marks API;
7. record the complete baseline canonical frame list and `marksRevision`.

Bounded mutation and verification:

1. issue one reversible marks mutation through the production authenticated API with a fresh `operationId`, explicit `operationRevision` and the baseline `expectedMarksRevision`;
2. retain the HTTP result, `commandId` and request fingerprint;
3. perform a fresh public marks GET and prove the exact expected normalized canonical state/revision is visible;
4. replay the **identical** mutation request and prove it returns the known/verified operation without a second native mutation;
5. perform another fresh marks GET and prove the revision/state did not change because of the replay;
6. send a request using the stale baseline revision and a fresh operation ID; prove HTTP conflict and no canonical state change;
7. send the original `operationId` with a changed payload/revision; prove operation conflict and no canonical state change;
8. where the controlled harness can suppress the first HTTP/Agent response after successful dispatch without changing the native request, retry the identical request and prove reconciliation returns the already reached canonical result instead of redispatching;
9. restore the baseline marks through the same authenticated API using the then-current revision;
10. perform a final fresh marks GET and prove the exact baseline frame list is restored.

Required acceptance evidence:

```text
candidate_sha=<exact frozen SHA>
ci_run=<complete green run>
build_hashes=<daemon/agent/tools/plugin hashes>
recording_id=<public test Recording id>
baseline_marks_revision=<revision>
mutated_marks_revision=<revision>
operation_id=<operation id>
operation_revision=<operation revision>
request_fingerprint=<durable fingerprint>
post_read_matches_expected=true
identical_replay_second_mutation=false
stale_revision_mutation=false
changed_payload_same_operation_mutation=false
lost_response_retry_second_mutation=false|not_exercised_with_reason
baseline_restored=true
NO_RAW_SQLITE_WRITE=true
NO_DIRECT_MARKS_FILE_WRITE=true
NO_MANUAL_NMARKS=true
NO_CUT=true
SLICE2C_REAL_ACCEPTANCE=PASS
```

Slice 2 remains **Active** until this real evidence is PASS on the exact frozen candidate. Only then may Slice 2 be marked completed and Slice 3 be considered for a separate kickoff.

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
