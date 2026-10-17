# Public-v1 EPG → TimerIntent admission: verified gap and first guard

Status: **CONTRACT FOUNDATION ONLY**. Dependent on draft VIDAA EPG identity
PR #491, source head `35bf9a0ebcce8ef9954c966a594c98e04bfa7f7d`.
The new guard is not wired into PublicApiRuntime and no endpoint/capability is
advertised. No end-user Timer CREATE can be claimed.

## Why the existing POST cannot be reused from the EPG

The accepted `POST /api/v1/timer-assignments/{timerAssignmentId}?backend=...`
only admits a native CREATE for a **previously durable**, revisioned
TimerAssignment with a server-selected NativeTimerSpecification. The bounded
Public-v1 `/epg/window` response returns channel ID, EPG event ID, title and
time, but **no TimerIntent, TimerAssignment or server-approved assignment
mapping**. Trying the old `/api/vdr/timers/actions/create`, a guessed
TimerAssignment ID, or a client-computed native Timer line would bypass
ADR-0044/0045, permission, backend-generation and idempotency owners.

The upstream Webfrontend may use legacy native VDR timer endpoints. That
route is not approved as a VIDAA Device-authenticated Public-v1 replacement.

## Proposed next Public-v1 resource (NOT implemented or frozen)

1. **Read** one exact EPG airing using a verified backend/channel/native event
   reference. Source: authoritative VdrEvent observation, not a title/time
   hash or mutable display card. Return a server-issued opaque strong ETag,
   current identity/provenance, start/end and metadata. Rejection if missing,
   stale, withdrawn, or backend changed. A backend-native `eventId` is NOT
   an ADR-0045 canonical `programEventId`.
2. **Explicit recording request** after independent `timers.create@backend`
   authorization and backend write policy. The Device submits only its
   backend/channel/native-event reference, the **read-derived** `If-Match`,
   and a caller-owned `Idempotency-Key`. Never trust title, epoch, native
   timer fields, margins, guessed ProgramEvent ID, or source URLs from TV.
3. **Server-owned admission:** re-read and validate the selected occurrence,
   actor/backend and observation revision; resolve authoritative source ID,
   source channel and backend generation; construct a backend-neutral
   `programmeEvent` TimerIntent (native event fallback only with complete
   provenance) using server-owned defaults/policy; create a durable
   TimerAssignment + native specification via existing planner; submit the
   existing native CREATE Operation without duplicated writes.
4. **Effect visibility:** an HTTP 202 is an accepted durable operation, not a
   completed VDR timer. Show Operation status (including uncertainty), then
   authoritative native timer readback. A lost response must **not** auto-retry
   with a new key.
5. **TV UX:** on an exact EPG event, OK opens programme detail; an explicit
   `Aufnahme planen` action appears **only after** the server exposes the
   fully implemented compatible capability and correct backend grant. A
   second focused confirmation sends precisely one immutable POST; BACK
   cancels without side effects. Never invent an EPG→Assignment mapping.

Exact wire route, JSON envelope, ETag construction, idempotency scope,
capability name, timer margins, timezone, conflict mapping, operation
reconciliation and the public EPG detail ID must be finalized **from the
productive runtime** before any VIDAA client requests a new endpoint.
This contract intentionally refrains from guessing those details.

## Implemented first boundary

`api/rest/include/PublicEpgTimerSelectionGuard.h` defines a pure,
side-effect-free internal validator. It fails closed on missing/oversized
identity, missing or weak ETag, invalid idempotency key, independently missing
grant, read-only backend, missing true native/source identity, stale
observation, missing generation, mismatched backend/channel/event,
revision mismatch or expired broadcast. It receives *trusted server
observation facts* only. Its `ready` result **does not authorize any side
effect** by itself.

`api/rest/tests/test_public_epg_timer_selection_guard.cpp` runs isolated
C++ assertions without SQLite, server, VDR or filesystem mutation.
The Make target is `test-phase69-public-epg-timer-selection`.

The initial work is purposely isolated: `PublicApiRuntime`, security HTTP
gate, OpenAPI capability inventory, deployed daemon, client and VDR binaries
remain unchanged. Next bounded implementation is EPG detail GET with
server-generated strong ETag and the real read-authority check.

## Acceptance and barriers

- `SOURCE_CI`: must be verified against the exact branch head.
- `PUBLIC_TIMER_INTENT_ROUTE=NOT_IMPLEMENTED`
- `VIDAA_EPG_TIMER_CREATE=NOT_AVAILABLE`
- `DEVICE_GRANT_TEST=PENDING`
- `REAL_VDR_READBACK=PENDING`
- `DEPLOY=NOT_STARTED`

Do not label this foundation a usable timer fix.

## Second server-side implementation boundary: typed TimerIntent projection

`api/rest/include/PublicEpgTimerIntentDraft.h` now takes an already verified
`AuthoritativeObservation`, a separately authenticated actor identity, and
**server-owned** recording defaults and timezone. It constructs an ADR-0044
`programmeEvent` `TimerIntentSpec` with a complete backend-native fallback
reference and matching source-qualified channel identity. It **does not** forge
a `programEventId`. It pins the selected backend, keeps one replica, and uses
existing `timerIntentValidSpec` validation. An empty timezone, overlarge
priority, stale EPG, missing write grant, source ID or backend generation
cannot yield a valid draft.

The occurrence `eventId` and source channel are capped to 160 characters at
this boundary because the actual `TimerIntent` model is stricter than the
read-only Public EPG window (256 bytes). An unrepresentable event is refused
rather than truncated. The corresponding independent C++ regression is
`test-phase69-public-epg-timer-intent-draft`.

**Next required:** a server-side EPG observation resolver plus revisioned
Public EPG detail GET, followed by durable actor-owned TimerIntent persistence,
planner-selected native specification, 202 Operation submission, and verified
readback. Neither pure guard nor typed draft creates a TimerIntent row or a
native VDR timer. Never deploy or announce Timer CREATE from this branch alone.
