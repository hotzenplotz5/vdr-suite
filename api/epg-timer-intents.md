# Public API v1 — one-shot EPG TimerIntent CREATE admission

## Current source contract in `main` (2026-10-22)

Consolidated backend [PR #500](https://github.com/hotzenplotz5/vdr-suite/pull/500) was **merged into `main`** as `ba083044ad661cbd8d979b5dc66b9eb3c3857f98` on 2026-10-21. `core/daemon/src/DaemonRuntimeInitialization.cpp` registers `EpgTimerIntentSubmit`; this production composition verifies fresh EPG identity/revision, independent backend write policy, Native VDR-local CLOCK attestation and conservative read-only `NTPREVIEW CHECK 1` before one atomic Intent + Assignment + durable Operation admission. The authenticated HTTP gate admits exactly scoped paired Devices and explicitly authorized administrator Browser Sessions (with CSRF); see below. Older standalone-branch and Draft-status statements below are **historical**, not statements about current `main`.

**Earlier read-only preview checkpoint (superseded by the hardware CREATE acceptance below):** On 2026-10-19 the installed yaVDR 2.8.3 / SuiteBridge 0.14.1 returned `900` for operation-specific native CREATE capability, real VDR-local `NTCREATE CLOCK 1`, and complete digest-bound `NTPREVIEW CHECK 1` for `Das Erste HD`, 2026-10-20 20:00–21:00 VDR-local. The valid preview was **`900 ... 0 2 isolated_clear`**: at that observation there were no existing Timers, two considered native devices, and a safely isolated one-shot candidate. This supersedes the earlier installed `CLOCK 501 unsupported` evidence. The preview is read-only, snapshot-bound, and **not** a durable CREATE or proof that the running Suite Daemon implements the consolidated API v1 source. The later real paired-Device/grant, EPG admission, dispatch and native readback acceptance is recorded below. Cross-process recovery and uncertain-outcome reconciliation remain separate. Never interpret HTTP 202 as a recorded programme; treat `outcome_unknown` as unresolved. VIDAA has a source-only explicit confirmation and deliberate Operation GET on `work/vidaa-epg-timer-operation-readback-20261017`; no Hisense acceptance has occurred.

## E2 source closeout assessment (2026-10-22)

- **FERTIG (source):** Production `main` has authoritative event/ETag checks, scoped Device and administrator Browser/CSRF authorization, independent backend write policy, VDR-local CLOCK and conservative preview attestation, transactionally durable TimerIntent/Assignment/Operation and idempotent replay, provider-fenced Agent dispatch and native readback completion. Public HTTP, security, atomic admission and productive-runtime regressions exist. On E1 merge head `ea7acfec7dd1b1f20aa49a479e3ca4a48bab0418`, all four GitHub workflows completed successfully, including [VDR-Suite CI](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39746706005). Later unrelated work has its own CI boundary.
- **REALTEST NACHGEWIESEN:** One bounded paired-Device/native one-shot CREATE described below: HTTP 202, Operation `succeeded`, native timer count 0 → 1, exact readback. The installed daemon identified as `git-f7e341ad6b97`, not the current `main` head.
- **QUELLCODE FERTIG / WEITERE REALTESTS OFFEN:** Browser-admin submission, reliable Agent availability after boot and actual recording/playback after the scheduled broadcast were not accepted on hardware; none follows from source or HTTP 202.
- **AUSSERHALB E2:** Hisense VIDAA device installation/UI acceptance and broader tuner/backend/concurrent/conflict/recurring/encrypted scenarios and separate operational recovery qualification. `outcome_unknown` stays unresolved pending genuine reconciliation evidence; no blind CREATE retry.

**E2 source judgment:** No new Timer CREATE implementation or architecture extension was warranted by this inspection. Source is complete for the bounded supported one-shot path; wider field/TV acceptance is separate. This document does not approve deployment.

## Real yaVDR public-v1 CREATE end-to-end acceptance (2026-10-19)

**Accepted, strictly bounded test:** The installed daemon identified itself as
`git-f7e341ad6b97` ([source commit](https://github.com/hotzenplotz5/vdr-suite/commit/f7e341ad6b9730bc35bf223c26a04f5b1df39ce8)).
On a real yaVDR running **VDR 2.8.3** and **SuiteBridge 0.14.1**, a paired,
administrator-approved test Device with explicit `epg.view` and
`timers.create` grants for backend `default` submitted a real
`POST /api/v1/epg/timer-intents`. The tested EPG identity was **Tagesschau**
on **Das Erste HD**, channel `C-1-1051-10301`, event `13520`, airing
**2026-10-20 20:00–20:15 Europe/Berlin**.

The recorded end-to-end results were:

```ini
API_V1_ROOT_HTTP=200
ADMIN_LOGIN_HTTP=200
PAIRING_CREATE_HTTP=201
PAIRING_APPROVE_HTTP=200
CREDENTIAL_ISSUE_HTTP=201
GRANT_EPG_VIEW_HTTP=200
GRANT_TIMERS_CREATE_HTTP=200
EPG_GET_HTTP=200
EPG_TARGET_CHECK=PASS
API_V1_TIMER_POST=202
OPERATION_ID=op_d9589fb8b43b7d91ac4309423a2fc372
OPERATION_STATE=succeeded
VDR_TIMERS_BEFORE=0
VDR_TIMERS_AFTER=1
VDR_MATCHING_NEW_TIMERS=1
VDR_READBACK=PASS
TEMP_DEVICE_REVOKE_HTTP=200
TEMP_DEVICE_REVOKED=PASS
ADMIN_LOGOUT_HTTP=204
TEST_RESULT=PASS
```

The resulting *native* VDR timer was:

```text
2 1:C-1-1051-10301:2026-10-20:2000:2015:50:99:Tagesschau
```

This is evidence of real Public-v1 admission, native CREATE execution,
durable Operation **`succeeded`**, native readback, and revocation of the
**test-only** Device. It is **not** a mock, and was **not** a direct
`svdrpsend NEWT` test. The timer was intentionally retained; the actual
broadcast recording, playback and later recording integrity were **not** yet
verified. No Hisense VIDAA TV acceptance follows from this backend test.

**Required operational fences:** An authenticated paired Device with
backend-specific grant, fresh strong EPG ETag and client-supplied idempotency
key; backend write permission and exact EPG/generation identity; a running
authenticated Backend Agent with current lease, provider facts, matching
SuiteBridge provider epoch and native CREATE capability; exact backend-local
CLOCK attestation and strictly read-only `NTPREVIEW CHECK 1` evidence. For
this isolated candidate the preview returned `isolated_clear`. The first
attempt returned HTTP **503** while the Backend Agent was
`inactive/dead`, autostart `disabled`, and its lease/provider evidence stale.
After a controlled Agent start and renewed provider evidence, admission
and dispatch passed. **Agent autostart remains disabled**, so reboot
reliability is **not** accepted. Do not treat `503` as a permanent feature
state or weaken any fence to bypass it.

**Scope limitations:** Only the specific future, one-shot, unencrypted,
nonconflicting live yaVDR scenario above was accepted. Recurrence, overlaps,
encrypted channels, remote or multi-backend scheduling, concurrent mutation,
cross-process recovery and `outcome_unknown` resolution are not proven by
this test. Never infer success from `202 Accepted` alone: inspect the
actor-owned `GET /api/v1/operations/{operationId}` and distinguish
`succeeded` from `outcome_unknown`. In an uncertain result do **not**
automatically issue another CREATE. The existing historical reconciliation
commands must not be reset, replayed or deleted; existing VIDAA Device identity
and grants must not be changed as part of test cleanup.

**Provenance:** The exact installed daemon build is
`git-f7e341ad6b97`, **not** automatically the current PR #500 head or
`main`. The test result is an operator-reported hardware acceptance record,
not a claim that every later source commit was independently installed.

## Historical Slice-2 boundary (superseded by consolidated source wiring)

`POST /api/v1/epg/timer-intents` is the *independent*, guarded Device-v1 entry for an EPG-selected one-shot recording. It is **not** a SearchTimer, an existing `TimerAssignment` POST, a native VDR timer ID, or a private `/api/vdr/timers/actions/create` alias.

**Slice 1 + incomplete Slice 2:** The request parser, strict permission gate, authoritative observation preflight and durable-Operation response adapter exist in source. The daemon registers the **read-only** `EpgTimerIntentObservationLookup`, reusing source event detail, backend Agent generation and write policy. The standalone atomic domain coordinator `EpgTimerIntentAtomicAdmissionService` now persists an intent, selected assignment, native CREATE Operation/payload and durable idempotency record in **one caller-owned transaction**, with rollback/replay/duplicate/stale-generation regressions. It requires a fresh authoritative native-specification attestation. **Historical 2026-10-18 Slice-2 finding (no longer current):** The earlier standalone source had no productive `EpgTimerIntentSubmit` registration and returned **503**. This statement is superseded by the composed daemon callback and the real 2026-10-19 public-v1 CREATE acceptance above. Source tests alone never authorized a hardware-acceptance claim.

## HTTP request

```http
POST /api/v1/epg/timer-intents
Content-Type: application/json
If-Match: <opaque strong ETag from GET /api/v1/epg/event>
Idempotency-Key: <client-generated visible ASCII key 1-160 bytes>

{"backendId":"home","channelId":"S19.2E-1-100","eventId":"81234"}
```

No query parameters. JSON is a closed object: exactly these three string fields, no start/stop, title, margins, VPS, source, generation, assignment or native Timer specification. The caller must obtain the event identity and ETag from `GET /api/v1/epg/event?backendId=...&channelId=...&eventId=...`. Reuse the same logical idempotency key and original precondition for a deliberate replay; **never** retry automatically after a timeout, 5xx or uncertain Operation outcome.

The `SecurityHttpGate` accepts either (a) an authenticated **paired Device** with explicit `timers.create` on the exact backend, independently of EPG read access, or (b) an authenticated **administrator Browser Session** with `role.admin` for that backend or `*` and valid Browser CSRF. An ordinary Human with only `timers.create` is refused. The actor and exact backend still flow through the normal authorization decision, and an independent backend write-policy check remains mandatory. The administrator path is covered by SecurityHttpGate tests; the recorded real yaVDR CREATE acceptance used a paired Device, **not** a Browser Session.

The runtime then fetches a fresh authoritative backend-scoped observation (backend generation, real event/source identity, future airing and server-owned EPG revision), independently re-resolves `GET /api/v1/epg/event` revision, and runs the existing `EpgTimerSelectionGuard::preflight`. A mismatch, expired airing, unavailable source, changed generation, missing rights or read-only backend cannot reach durable submit. The productive submit repeats generation/identity and idempotency checks **atomically** with persistence and uses `EpgTimerIntentDraft`, `TimerIntentRepository`, `TimerAssignmentRepository`, `NativeTimerCreateAdmissionService` and the existing Agent/readback/reconciliation chain. Do not create a second scheduler.

### Native capability and VDR-local clock findings (2026-10-18)

Real yaVDR read-only observations: VDR **2.8.3**, epgsearch **2.4.6**, RESTfulAPI HTTP conflict endpoint **200**, `PLUG epgsearch LSCC` returns **901 no conflicts found**, native SuiteBridge **0.14.1** responds to `PLUG suitebridge NTCREATE CAP 1` with `vdr-suite-ntcreate-cap/1 ... timer-create enabled suitebridge ... 1 1 enabled`. The general `CAPS 1` discovery's `mutations: disabled` is a distinct general module capability and does not negate this operation-specific native timer CREATE capability. **Neither result proves conflict freedom for a proposed Timer.** The local checkout HEAD `c9f9a1160` differs from this working branch; no live daemon/plugin was installed by these changes.

Source work in `5ddc9d53ffb61fc9daac393254ba87e26f837cf9` adds a **read-only** `PLUG suitebridge NTCREATE CLOCK 1 <unix-start> <unix-stop>` command to the native plugin handler without changing `EXEC`. It returns exact VDR-process-local calendar/HHMM and UTC offsets for both requested times, plus SuiteBridge provider epoch/revisions. Backend Agent commit `ad4d2cd4f2e29c228244cceb6d5c0bc4c25b28af` accepts only a typed response for the exact request, local zone projection and selected provider fence. This new CLOCK command is **not present in the already installed 0.14.1 plugin** until an explicitly approved new plugin build/install. The command is not advertised in public SVDRP HELP because existing architecture security policy forbids advertising native CREATE there. The derived domain provider fence is bounded by SHA-256 in `7b38f93338db8df4c9d1333c17b3b70201114d94`, retaining the 160-byte planner limit.

The source and Agent parsing still require **a real server/Agent runtime wiring and a native proposed-timer conflict preview**, neither of which is supplied by epgsearch `LSCC`. Do not call `confirmedClear`, register the productive submit, activate VIDAA recording actions, or create a native Timer based solely on successful `CAP`/`CLOCK`. The new native plugin code has standalone host CI but not a completed yaVDR/VDR-header/plugin install acceptance. No new CREATE is available on the running daemon.

## Durable replay at the Public-v1 edge (historical source-stage note)

The HTTP layer now has a read-only `EpgTimerIntentReplayLookup`, registered against `EpgTimerIntentAdmissionRepository` and the actor-authorized `MutationOperationReadService` in the Daemon. With an enabled submit, a re-sent logical key is looked up **after Device grant verification and strict body/precondition parsing but before live EPG freshness checks**. This allows a previously accepted Operation to be recovered after EPG expiration/backend loss; changed Event/ETag returns `409`, never a second CREATE. The original Operation may be `outcome_unknown` and **must** be displayed unresolved. This was the historical source-stage limitation; the later wired submit and the bounded real hardware CREATE acceptance supersede it.

## Backend-native one-shot clock proof (historical Slice-2 source stage)

`EpgNativeTimerTimeGuard.h` requires **backend-owned** start and stop UTC offsets (attested for the actual margin-adjusted times), rather than assuming the daemon's timezone matches a remote VDR backend. It checks native `day`, `HHMM` start/end, channel, recording margins, active one-shot weekdays, priority, lifetime and VPS against the authoritative EPG occurrence. Start seconds round down, stop seconds round up. Unknown clock provenance or an offset transition between boundaries remains **unavailable**: a VDR `day + HHMM` specification cannot disambiguate a DST repeated hour without additional native backend evidence. These inputs are not permitted in the Device's JSON body. At that earlier source stage no truthful runtime clock/capability/conflict provenance was connected to the public submit; the 2026-10-19 hardware acceptance above supersedes that status.

### Source follow-up: typed native clock attestation and conflict boundary (2026-10-18)

At source [`e17786ff`](https://github.com/hotzenplotz5/vdr-suite/commit/e17786ff6f6557a2cc7e5e06e82bca0fbb3351c3), `EpgNativeTimerClockAttestation.h` checks an already **typed/verified** `SuiteBridgeNativeTimerClockProjection` against the selected Agent lease/generation, Provider fence/epoch, exact start/stop Unix window and both local HHMM projections before generating domain clock evidence. The added domain regressions do not prove an installed native CLOCK command. This helper does not perform network I/O, create Timers, or supply conflict results.

An independent source review of [epgsearch `LSCC`](https://manpages.debian.org/testing/vdr-plugin-epgsearch/epgsearch.4.en.html) and its [published Service interface](https://github.com/vdr-projects/vdr-plugin-epgsearch/blob/master/services.h) found no non-mutating candidate-injection conflict query. Current `TimerConflictList` and `LSCC` enumerate existing conflicts. A new candidate's tuner/device impact is not verified by either endpoint. `ProspectiveNativeTimerConflictProof` must therefore remain absent until a real simulation is built and accepted. No `EpgTimerIntentSubmit` registration or public CREATE activation is authorized by this source change. Previous green CI `911beb67` / [39118345979](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39118345979); new source CI [39119610820](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39119610820) must be rechecked.

### Real installed SuiteBridge 0.14.1 outcome (2026-10-18)

Read-only native `NTCREATE CAP 1` returns `900 ... timer-create enabled`, but `NTCREATE CLOCK 1 1792222800 1792226400` returns `501 vdr-suite-ntcreate-rejected/1 unsupported` on the user's running yaVDR, whose checkout is on MU10E3, not this Slice-2 branch. This is an expected old-plugin response, not evidence of VDR time zone correctness. The more general discovery `mutations: disabled` is independent of operation-specific NTCREATE CAP. No timer was created.

The new `attestBackendNativeTimerClockReply` source entry requires a validated typed native CLOCK reply plus current Agent lease, Backend generation, Provider fence and exact request window. A `501` maps to `native_timer_clock_unavailable`, resets the evidence to non-current and cannot satisfy atomic admission. This is regression-tested with a simulated wire reply in the Timer domain, not a real installed CLOCK acceptance. [Source CI 39121803291](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39121803291) requires final check. That earlier checkpoint lacked proposed-timer conflict preview and a productive submit; both were subsequently wired for the bounded case documented above.

### Native prospective conflict preview kernel (source-only, 2026-10-18)

`suitebridge_native_timer_preview_core.cpp` introduces a bounded pure decision step and C++ failure regressions. It does **not** emit a trusted `ProspectiveNativeTimerConflictProof`, and cannot on its own justify `candidate.conflict = confirmedClear`. It rejects unresolved overlapping Timers, repeating active Timers, missing native snapshots, encrypted candidates, and absent/non-preempting device evidence. Producing a positive proof still requires VDR-owned, read-only timer/channel/device capture and provider-epoch, backend-generation and exact-specification verification at the control plane. The installed SuiteBridge 0.14.1 is unchanged, `NTCREATE CLOCK` remains unproven locally, and the Public API CREATE remains fail-closed.

### Native preview wire and admission boundary (2026-10-18)

Source series `54199108` through `97245f5e` now includes a bounded C++ native VDR snapshot producer, a separate `NTPREVIEW CHECK 1` SuiteBridge SVDRP handler, a typed Backend-Agent transport verifier, and a conversion helper that binds the already verified prospective result to the full native Timer specification, Agent lease/generation and selected Provider epoch/fence. The source snapshot code uses `GetTimersRead`, `GetChannelsRead`, indexed `cDevice::GetDevice(index)`, `ProvidesTransponder`, `MaySwitchTransponder` and a repeated Timer state check; it does **not** write Timers, tune devices, update CAM assignments or invoke Native CREATE. Its conflict checks cover the complete VDR-minute-rounded candidate interval, including subminute edges. The native proof age is counted from the VDR observation time rather than restarted by the receiving Agent. The preview engine only returns `isolated_clear` for a complete read-only native inventory with no active overlapping Timer and at least one unoccupied, compatible device for an exactly projected free-to-air one-shot. Recurrences, overlaps, encrypted channels, races, unavailable device evidence and incomplete snapshots return **unknown**, not `confirmedClear`. A bounded rolling SHA-256 record covers all inspected facts up to 4096 Timers.

**2026-10-18 native audit hardening:** Upstream VDR 2.8.3 `cDevice::GetDevice(channel, ..., Query=true)` can release an inactive CAM slot despite `Query=true`; `cDvbDevice::ProvidesChannel` also modifies its bonded-receiver flag while `const`. Neither method is called in the revised native preview. Source commit [`cd39c4e5`](https://github.com/hotzenplotz5/vdr-suite/commit/cd39c4e5752fd8dad205148ad021bee929476292) passed pinned VDR 2.8.3 header CI [39127037496](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39127037496). Minute-window and boundary regression source commit [`e500da40`](https://github.com/hotzenplotz5/vdr-suite/commit/e500da40a3ae57b3b8f4e2fd2b8d3d0898f6c4b7) also passed [pinned native header CI 39127157793](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39127157793); [full source CI 39127157979](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39127157979) must be checked independently. This is **not** an installed-plugin, production read-only, scheduler or hardware acceptance. Public CREATE remains disabled and VIDAA not accepted.

**Limitations / release gate:** This is a strictly conservative prospective subset, not epgsearch's full tuner-allocation scheduler. A source mock may validate the parser, but its success must NEVER be treated as live VDR evidence. The new native VDR object and SuiteBridge entrypoint/service files now **pass syntax compilation against the exact official VDR 2.8.3 release headers**, pinned to upstream commit `339b9969b0cfaf27d3941f135e17a09b984cd094`, in [native CI 39126057133](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39126057133) on source commit `afb50c4b`. This proves source compatibility only: the plugin is **not built as a deployed binary**, installed, or accepted on real hardware. The existing SuiteBridge on yaVDR still returns `501 unsupported` for `NTCREATE CLOCK`. At this historical checkpoint the Daemon had **not** registered the productive `EpgTimerIntentSubmit` callback; the later source and installed-daemon acceptance above supersede this status. A final full-head CI acceptance plus native build/read-only preview acceptance, exact backend-scoped Daemon composition, Native CREATE/readback/outcome-unknown tests and public HTTP acceptance remain mandatory. Relevant source CI [39125145695](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39125145695) must be checked at the exact head.

## Results and client obligations

`202 Accepted` with `Location: /api/v1/operations/{operationId}` means **durable Operation accepted**, never **Timer scheduled successfully**. Show accepted/queued/dispatching/verified states separately. `outcome_unknown` is unresolved and must not trigger another POST. After authenticated Suite SSE hints, refresh the authorized Operation and native Timers resource using normal Public-v1 GET; no new polling loop or private fallback.

Other outcomes include 400 malformed selection/headers, 401 unauthenticated, 403 Device-grant or backend-write denial, 404 no observed airing, 409 expired/identity/generation/state/idempotency conflict, 412 EPG revision conflict, 415 unsupported media type, 422 invalid server-side intent, 428 missing strong If-Match and 503 missing/unavailable admission services. See `PublicApiRuntime.cpp`, `SecurityHttpGate.h`, `test_public_epg_timer_intent_admission.cpp` and `test_public_epg_timer_intent_security.cpp`.

## Atomic admission implementation (earlier source checkpoint and later live acceptance)

The [Slice-2 coordinator](../core/timers/include/EpgTimerIntentAtomicAdmissionService.h) shares the existing TimerIntent and TimerAssignment repositories, the planner and NativeTimerCreateAdmissionService. A single SQLite transaction encompasses intent, assignment, native CREATE Operation/payload and the separately scoped actor/backend/idempotency-to-operation mapping. Exact replay returns the original durable Operation; a reused key with changed Event/ETag fails, and an active semantically equivalent intent is not silently duplicated. A failed Operation INSERT rolls back everything. The SQL implementation belongs solely to `EpgTimerIntentAdmissionRepository`, preserving the architecture boundary.

The coordinator does **not** invent freshness evidence: a mandatory server-owned `revalidateSourceAndNativeSpecification` callback must prove that authoritative backend generation/EPG identity, complete native TimerSpecification, source title and chosen planning evidence are still current before any write. The real Daemon submit must also construct genuinely current execution authority, capability, health, channel, conflict and backend-local VDR clock evidence. No `confirmedClear` conflict can be inferred from a generic snapshot or user-supplied text. The subsequent daemon callback supplies these prerequisites for the narrowly accepted one-shot hardware scenario above; unavailable or stale evidence still fails closed.

**Verified source code:** `f8a5ab84fecd06e9de3f968b4c8deaa0181123bf`; [GitHub CI #10269 / run 39096159408](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39096159408), six checks PASS, including C++ fast regressions and hosted daemon build. **No yaVDR apply, running-daemon hash change or Hisense acceptance.** For volatile state, see [CURRENT](../docs/CURRENT.md).

## Remaining authorization and acceptance

- **Backend Agent restart reliability:** Inspect systemd/packaging and confirm identity reuse, current lease, provider facts and epoch after an actual restart before considering boot reliability accepted. The Agent is running for the successful test but autostart is still disabled.
- **Timer execution:** The public-v1 one-shot CREATE and native readback succeeded on the installed daemon described above. Actual 2026-10-20 recording, restart/reconciliation, ambiguous outcomes, multi-backend, concurrent/conflicting, encrypted and recurring cases remain unaccepted.
- **VIDAA:** Consume this same Public-v1 Device contract from EPG detail with explicit confirmation, strong ETag, idempotency key and an actor-owned Operation GET; never equate `202` with a native Timer. No Hisense TV acceptance yet.
- **Lifecycle:** Keep historical reconciliation commands and existing paired VIDAA devices intact. `docs/CURRENT.md` is still the `main` operational authority; this branch-specific 2026-10-19 acceptance must be reconciled there at the appropriate integrated checkpoint.

Return to [TimerAssignment contract](timer-assignments.md) · [coverage roadmap](../docs/planning/public-v1-coverage-roadmap.md).
