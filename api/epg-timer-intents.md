# Public API v1 — one-shot EPG TimerIntent CREATE admission

## Boundary and current status (2026-10-18)

`POST /api/v1/epg/timer-intents` is the *independent*, guarded Device-v1 entry for an EPG-selected one-shot recording. It is **not** a SearchTimer, an existing `TimerAssignment` POST, a native VDR timer ID, or a private `/api/vdr/timers/actions/create` alias.

**Slice 1 only:** The request parser, strict permission gate, authoritative observation preflight and durable-Operation response adapter exist in source. The production daemon does **not** yet register both required `EpgTimerIntentObservationLookup` and `EpgTimerIntentSubmit`. Until Slice 2 wires and tests atomic TimerIntent/TimerAssignment/Operation persistence, this route must remain `503`; no real native Timer is created by this contract. Discovery must never imply otherwise. No daemon rebuild or installation is authorized by this document.

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

## Results and client obligations

`202 Accepted` with `Location: /api/v1/operations/{operationId}` means **durable Operation accepted**, never **Timer scheduled successfully**. Show accepted/queued/dispatching/verified states separately. `outcome_unknown` is unresolved and must not trigger another POST. After authenticated Suite SSE hints, refresh the authorized Operation and native Timers resource using normal Public-v1 GET; no new polling loop or private fallback.

Other outcomes include 400 malformed selection/headers, 401 unauthenticated, 403 Device-grant or backend-write denial, 404 no observed airing, 409 expired/identity/generation/state/idempotency conflict, 412 EPG revision conflict, 415 unsupported media type, 422 invalid server-side intent, 428 missing strong If-Match and 503 missing/unavailable admission services. See `PublicApiRuntime.cpp`, `SecurityHttpGate.h`, `test_public_epg_timer_intent_admission.cpp` and `test_public_epg_timer_intent_security.cpp`.

## Remaining authorization and acceptance

- **Backend Slice 2:** Implement trusted authoritative resolver and atomic durable submit; readback, no-duplicate/replay, unknown-outcome and restart tests.
- **VIDAA Slice 4:** Use the same public Device contract from EPG timeline and day detail after backend installation, preserving D-pad focus and explicit confirmation; never equate `202` with native Timer presence.
- **Real yaVDR/Hisense:** Uninstalled and unaccepted on current Slice-1 source. `docs/CURRENT.md` remains the operational truth.

Return to [TimerAssignment contract](timer-assignments.md) · [coverage roadmap](../docs/planning/public-v1-coverage-roadmap.md).
