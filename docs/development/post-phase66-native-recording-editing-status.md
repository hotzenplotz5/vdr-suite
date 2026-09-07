# Post-Phase-66 Native Recording Editing — Current Status

This file is the compact current execution/evidence status for the bounded post-Phase-66 native Recording editing workstream. The architecture and detailed slice contracts remain owned by `post-phase66-native-recording-editing.md` and `post-phase66-native-recording-editing-slices.md`.

Phase 66 remains complete. Phase 67 has not started.

## Current installed candidate — 2026-09-07

Product head: `c500224d6601ccc3181996bbbb9db9cc9aaf4998`, branch
`work/post-phase66-native-recording-editing`; PR #268 remains Draft.
[VDR-Suite CI #8842, run 34159335352](https://github.com/hotzenplotz5/vdr-suite/actions/runs/34159335352)
passed all six jobs for this exact product head.

The backend-scoped Recording refresh queue now commits the cache before publishing
through the existing change feed and live transport. Polling recovery follows the
same ordering. Recordings 2 subscribes through the existing client API, coalesces
updates and retains its fallback refresh and lifecycle cleanup.

The native marks editor is integrated into the production browser-view bundle and
observes the canonical playback owner. It supports native marks readback and
revision-fenced editing, native cut preview and explicit confirmation, and
identical-operation reconciliation without blind retries. Focused tests exercise
the real browser-view composition, internal playback start, owner lifecycle,
revision conflicts, pending outcomes and mock-only cut confirmation.

Installation on yaVDR completed at 22:39 CEST using the previously built daemon and
`make install PREFIX=/usr`. Installed and candidate daemon SHA-256:
`acaabbfe8be52ebd1d6fd171a6f843bcaa793c207cad91af0ae0067f87ea4256`.
Installed production browser bundle matches the source concatenation, SHA-256:
`e654cf0fd264a1936f14151b3bbc742cefe1efc800d6134ace5ca6deaf8fe79a`.
Frontend syntax and nginx configuration checks passed. Daemon, Agent and VDR are
active; VDR and Agent retained their original PIDs. No new daemon journal warnings
were reported after startup. These facts establish deployment, not functional
browser acceptance.

**Installed functional acceptance remains pending.** The available integrated
browser has no authenticated session and rejects the local self-signed HTTPS
certificate. No authenticated live-feed latency or real editor lifecycle result
is claimed. A local mock visual fixture is not real yaVDR acceptance.

**User safety instruction supersedes the historical first-cut gate below.**
The user reports that the Brisant cut already occurred; source and edited-result
directories exist. No second real cut and no changes to either Brisant recording
are authorized. This session issued no real marks or cut mutation.
The 22:20-to-22:39 file inventory comparison shows unchanged sizes and mtimes for
all edited-result files and source video files; the source marks hash is unchanged
(`c41ded31f4a936a8c0bd58fe833e59516f783480226f9ed07da5af93df1b0dcd`).
Source `index` and `info` changed at 22:24:58, before this installation; their
cause is unverified. Consequently an entirely unchanged source cannot be claimed.
The historical Slice-3 evidence below is retained for context, not as permission
to repeat the cut. Phase 67 remains out of scope.

## Slice 1 — Native Marks Read Model

Status: **COMPLETE**.

The native RMARKS read model is retained as a mandatory regression boundary. VDR remains the canonical marks owner and callers do not receive filesystem authority.

## Slice 2 — Safe Native Marks Mutation

Status: **COMPLETE, including real yaVDR Slice-2C acceptance**.

Accepted real candidate:

```text
candidate_sha=b51b906becbaec8dfa72a5649e76327336eac2ea
SLICE2C_REAL_ACCEPTANCE=PASS
```

Final accepted native readback after the bounded replace/add acceptance sequence:

```text
marksRevision=4f7192f491f32966be2e9e8f9bc8b17e
frames=7500,15000,18750,22500
sequenceCount=2
inUseFlags=0
```

The real acceptance used the production protected Control Plane -> Agent -> SuiteBridge path. The plugin package was rebuilt/installed for that acceptance candidate and VDR/daemon/Agent health passed. No cut was executed as part of Slice 2.

## Slice 3 — Native VDR Cut Execution

Status: **AUTOMATED IMPLEMENTATION COMPLETE; REAL yaVDR CUT ACCEPTANCE PENDING**.

Automated implementation and acceptance-preflight head before this documentation commit:

```text
implementation_head=c5c7118dc252fb4eb564e0c92cedf38035f72be6
```

Complete hosted CI for that exact implementation head:

```text
run=34035354503
run_number=8795
result=PASS
jobs=architecture-check,fast-regression-test,make-test-audit,frontend-regression-test,packaging-regression-test,docs-check
```

The implemented Slice-3 boundary includes:

- explicit authenticated `GET /api/vdr/recordings/cut` native preview;
- protected `POST /api/vdr/recordings/cut` start path;
- public Recording identity resolved to the current opaque native Recording key;
- explicit `recording-cut-state` SuiteBridge discovery capability;
- current marks readability, exact `marksRevision`, mark/sequence count, in-use flags, handler usage and edited-result facts from RCUT;
- backend write policy, `recordings.cut` permission, authentication, browser CSRF, backend scope and read-only-role enforcement;
- explicit authorization-denied/allowed and protected-operation outcome accountability coverage for the cut route;
- Control Plane operation/idempotency assignment with active Agent lease, backend generation, Agent instance and SuiteBridge provider ownership/generation/capability fences;
- exact assignment replay for the same operation and fail-closed conflict for changed Recording or marks revision;
- stale `expectedMarksRevision` can only perform a replay probe and cannot create a new cut;
- backend write denial occurs before any cut dispatcher is reached;
- durable Agent local `starting` state before possible native dispatch;
- no-blind-retry recovery: possible dispatch becomes `outcome_unknown` / reconciliation-only;
- native VDR precondition re-read immediately before dispatch;
- rejection for missing/unusable marks, in-use Recording, handler conflict and existing edited destination;
- exactly one native enqueue authority: `RecordingsHandler.Add(ruCut, ...)`;
- no direct `cCutter`, ffmpeg, shell or browser/filesystem cutter path;
- native edited-result identity derived from `cCutter::EditedFileName(...)` and reconciled against current VDR Recording state;
- original Recording is never automatically deleted or replaced;
- complete Slice-3 API/Security/Agent/transport/protocol/state/reconciliation test edge is mandatory in `test-fast`;
- static architecture guard prevents bypass of the typed Control Plane/Agent/SuiteBridge owner path.

The real-system preflight is also implemented and guarded as strictly read-only. Before any controlled cut it proves the exact branch/head and clean worktree, byte-identical candidate versus installed daemon, Backend Agent and SuiteBridge plugin, records their hashes, verifies the read-only RCUT capability and the fenced NCUT provider capability, and checks public marks/cut preview readiness. The preflight contains no HTTP POST, no `NCUT EXEC`, no marks mutation and no service mutation.

## Historical real Slice-3 acceptance gate (superseded by current safety instruction)

The first real cut is intentionally still closed. No real Recording may be mutated by a cut until explicit user approval identifies/accepts a dedicated non-critical test Recording and the exact candidate has complete green hosted CI.

The controlled acceptance must then prove at least:

1. exact candidate/build/plugin identity;
2. read-only cut preview reports `ready=true` for the dedicated inactive test Recording and binds the expected marks revision;
3. one authenticated protected cut start is accepted through the production Control Plane -> Agent -> SuiteBridge path;
4. the Agent does not issue a blind second NCUT on retry/recovery;
5. VDR reports native handler activity while applicable;
6. one edited Recording is discovered through native VDR state and exact expected edited Recording identity;
7. the original Recording remains present and unchanged by Suite policy;
8. no VDR/daemon/Agent crash and no Recording corruption occurs;
9. acceptance evidence records source/result identities and final verification state.

Until that evidence is PASS, Slice 3 is not real-system complete and Slice 4 must not be represented as accepted server-side functionality.

## Historical Slice-3 implementation safety boundary

No real cut was executed while implementing, testing or documenting the Slice-3 automated boundary. Any first real `POST /api/vdr/recordings/cut` remains an explicit approval gate.
