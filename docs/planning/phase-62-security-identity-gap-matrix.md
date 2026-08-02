# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 implementation and closeout matrix

```text
Repository baseline:
cb77ff66e11dca7db2eafa36525762dcde35102d

Accepted real-runtime slices:
Slice 1 through Slice 2W

Accepted Slice-2W source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Accepted Slice-2W runtime marker:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Current bounded slice:
Slice 2X - Protected Mutation Response Outcomes

Slice 2X state:
production implementation complete;
focused tests and architecture guard complete;
isolated installation/runtime harness complete;
real yaVDR acceptance pending.

PR #117:
open, Draft, unmerged
```

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Source-head proof
alone is insufficient.

## Necessity rule

A remaining idea is Phase-62 implementation work only when all four gates hold:

1. binding Phase-62 requirement;
2. concrete gap in accepted code;
3. real distinguishable failure or security consequence;
4. smallest change that closes exactly that gap.

A roadmap mention or general usefulness is not proof.

## Slice 2X necessity and implemented closure

### Binding requirement

```text
every privileged mutation has actor, decision and outcome evidence
```

### Prior accepted-code gap

`SecurityHttpGate::appendDecisionEvent()` recorded only pre-dispatch
`dispatch_authorized`/`dispatch_denied`. `TestHttpServer` then returned the
router result without a protected business-mutation outcome event.

### Concrete failure

An authorized success and an authorized returned router/backend/domain failure
left indistinguishable accountability evidence.

### Implemented minimal closure

For every already-protected, authorized POST that reaches
`ApiRouter::handleClientPost()`:

```text
HTTP 200..299  -> operation.succeeded / succeeded
all other HTTP -> operation.failed    / failed
reason_code    -> http_status_<decimal status>
```

The event reuses the existing authorization context. No route, permission, role,
schema, repository, configuration, frontend or packaging owner was added.

Binding documents:

- [Slice 2X Contract](../development/phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2X yaVDR Runbook](../development/phase-62-slice-2x-runtime-acceptance-runbook.md)

## Candidate proof table

| Candidate | Binding requirement | Demonstrated failure | Smallest justified result | Current decision |
|---|---|---|---|---|
| Protected mutation response outcomes | Explicit actor/decision/outcome exit criterion | Allowed protected success and returned failure were indistinguishable | One post-router event using existing context/repository | **Implemented as Slice 2X; runtime acceptance pending** |
| Protected audit HTTP read | No exit criterion requires an HTTP reader | No runtime/security failure from its absence | None | **Not necessary** |
| Audit export/filter/redaction/retention | No current consumer or acceptance requirement | No demonstrated failure | None | **Not necessary** |
| Generic security administration | Current identities/grants/roles can be provisioned without it | No required operation blocked | None until a real operator workflow exists | **Not necessary now** |
| Native/service credential lifecycle | Agent identity is representable in the transport-neutral model | No current Phase-62 client requires enrollment/rotation | Defer to the real consumer phase | **Not necessary now** |
| Universal revisions/idempotency framework | Required only where a resource contract proves need | No specific unsafe accepted resource shown | Prove per resource | **Not proven** |
| Transactional Outbox | Improves crash consistency but is not itself the exit criterion | No concrete current failure proves it is the minimal fix | Separate failure proof required | **Not proven** |
| Compatibility retirement | Legacy compatibility is transitional | Readiness not yet evaluated after mandatory outcome acceptance | Evaluate after Slice 2X runtime pass | **Next closeout decision** |

## Gap matrix

| Security area | Current accepted/implemented state | Proven remaining gate | Unproven or later work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context accepted | None demonstrated | Production administration only for a real workflow |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions; strict precedence, issuer binding, absolute/idle expiry and retention accepted | Compatibility-retirement readiness after Slice 2X acceptance | Native/service lifecycle only with a concrete client |
| Browser sessions | Atomic issue/logout, independent secrets, replay denial, outcomes, concurrency, idle expiry and bounded cleanup accepted | None | Listing/logout-all/admin optional |
| Grants and fixed roles | Exact actor grants and exact-scope Admin/Read-only roles accepted | None | Generic role/grant administration optional |
| CSRF | Enforced for accepted browser mutations; memory-only frontend ownership | Preserve behavior | No new CSRF feature |
| Central authorization | Every registered central POST protected or explicitly Safe POST | None | No additional migration slice |
| Pre-dispatch accountability | Actor/decision/allow-deny append-only and fail closed | Preserve behavior | No audit reader selected |
| Browser lifecycle outcomes | Issue/revoke/cleanup outcomes accepted | None | No broader lifecycle work selected |
| Business mutation outcomes | Source implementation now records protected router result | **Real yaVDR Slice-2X acceptance** | Cross-system crash atomicity remains unproven |
| Revisions/idempotency | Domain-specific partial mechanisms | None proven | Must be justified per resource |
| Security administration | No general production management API | None proven | Select only for a concrete operator need |
| Native/service clients | Core model represents service/agent actors | None proven for Phase 62 | Phase/client-owned lifecycle later |
| Audit product | Append-only persistence and test/runtime inspection exist | None proven | HTTP read/export/redaction/retention not selected |
| Compatibility retirement | Legacy compatibility remains transitional | Evaluate after Slice-2X runtime closeout | Final Phase-62 decision |

## Source and harness state

Implemented owners:

- `SecurityGateDecision` retains authorization decision and operation ID;
- `SecurityHttpGate` appends the exact outcome event;
- `TestHttpServer` calls the outcome path after POST dispatch and before final
  response;
- `AccountabilityEventRepository` remains append-only.

Validation owners:

- focused `test_security_http_gate.cpp` coverage;
- `check_architecture.py` order/scope/secret-source guard;
- `protected-mutation-outcome-runner.py` for exact event-pair proof;
- `protected-mutation-outcome-runtime-entry.py` for backup, atomic install,
  isolated dual-database override, rollback and final service restoration;
- `phase-62-slice-2x-runtime-acceptance-runbook.md` for the bounded yaVDR call.

The earlier implementation/harness head
`4b61583b604626cd49e213356241759c81e60d04` passed VDR-Suite CI #6871, Run ID
`30750871845`, with all five jobs successful. The later isolated runtime-entry
fingerprint requires a fresh complete CI run on the final stabilization head
before runtime installation.

## Runtime acceptance gate

The real yaVDR pass must prove:

- exact clean branch/head and candidate hash;
- old runtime evidence backup;
- both runtime database paths bound to an isolated scenario copy;
- one protected HTTP 200 with exact `operation.succeeded` pair;
- one deterministic protected HTTP 500 with exact `operation.failed` pair;
- context continuity and secret-free persistence;
- scenario test row/trigger removal, grant restoration and browser-session
  revocation;
- production database unchanged during the scenario;
- unchanged loader/configuration;
- removed systemd override;
- candidate retained only after a complete pass;
- old daemon restored after failure;
- normal production service active at the end.

## Phase 62 dependency order

1. Identity and authorization foundation — accepted.
2. Persistent lifecycle, browser sessions, exact grants and fixed roles — accepted.
3. Protected central mutations and Safe POST inventory — accepted.
4. Absolute lifetime — Slice 2R accepted.
5. Browser issue/revoke outcomes — Slice 2S accepted.
6. Issuer binding — Slice 2T accepted.
7. Concurrency limit — Slice 2U accepted.
8. Idle expiry/activity throttle — Slice 2V accepted.
9. Terminal retention cleanup — Slice 2W accepted.
10. Necessity proof — complete.
11. Protected Mutation Response Outcomes — Slice 2X implemented; real runtime acceptance pending.
12. Compatibility-retirement readiness and final Phase-62 closeout — only after Slice 2X acceptance.

No other implementation item is currently proven necessary.

## Exact next action

1. Complete canonical documentation consistency.
2. Require all five CI jobs green on the final head containing the isolated
   runtime-entry fingerprint.
3. Run the bounded Slice-2X yaVDR acceptance.
4. On pass, record the runtime closeout.
5. Evaluate compatibility-retirement readiness and final Phase-62 closeout only.

Do not reopen Slice 2W without a changed relevant fingerprint. Do not implement
an audit reader, administration API, Outbox, generic operation framework,
native/service lifecycle, Android, Android TV or Phase 63-67 runtime.
