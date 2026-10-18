# Public API v1 — one-shot EPG TimerIntent CREATE admission

## Boundary and current status (2026-10-18)

`POST /api/v1/epg/timer-intents` is the *independent*, guarded Device-v1 entry for an EPG-selected one-shot recording. It is **not** a SearchTimer, an existing `TimerAssignment` POST, a native VDR timer ID, or a private `/api/vdr/timers/actions/create` alias.

**Slice 1 + incomplete Slice 2:** The request parser, strict permission gate, authoritative observation preflight and durable-Operation response adapter exist in source. The daemon registers the **read-only** `EpgTimerIntentObservationLookup`, reusing source event detail, backend Agent generation and write policy. The standalone atomic domain coordinator `EpgTimerIntentAtomicAdmissionService` now persists an intent, selected assignment, native CREATE Operation/payload and durable idempotency record in **one caller-owned transaction**, with rollback/replay/duplicate/stale-generation regressions. It requires a fresh authoritative native-specification attestation. **No productive `EpgTimerIntentSubmit` is registered yet**, because the necessary true backend-native schedule, capability/readback, health and conflict evidence is not currently composed in the daemon. The public route intentionally remains **503**, has not created a native timer, and must not be exposed as available. Source tests and CI do not authorize daemon build or installation.

## HTTP request

```http
POST /api/v1/epg/timer-intents
Content-Type: application/json
If-Match: <opaque strong ETag from GET /api/v1/epg/event>
Idempotency-Key: <client-generated visible ASCII key 1-160 bytes>

{"backendId":"home","channelId":"S19.2E-1-100","eventId":"81234"}
```

No query parameters. JSON is a closed object: exactly these three string fields, no start/stop, title, margins, VPS, source, generation, assignment or native Timer specification. The caller must obtain the event identity and ETag from `GET /api/v1/epg/event?backendId=...&channelId=...&eventId=...`. Reuse the same logical idempotency key and original precondition for a deliberate replay; **never** retry automatically after a timeout, 5xx or uncertain Operation outcome.

The SecurityHttpGate requires an authenticated **paired Device** and an explicit `timers.create` grant on that precise backend, independently of EPG read access. Browser-session credentials cannot substitute for a Device credential; no wildcard permission broadening. The backend write policy is a separate server-side requirement.

The runtime then fetches a fresh authoritative backend-scoped observation (backend generation, real event/source identity, future airing and server-owned EPG revision), independently re-resolves `GET /api/v1/epg/event` revision, and runs the existing `EpgTimerSelectionGuard::preflight`. A mismatch, expired airing, unavailable source, changed generation, missing rights or read-only backend cannot reach durable submit. The productive Slice-2 submit must repeat generation/identity and idempotency checks **atomically** with persistence and use `EpgTimerIntentDraft`, `TimerIntentRepository`, `TimerAssignmentRepository`, `NativeTimerCreateAdmissionService` and the existing Agent/readback/reconciliation chain. Do not create a second scheduler.

### Native capability and VDR-local clock findings (2026-10-18)

Real yaVDR read-only observations: VDR **2.8.3**, epgsearch **2.4.6**, RESTfulAPI HTTP conflict endpoint **200**, `PLUG epgsearch LSCC` returns **901 no conflicts found**, native SuiteBridge **0.14.1** responds to `PLUG suitebridge NTCREATE CAP 1` with `vdr-suite-ntcreate-cap/1 ... timer-create enabled suitebridge ... 1 1 enabled`. The general `CAPS 1` discovery's `mutations: disabled` is a distinct general module capability and does not negate this operation-specific native timer CREATE capability. **Neither result proves conflict freedom for a proposed Timer.** The local checkout HEAD `c9f9a1160` differs from this working branch; no live daemon/plugin was installed by these changes.

Source work in `5ddc9d53ffb61fc9daac393254ba87e26f837cf9` adds a **read-only** `PLUG suitebridge NTCREATE CLOCK 1 <unix-start> <unix-stop>` command to the native plugin handler without changing `EXEC`. It returns exact VDR-process-local calendar/HHMM and UTC offsets for both requested times, plus SuiteBridge provider epoch/revisions. Backend Agent commit `ad4d2cd4f2e29c228244cceb6d5c0bc4c25b28af` accepts only a typed response for the exact request, local zone projection and selected provider fence. This new CLOCK command is **not present in the already installed 0.14.1 plugin** until an explicitly approved new plugin build/install. The command is not advertised in public SVDRP HELP because existing architecture security policy forbids advertising native CREATE there. The derived domain provider fence is bounded by SHA-256 in `7b38f93338db8df4c9d1333c17b3b70201114d94`, retaining the 160-byte planner limit.

The source and Agent parsing still require **a real server/Agent runtime wiring and a native proposed-timer conflict preview**, neither of which is supplied by epgsearch `LSCC`. Do not call `confirmedClear`, register the productive submit, activate VIDAA recording actions, or create a native Timer based solely on successful `CAP`/`CLOCK`. The new native plugin code has standalone host CI but not a completed yaVDR/VDR-header/plugin install acceptance. No new CREATE is available on the running daemon.

## Durable replay at the Public-v1 edge (source, not productive CREATE)

The HTTP layer now has a read-only `EpgTimerIntentReplayLookup`, registered against `EpgTimerIntentAdmissionRepository` and the actor-authorized `MutationOperationReadService` in the Daemon. With an enabled submit, a re-sent logical key is looked up **after Device grant verification and strict body/precondition parsing but before live EPG freshness checks**. This allows a previously accepted Operation to be recovered after EPG expiration/backend loss; changed Event/ETag returns `409`, never a second CREATE. The original Operation may be `outcome_unknown` and **must** be displayed unresolved. The backend does not register a productive submit yet, so the public POST remains `503` pending real backend execution/clock/conflict attestation.

## Backend-native one-shot clock proof (Slice 2 source, not enabled)

`EpgNativeTimerTimeGuard.h` requires **backend-owned** start and stop UTC offsets (attested for the actual margin-adjusted times), rather than assuming the daemon's timezone matches a remote VDR backend. It checks native `day`, `HHMM` start/end, channel, recording margins, active one-shot weekdays, priority, lifetime and VPS against the authoritative EPG occurrence. Start seconds round down, stop seconds round up. Unknown clock provenance or an offset transition between boundaries remains **unavailable**: a VDR `day + HHMM` specification cannot disambiguate a DST repeated hour without additional native backend evidence. These inputs are not permitted in the Device's JSON body. No truthful runtime clock/capability/conflict provenance has yet been connected to the public submit callback; the POST therefore still returns `503`.

### Source follow-up: typed native clock attestation and conflict boundary (2026-10-18)

At source [`e17786ff`](https://github.com/hotzenplotz5/vdr-suite/commit/e17786ff6f6557a2cc7e5e06e82bca0fbb3351c3), `EpgNativeTimerClockAttestation.h` checks an already **typed/verified** `SuiteBridgeNativeTimerClockProjection` against the selected Agent lease/generation, Provider fence/epoch, exact start/stop Unix window and both local HHMM projections before generating domain clock evidence. The added domain regressions do not prove an installed native CLOCK command. This helper does not perform network I/O, create Timers, or supply conflict results.

An independent source review of [epgsearch `LSCC`](https://manpages.debian.org/testing/vdr-plugin-epgsearch/epgsearch.4.en.html) and its [published Service interface](https://github.com/vdr-projects/vdr-plugin-epgsearch/blob/master/services.h) found no non-mutating candidate-injection conflict query. Current `TimerConflictList` and `LSCC` enumerate existing conflicts. A new candidate's tuner/device impact is not verified by either endpoint. `ProspectiveNativeTimerConflictProof` must therefore remain absent until a real simulation is built and accepted. No `EpgTimerIntentSubmit` registration or public CREATE activation is authorized by this source change. Previous green CI `911beb67` / [39118345979](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39118345979); new source CI [39119610820](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39119610820) must be rechecked.

## Results and client obligations

`202 Accepted` with `Location: /api/v1/operations/{operationId}` means **durable Operation accepted**, never **Timer scheduled successfully**. Show accepted/queued/dispatching/verified states separately. `outcome_unknown` is unresolved and must not trigger another POST. After authenticated Suite SSE hints, refresh the authorized Operation and native Timers resource using normal Public-v1 GET; no new polling loop or private fallback.

Other outcomes include 400 malformed selection/headers, 401 unauthenticated, 403 Device-grant or backend-write denial, 404 no observed airing, 409 expired/identity/generation/state/idempotency conflict, 412 EPG revision conflict, 415 unsupported media type, 422 invalid server-side intent, 428 missing strong If-Match and 503 missing/unavailable admission services. See `PublicApiRuntime.cpp`, `SecurityHttpGate.h`, `test_public_epg_timer_intent_admission.cpp` and `test_public_epg_timer_intent_security.cpp`.

## Source-tested atomic admission (not an enabled HTTP submit)

The [Slice-2 coordinator](../core/timers/include/EpgTimerIntentAtomicAdmissionService.h) shares the existing TimerIntent and TimerAssignment repositories, the planner and NativeTimerCreateAdmissionService. A single SQLite transaction encompasses intent, assignment, native CREATE Operation/payload and the separately scoped actor/backend/idempotency-to-operation mapping. Exact replay returns the original durable Operation; a reused key with changed Event/ETag fails, and an active semantically equivalent intent is not silently duplicated. A failed Operation INSERT rolls back everything. The SQL implementation belongs solely to `EpgTimerIntentAdmissionRepository`, preserving the architecture boundary.

The coordinator does **not** invent freshness evidence: a mandatory server-owned `revalidateSourceAndNativeSpecification` callback must prove that authoritative backend generation/EPG identity, complete native TimerSpecification, source title and chosen planning evidence are still current before any write. The real Daemon submit must also construct genuinely current execution authority, capability, health, channel, conflict and backend-local VDR clock evidence. No `confirmedClear` conflict can be inferred from a generic snapshot or user-supplied text. Until the daemon owns these facts and wires the submit callback, public EPG-to-native CREATE remains unavailable.

**Verified source code:** `f8a5ab84fecd06e9de3f968b4c8deaa0181123bf`; [GitHub CI #10269 / run 39096159408](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39096159408), six checks PASS, including C++ fast regressions and hosted daemon build. **No yaVDR apply, running-daemon hash change or Hisense acceptance.** For volatile state, see [CURRENT](../docs/CURRENT.md).

## Remaining authorization and acceptance

- **Backend Slice 2:** Implement the remaining production daemon-backed trusted planner/native-specification evidence and `EpgTimerIntentSubmit` registration. The atomic SQLite coordinator and its replay/rollback regression are already source-implemented and CI-tested; runtime dispatch, real readback, cross-process restart and unknown-outcome acceptance remain open.
- **VIDAA Slice 4:** Use the same public Device contract from EPG timeline and day detail after backend installation, preserving D-pad focus and explicit confirmation; never equate `202` with native Timer presence.
- **Real yaVDR/Hisense:** Uninstalled and unaccepted on current Slice-1 source. `docs/CURRENT.md` remains the operational truth.

Return to [TimerAssignment contract](timer-assignments.md) · [coverage roadmap](../docs/planning/public-v1-coverage-roadmap.md).
