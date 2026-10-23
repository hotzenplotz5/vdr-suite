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

## Implemented Public-v1 safety boundary (source; hardware acceptance pending)

- `POST /api/v1/search-timers?backend=<id>` requires explicit `active:false`,
  `confirmed:true`, a scoped Device `searchtimers.create` grant, and a
  durable, idempotent single-attempt CREATE. Missing or active state is 422.
  Historical queued active CREATE operations fail before dispatch.
- `GET /api/v1/search-timers/{searchTimerId}/version?backend=<id>`
  reads a native durable `sr_` revision as a strong ETag, only for the
  exact authorized Device/backend. The native `sg_` generation and numeric
  epgsearch ID remain private.
- `POST /api/v1/search-timers/{searchTimerId}/mutations?backend=<id>`
  accepts confirmed `update`, `activate`, `deactivate` and `delete`,
  strong quoted `If-Match`, `Idempotency-Key` and the independently
  granted action permission: `searchtimers.modify`, `searchtimers.execute`,
  or `searchtimers.delete` on that backend.
- UPDATE changes only the query of an **inactive** rule. The local native
  payload builder retains the full existing epgsearch settings; unsupported
  legacy encoding is rejected rather than guessed.
- The JS reference client asks for explicit fresh draft-preview acknowledgement
  before ACTIVATE. Preview counts matched EPG events, **not** recordings.
  The HTTP server requires explicit confirmation but does not claim to prove
  the user saw a preview; do not advertise preview as server-enforced.
- All mutations fail closed without the specifically patched native epgsearch
  capability, matching SuiteBridge ABI, online writable backend and fresh
  Agent generation. No legacy RESTfulAPI or SVDRP mutation fallback exists
  for this versioned management path.

## Native generation, exactly-once and readback contract

The pinned epgsearch patch series provides persisted per-rule generation and
revision metadata. Each CAS Service compares these values while holding the
native rule lock, then performs at most one durable native change. Deletion and
recreation rotate generation even when the numeric native ID is reused.

The daemon first resolves the *public* opaque ID to an active backend-bound
binding and obtains the native generation/revision via the protected local
Agent/SuiteBridge control plane. It persists an immutable request fingerprint
and operation in the existing SQLite `MutationOperationRepository`, claims
`accepted -> dispatching` exactly once and sends only one CAS invocation.
A replay, even after the rule has changed or disappeared, returns the original
operation and does not send another native write.

SuiteBridge separately calls the native locked `observe-v1.1` Service after
the CAS acknowledgement. Only exact generation plus resulting revision, or
the verified absence of the deleted generation, may return
`applied_verified`. The durable state becomes `succeeded` only after that
proof. Attested no-write revision/generation mismatch becomes
`failed_verified`; transport loss, uncertain persistence, malformed
readback or inability to finish the operation remains `outcome_unknown` or
the appropriate unverified state. **Never retry the provider write** for an
unknown outcome.

The public catalogue still uses conservative signature-based IDs for read
reconciliation; it cannot independently prove identity through a
same-ID identical delete/recreate between reads. That observation is not a
mutation authorization source. Only the native generation-fenced CAS is.

The native source-compile CI, SuiteBridge/daemon package builds, ABI guard and
HTTP/SQLite security regression must all pass on the *same* release HEAD.
The real VDR was **not** modified by this S1 source work. Real end-to-end
authorization, restart, persistence, rollback and negative-race acceptance
remain deferred until **all API-v1 components** have been integrated, under
the single checklist `docs/development/public-v1-final-hardware-acceptance.md`.

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
