# VIDAA EPG → genuine native Timer: verified gap and acceptance contract

Status (2026-10-17): **NOT IMPLEMENTED / no Device timer mutation is approved.**
This file accompanies an independent read-only `eventId` projection, not a release claim.

## Existing source boundaries (exact installed parent b45799b0)
- `/api/v1/epg/window` provides Device-authorized channel/time-scoped EPG,
  but the installed baseline omits native `VdrEvent.id` and full description.
  This PR adds an **optional, bounded source ID** to the read response only.
- `GET /api/v1/native-timers` already gives `timers.view@backend` a snapshot
  of real VDR timers. The VIDAA client uses it; do not replace it with Suite-owned
  TimerAssignments.
- `POST /api/v1/timer-assignments/{assignmentId}` uses conditional ETag,
  Idempotency-Key, durable MutationOperation, and eventual native readback,
  **but requires an already existing TimerAssignment**. It does not start from
  an EPG event or create the missing TimerIntent/Assignment.
- Browser `POST /api/vdr/timers/actions/create` accepts a legacy native timer
  payload under browser authorization/CSRF. It is **not Public-v1**, not a Device
  API, and must not be called or proxied by VIDAA.
- `DeviceGrantAdministrationService::supportedGrant` currently permits
  `timers.view` but not Device `timers.create`. No implicit grant from
  pairing or Administrator authority may be inferred.

## Proposed separate additive slice (requires review before coding)
1. Expose an event detail read under exact `epg.view`, backend/channel/event
   ownership, backed by the current EPG cache. Return full bounded title,
   description, start/end, real event ID, and a strong opaque revision/ETag.
   The incoming identifier is **not** proof of authenticity by itself.
2. Have the server re-read and verify event identity/revision and recordable
   channel before constructing a typed NativeTimerSpecification. Keep options
   bounded, initial defaults explicit, and record the selected event reference.
3. Admit one Device-only timer request under exact `timers.create@backend`,
   an explicitly reviewed additional allowlisted Device grant for **one concrete
   backend**, plus existing backend write-mode and SecurityGate checks.
   Require the original opaque strong event ETag and one stable caller-owned
   Idempotency-Key. Browser session CSRF and Device credentials remain distinct.
4. Reuse the existing TimerIntent → TimerAssignment → native CREATE durable
   operation pipeline (reservation, typed SuiteBridge, readback, reconciliation).
   Do not invent a second worker/queue or call legacy Timer CRUD. HTTP 202 means
   accepted only; it **never** means the native VDR timer already exists.
5. The TV must display a separate, explicit confirmation first. It must never
   automatically retry a possibly accepted POST, silently change its backend,
   infer success from an event title, or create a second timer when readback
   is delayed. Present the operation ID and later observed status accurately.
6. Existing recordings, EPG cache, grants and timer entries are read-only to
   any preflight/tests. Real effect acceptance requires an isolated known
   disposable timer fixture and an explicit operator-controlled TV test.

## Required evidence before a TV button can be enabled
- exact event ID preserved; absent, oversized, stale, wrong-channel or
  cross-backend IDs rejected without a fabricated fallback;
- Device with only `timers.view` denied CREATE; exact `timers.create` verified;
  revoked, wrong-backend, unauthenticated and browser-CSRF cases segregated;
- duplicate POST with the same key cannot dispatch a second native action;
  lost POST response and outcome_unknown are reconciliation-only;
- valid real backend ownership/generation/lease; readback confirms one VDR
  timer, with correct native channel, start/end, title, active state;
- independent UI tests with native Hisense remote and actual EPG details,
  followed by user-confirmed hardware acceptance; CI alone is insufficient.

## Follow-up ownership
This read-only event identity PR must **not** be used as a reason to rebuild
the yaVDR daemon or install a TV candidate. The protected event-detail and
native Timer CREATE admission are separate, reviewable product slices.
