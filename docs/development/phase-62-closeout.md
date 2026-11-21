# Phase 62 — Identity, RBAC and Accountability Foundation Closeout

## Status

**Phase 62 is completed and merged.**

PR #117 was merged into `main` as:

```text
f9e5f88bc223a2ce8a30fdbf4596893b34bc1551
```

Phase 62 established production-grade Suite actor identity, scoped server-side authorization, browser-session lifecycle policy, CSRF enforcement and append-only accountability for protected mutations. Phase 63-67 runtime was not advanced.

## Final accepted Phase-62 runtime

The final runtime-changing Phase-62 slice is [Slice 2X — Protected Mutation Response Outcomes](phase-62-slice-2x-runtime-closeout.md).

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
configuration_sha256=8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

The final closeout head `75787cf61cc123f111b421a261ecd535068a6789` passed VDR-Suite CI #6920, Run ID `30755925693`, with all five jobs successful before merge.

This acceptance is the durable completion evidence for the accepted Phase-62 candidate. It is intentionally historical: later daemon changes require their own regression and runtime evidence and do not rewrite this fingerprint.

## Completed exit criteria

Phase 62 proves:

- persistent canonical actor, device, session and credential identity;
- request and correlation context across authentication, authorization and accountability;
- distinct rights for different actors on the same backend through exact permission and backend scopes;
- fixed exact-scope Admin and Read-only role semantics;
- server-side denial for every protected central mutation route;
- an explicit Safe POST inventory for non-mutating POST contracts;
- strict browser-cookie precedence with no fallback from a presented invalid browser credential;
- atomic browser-session issue and logout with independent session and CSRF secrets;
- immutable absolute lifetime, optional idle expiry, bounded concurrency and terminal retention cleanup;
- request-time issuing-credential lifecycle binding;
- server-owned CSRF enforcement for browser mutations and memory-only frontend CSRF ownership;
- append-only pre-dispatch allow/deny evidence that fails closed before dispatch;
- browser lifecycle completion evidence;
- protected business-mutation success/failure outcome evidence with continuous actor, decision, operation, request and correlation context;
- agent/service actor representation sufficient for Phase 63 design without implementing Phase 63 runtime;
- guarded real-yaVDR acceptance, rollback tooling and durable non-secret evidence.

## Compatibility-retirement decision

Compatibility retirement was explicitly evaluated after the mandatory Slice-2X pass.

**Decision: retain Legacy Basic compatibility as a transitional deployment mode at Phase-62 closeout.**

Immediate removal was not ready because:

- `legacy-basic` remained the code default when `VDR_SUITE_SECURITY_MODE` was not explicitly configured;
- packaged daemon defaults did not yet require an operator migration to `enforced`;
- removal without a deployment migration would change existing installation authentication behaviour rather than merely delete dead code.

This does not leave an unclosed Phase-62 security gap. Managed Basic and browser sessions use the common persistent identity and authorization model, and `enforced` mode provides the fail-closed target behaviour.

Retirement requires a separate future migration contract proving configuration rollout, operator recovery and compatibility impact. It must not be smuggled into Phase 63 or performed as unreviewed cleanup.

## Explicitly deferred work

The following were evaluated and are not required to close Phase 62:

- protected HTTP audit reader, export, filtering, redaction or retention product;
- generic actor, identity, credential, grant or role administration API/UI;
- native/service credential enrollment, rotation and revocation before a concrete client requires them;
- universal revision, `If-Match`, idempotency or durable operation infrastructure;
- transactional Outbox or generic cross-system commit coupling;
- Android, Android TV or any Phase 63-67 runtime.

Each requires its own binding requirement, accepted-code gap, concrete failure and smallest closing change.

## Post-closeout changes

Later completed PRs and direct commits do not reopen Phase 62 merely because they use its security framework.

The most security-relevant later change is PR #132, which added a protected per-backend series-artwork settings POST. It uses a dedicated scoped permission, fixed Admin/Read-only semantics, browser CSRF, pre-dispatch accountability and post-dispatch outcome evidence. Managed provider tokens are stored privately and are not returned by the API or written to accountability events.

No known Phase-62 guarantee was bypassed by PRs #118, #123 or #132. Because PR #132 changed daemon routing and protected-mutation handling, the historical acceptance above must not be described as a byte-for-byte acceptance of the later daemon. The evidence boundary and recommended focused hardening are recorded in [Post-Phase-62 Security Review](post-phase-62-security-review.md).

## Next strict runtime phase

```text
Phase 63 - Backend Agent and Secure Multi-Site Runtime
```

Phase 63 remains planned and must begin with its own bounded contract. No Phase-63 runtime is implemented by this closeout or the completed post-Phase-62 artwork work.

## Related documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](current-status.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
