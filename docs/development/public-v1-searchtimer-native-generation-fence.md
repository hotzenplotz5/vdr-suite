# Public-v1 SearchTimer MODIFY / ACTIVATE / DELETE: native generation fence

## Live yaVDR evidence (2026-10-22)

The operator deleted both epgsearch rules after a broad `terra` query
created 106 recording timers. The native `/searchtimers.json` catalogue
then contained 0 rules, while the Suite SQLite identity repository still
contained bindings for native IDs `0` and `1`.
The merged SearchTimer identity-lifecycle fix (PR #512) retires observed
absences and rotates unverified legacy mappings on an authoritative read.

That mitigates **observed** native-ID reuse; it does **not** establish the
identity of a native rule that was removed and recreated identically
*between* two successful reads.

## Current safe Public-v1 write boundary

- `POST /api/v1/search-timers` requires `active:false` **explicitly**
  and `confirmed:true`. Any omitted `active` or `active:true` is 422
  before durable admission. The productive callback also checks.
- A successful `202` represents a durable one-shot **CREATE** operation,
  not automatic activation and not a recording forecast.
- `GET /api/v1/search-timers/draft-preview` is bounded read-only EPG
  matching. The reported `totalCount` counts matched EPG events, not
  eligible VDR recordings after epgsearch repeat rules, channel filtering,
  history, availability, and recording policy.
- Reference clients should perform the draft preview and require explicit
  acknowledgement before inactive staging.
- Public-v1 `UPDATE`, `ACTIVATE`, `DEACTIVATE`, and `DELETE` remain
  **closed**. They must not be implemented by passing only a public ID through
  `PublicSearchTimerIdentityRepository` to a native ID and then calling
  the legacy RESTfulAPI write. A fingerprint, timestamp, two separate GETs,
  or SQLite lock is **not** an atomic native generation proof.

## Required provider capability for safe mutations

Before exposing Public-v1 modifications:

1. Establish an **immutable per-rule generation/nonce** that changes on
   deletion and recreation, including after daemon/plugin restart and native
   numeric-ID reuse. It must not derive from the mutable search text.
2. Provide native atomic `UPDATE`/`TOGGLE`/`DELETE` semantics that
   compare this generation at the same serialization point as the write.
   A read-then-RESTfulAPI-POST is vulnerable to the time-of-check/time-of-use
   race, even if the rule is re-read immediately.
3. Bind actor, exact backend ID, backend generation, public rule ID,
   native rule generation, full expected revision/ETag, action family, and
   request fingerprint to one durable idempotent MutationOperation.
   Native write must be single-attempt; after ambiguous transport outcomes,
   do not guess success or automatically retry.
4. For activation, require an explicit operator decision after a fresh,
   bounded preview. Document that preview is EPG-only and not guaranteed
   to equal the final number of recordings. Apply a separate activation grant,
   keep it distinct from `searchtimers.create` and `searchtimers.view`.
5. After write, require authoritative exact-generation readback (or verified
   absence for deletion). Persist the completed operation and invalidate
   old public identities where appropriate.
6. On missing generation support, offline backend, malformed inventories,
   stale revisions or mismatched generations: fail closed with an explicit
   classified error; **never fall back to the legacy unversioned endpoint**
   or silently turn 503 into an empty inventory.

## Versioned native CAS policy seam (mock-only)

`core/vdr/include/SearchTimerNativeCasContract.h` and its matching
implementation now define a non-HTTP native provider interface and an
explicit fail-closed entry point for UPDATE / ACTIVATE / DEACTIVATE /
DELETE. The interface requires a *native* `supportsAtomicGenerationCasV1`
capability and an atomic reply carrying the same immutable rule generation
and an authoritative new revision (or exact removal). It rejects malformed
public IDs, numeric IDs, generations, revisions, durable-operation claims,
operator confirmation and backend authorization **before any provider call**.
A native transport exception or an unproven post-write acknowledgement is
classified as `outcomeUnknown` and is never retried within the call.

**This is an internal protocol prerequisite only**, tested with a fake
provider. It is not bound to a public endpoint, is not backed by a native
CAS-capable epgsearch provider and does not prove persistent idempotency by
itself. The calling runtime must first verify a durable operation CAS,
its actor and payload fingerprint, and the epgsearch plugin itself must
supply the atomic generation/revision proof. Old SVDRP `EDIS`/`MODS`/
`DELS`, RESTfulAPI numeric-ID writes and the older epgsearch Services
interface **must never** implement or advertise this interface. Public-v1
mutation routes remain closed pending Issue #518.

## Required regression matrix

- Native ID `0` deleted/reused for a distinct and an **identical** rule.
- Old public ID attempted against a new native generation: **no write**.
- Concurrent external deletion/recreation between preflight and write.
- Deactivated rule toggled; unchanged rule revision vs configuration change.
- Backend reconnect/restart, out-of-order readback, corrupt provider reply.
- Two callers racing the same idempotency key or expected ETag.
- Transport loss before/after a write: `outcome_unknown`, not duplicate write.
- Backend-scoped Device authorization negative cases, revocation and auditing.
- No change to production yaVDR during CI; controlled hardware acceptance is
  a separate deployment gate, preceded by a reviewed database backup decision.

This is a **hard safety prerequisite**, not a suggestion to reopen
Public-v1 write endpoints based on a stored native numeric ID alone.
