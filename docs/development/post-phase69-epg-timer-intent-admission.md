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

`core/timers/include/EpgTimerSelectionGuard.h` defines a pure,
side-effect-free internal validator. It fails closed on missing/oversized
identity, missing or weak ETag, invalid idempotency key, independently missing
grant, read-only backend, missing true native/source identity, stale
observation, missing generation, mismatched backend/channel/event,
revision mismatch or expired broadcast. It receives *trusted server
observation facts* only. Its `ready` result **does not authorize any side
effect** by itself.

`core/timers/tests/test_epg_timer_selection_guard.cpp` runs isolated
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

`core/timers/include/EpgTimerIntentDraft.h` now takes an already verified
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

**Ownership correction:** Both pure value checks live in `core/timers`,
not in the public REST implementation. The later HTTP controller only
validates the wire representation and invokes this domain-owned gateway;
no timer policy or NativeTimerSpecification will be assembled in the
PublicApiRuntime serializer. This migration changes no runtime link graph.

## Critical cache-provenance prerequisite (source verified 2026-10-17)

`core/vdr/src/EpgEventRepository.cpp:stableEventId` synthesizes a transient
cache key from `channelId:startTime:title` when the provider supplied **no**
native event ID. A read from `/api/v1/epg/window` exposes the stored key as
`eventId`, so even a nonempty EPG response ID does **not** prove a native
VDR event reference! Synthetic keys must never enter an ADR-0044 TimerIntent as
an exact source EventID.

The new **additive** `epg_event_id_source_evidence` table stores whether
`VdrEvent.id` was nonempty on input to the EPG repository; a scoped
after-delete trigger retires evidence with its cache row. No
`epg_events` table columns were changed (older explicit/positional SQL
inserts must remain valid). An upsert records source-ID presence in the same
database transaction as the EPG event; repeated unchanged inputs avoid
touching either record. Existing cache rows have **no evidence** and the new
`hasSourceEventIdForBackend` accessor refuses them until a fresh source
observation. SQL errors, wrong backends, deleted events and generated fallback
identities refuse the match.

This is **necessary but not sufficient** for `exactNativeIdentity` in the
admission guard: an upstream adapter may already have synthesized its own
ID; source/adaptor provenance and backend generation also require
authoritative verification before `ready`. No HTTP or native Timer
mutation is exposed. Regression: `test-epg-event-repository`, covering
migration/old rows, true IDs, fallbacks, different backends, no-op upsert,
removal evidence and cleanup.

This changes the compiled repository only on this unpublished source branch.
Do not run schema migration or install a new daemon on yaVDR before an exact
candidate, CI, resource preflight and separately authorized deployment.

## Read-only Public EPG projection now consumes provenance evidence

`EpgCacheService::hasSourceEventIdForBackend` delegates through the existing
read-repository, not an independent cache. The draft PR #491's productive
`DaemonRuntimeInitialization.cpp` `/api/v1/epg/window` mapping emits the
optional `eventId` only when exact backend/channel/key evidence exists and
the ID is representable by the 160-byte `TimerIntentBackendEventRef`.
Fallback identifiers and old cache rows remain valid for **displaying** an EPG
programme but have no recordable ID. No new API route, write permission or
TimerAssignment is inferred. Existing VIDAA clients already support absent
`eventId` as a read-only event, though the timer CREATE UI remains gated.

The EPG read-model difference must still be integration-tested with a real
cache refresh before relying on the source ID. Event-ID presence alone is
necessary but not sufficient to prove actor, backend, generation, source type
and final authorization.

The additional static source-ownership regression
`tools/check_public_epg_event_id_provenance.py` is bound to
`make test-phase69-public-epg-timer-selection`. It verifies the
original fallback behavior still exists, persistent source-id evidence,
the read-repository delegation, scoped public-window emission, and
the native-ID check at the domain boundary. Its PASS is **not** a
substitute for compiling/running the repository and live runtime tests.

## Public-v1 wire boundary: 160-byte EPG Event ID (2026-10-17)

The server-owned `TimerIntentBackendEventRef.eventId` and the authoritative
EPG cache projection accept source-provenanced event IDs of at most **160
bytes**. Public-v1 `/api/v1/epg/window` must not emit longer values, even if
a separately registered read callback supplies one: exposing a 161–256-byte
`eventId` would imply a selectable occurrence that the TimerIntent guard
cannot represent. `PublicApiRuntime.cpp` now caps that optional field at 160,
with boundary regressions for 160 (included) and 161 (omitted) in
`test_public_epg_now_next.cpp`. This is a serialization consistency fix,
**not** a claim of a complete EPG-detail/CREATE/Device-write route. Provenance,
backend generation, grant, revision, admission and native readback remain
mandatory before enabling a VIDAA one-off recording button.

## Exact source-event lookup prerequisite (2026-10-17)

`EpgEventRepository::findSourceEventForBackend` now resolves a single
backend/channel/native event ID in **one SQLite query** joined to the existing
`epg_event_id_source_evidence` table. It returns the original bounded
`VdrEvent` (including description and source schedule), or no observation for
legacy rows, synthetic cache keys, wrong scope, deleted rows or SQL failures.
`EpgCacheService` delegates to the same read repository as the public EPG
window, without a competing index or independent provider. The existing
`test-epg-event-repository` target covers positive read, description,
backend/channel isolation, source-ID absence, historical rows, updates,
deletes and oversized IDs. This is the prerequisite for a future revisioned
public event-detail resource and admission resolver. It does **not** assert
backend generation/currentness, allow a Device write or generate a timer.
Hardware/native effect and the unimplemented HTTP/Assignment pipeline remain
pending; CI results for this exact product commit must be evaluated separately.

## Protected Public-v1 native EPG event detail GET (2026-10-17)

`GET /api/v1/epg/event?backendId=<backend>&channelId=<channel>&eventId=<native>` is now a scoped read-only resource after the existing `epg.view@backend` or exact-backend `media.live.play` authorization. A matched backend agent generation, live backend/channel snapshot and durable native-ID evidence are prerequisites. The response contains bounded title, subtitle, description, source time/duration and an opaque strong ETag built from SHA-256 over length-delimited event fields and generation. The response never reveals raw generation or NativeTimerSpecification. `If-None-Match` yields a no-body 304; absent native identity is 404, backend unavailable 503, invalid selection 400. Test target: `make test-public-epg-event-detail`. A read ETag does NOT grant writes; no Device TimerIntent admission or one-off VDR CREATE route is added in this slice. No TV installation/approval is authorized.

The read-detail API regression is included in `CI_FAST_TESTS` and the
existing security EPG gate regression now proves same-backend `epg.view`,
`media.live.play`, wrong-backend rejection and no mutation flag for the
new exact-event route. This is not an authorization proof for a future POST.

The capability discovery `GET /api/v1/capabilities` advertises
`public-api.epg-event-detail-read` at version 1, dynamically
`available` only while the read callback is registered and
`unavailable` otherwise. The runtime contract test asserts both states.
The capability is deliberately separate from `public-api.timer-create-admission`;
it grants no `timers.create` action and does not certify native timer
execution. This additive read capability is included without changing
or weakening the existing Phase-69 route inventory gate.

## Reuse existing Intent/Assignment without consuming native CREATE admission

`TimerIntentApplicationService::submitAndSelectPrimary` is added to the
*existing* TimerIntent application workflow. It uses the same durable
`TimerIntentRepository`, `TimerAssignmentSchedulingService`, candidate
planner and duplicate-ownership checks as `submitAndProvisionPrimary`, but
returns a revisioned primary Assignment in `selected` state. The caller
must then use the already-existing atomic
`NativeTimerCreateAdmissionService::admit`, which alone transitions that
Assignment to `provisioning` and durably prepares the native Operation.
The original immediate-provision method is unchanged for existing callers.
Regression `test-timer-intent-application-service` covers persisted
selection, restart/idempotent replay, no premature mutation, subsequent
legacy provisioning and conflicting semantic replay.

**Limit:** This is domain-service composition, not a public Device POST.
No synthetic conflict-clear evidence, guessed backend timezone or grant
is introduced. Selection cannot be exposed as a verified VDR recording;
public write admission, capability/health/channel/conflict evidence and
native readback still gate the VIDAA action.
