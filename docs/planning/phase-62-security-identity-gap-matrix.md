# Phase 62 Security and Identity Gap Matrix

Status: **completed Phase 62 closeout matrix**

```text
Repository baseline:
cb77ff66e11dca7db2eafa36525762dcde35102d

Accepted real-runtime slices:
Slice 1 through Slice 2X

Final accepted runtime head:
4762583d5b5170866838ed9f03b928adbf39f99e

Final runtime marker:
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS

Final runtime evidence:
/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51

Phase 62 state:
completed

PR #117:
open, Draft, unmerged
```

## Necessity rule

A future item becomes implementation work only when all four gates hold:

1. binding requirement;
2. concrete gap in accepted code;
3. distinguishable failure or security consequence;
4. smallest change closing exactly that gap.

General usefulness or a roadmap mention is not proof.

## Final accepted Slice 2X closure

The final mandatory gap was protected business-mutation outcome evidence.

For every already-protected authorized POST reaching the router:

```text
HTTP 200..299  -> operation.succeeded / succeeded
all other HTTP -> operation.failed    / failed
reason_code    -> http_status_<decimal status>
```

The real yaVDR pass proved exact HTTP 200 and HTTP 500 event pairs with continuous actor, device, session, authentication, permission, backend, action, operation, request and correlation context. It also proved secret-free persistence, isolated database execution, cleanup, removed systemd override and active final service.

See [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md).

## Final gap decisions

| Candidate | Final Phase-62 decision |
|---|---|
| Protected mutation response outcomes | **Completed and real-runtime accepted as Slice 2X** |
| Protected audit HTTP read | **Not required for Phase 62** |
| Audit export/filter/redaction/retention | **Deferred until a concrete consumer exists** |
| Generic security administration | **Deferred until a required operator workflow exists** |
| Native/service credential lifecycle | **Deferred to a real client/agent requirement** |
| Universal revisions/idempotency | **Must be justified per resource** |
| Transactional Outbox | **Not proven as the minimal closing change** |
| Compatibility retirement | **Explicitly decided: retain transitional Legacy Basic; remove only through a future deployment-migration contract** |

## Final security-area matrix

| Security area | Accepted Phase-62 result | Remaining decision |
|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | None for Phase 62 |
| Authentication | Legacy Basic compatibility, optional Managed Basic and browser sessions with strict precedence and lifecycle binding | Legacy retirement deferred to migration contract |
| Browser sessions | Atomic issue/logout, CSRF, absolute/idle expiry, concurrency and terminal cleanup | Optional administration later |
| Grants and roles | Exact actor/backend grants and fixed exact-scope Admin/Read-only roles | Generic administration optional |
| Central authorization | Every registered central POST protected or explicitly Safe POST | None |
| Pre-dispatch accountability | Append-only allow/deny evidence; required failure prevents dispatch | None |
| Lifecycle outcomes | Issue, revoke and cleanup outcomes accepted | None |
| Business mutation outcomes | Success/failure response outcomes accepted through Slice 2X | Generic cross-system atomicity not claimed |
| Revisions/idempotency | Resource-specific mechanisms only | Prove need per resource |
| Native/service clients | Actor types represent agents/services | Enrollment belongs to a concrete later phase |
| Audit product | Append-only persistence and evidence inspection exist | HTTP product deferred |
| Compatibility retirement | Readiness evaluated and decision recorded | Transitional mode retained |

## Phase 62 exit criteria

All binding exit criteria are satisfied:

- different actors can hold different rights on the same backend;
- every protected route denies server-side;
- browser sessions are securely issued, expired, revoked and CSRF-protected;
- the read-only/second-house scenario is represented by exact scoped rights;
- every privileged mutation has actor, decision and outcome evidence;
- required pre-dispatch accountability failure prevents dispatch;
- revision/idempotency is required only by concrete resource contracts;
- agent identities are representable for Phase 63;
- compatibility-retirement readiness is explicitly decided.

## Compatibility-retirement closeout

Immediate Legacy Basic removal is not ready because `legacy-basic` remains the code default and packaged configuration does not mandate operator migration to `enforced`.

Retaining the mode is an explicit transitional compatibility decision, not an unclosed security gap. A later removal must prove rollout, recovery and compatibility impact.

## Exact next action

1. Require all five CI jobs green on the final closeout documentation head.
2. Obtain explicit approval before PR #117 metadata changes or merge.
3. Begin Phase 63 only under a new bounded contract.

Do not invent another Phase-62 slice.
