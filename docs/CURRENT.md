# VDR-Suite Current State

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Post-Phase-62 Security Review](development/post-phase-62-security-review.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](adr/index.md)
- [Documentation Index](index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Current branch authority: main
Post-Phase-62 runtime/frontend baseline before this documentation refresh:
2d04a963054e9925f6b8cb12392b188a89e11f07

Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 state:
completed and merged through PR #117

Previous completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed cross-cutting platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
none; Phase 63 is planned but not started

Phase 63-67 runtime:
not advanced
```

PR #117 is merged, not open or Draft. Its merge commit is `f9e5f88bc223a2ce8a30fdbf4596893b34bc1551`.

## Completed post-Phase-62 work

The current platform additionally includes:

- TVScraper genre-classification and refresh corrections from PR #118;
- EPG artwork resolution under public base paths from PR #123;
- guarded series-artwork fallback from PR #132;
- TVmaze and TMDB provider integration without browser access to provider credentials;
- secure per-backend series-artwork settings and managed TMDB-token storage;
- deterministic TVDB/TMDB series identity preservation through SuiteBridge;
- series/season cover preference before episode images;
- poster-first TMDB selection with deterministic backdrop fallback;
- frontend correction keeping channel-detail text beside artwork on wide layouts.

PR #132 was merged as `441e5febf7d3ab0121a585ce1176a8e5a7c67ce0`. Its final feature head passed VDR-Suite CI #6982 with all five jobs successful. Real yaVDR operation proved multiple persisted `provider=tmdb` fallback rows, valid cached image files and browser delivery.

## Final Phase 62 runtime evidence

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

This remains the durable completion evidence for the accepted Phase-62 runtime. Later daemon changes do not reopen the phase, but they are not covered byte-for-byte by this historical fingerprint.

## Current security position

Phase-62 identity, exact backend-scoped authorization, fixed Admin/Read-only roles, browser-session lifecycle, CSRF, fail-closed central POST classification and append-only allow/deny/outcome accountability remain present.

The series-artwork settings POST added after Phase 62 is classified as a protected mutation and uses a dedicated backend-scoped permission. Browser callers require CSRF; Read-only actors are denied; successful and failed authorized responses receive operation outcomes. Managed TMDB tokens are kept beneath the private secret root and are not returned to the browser or written to accountability events.

No known Phase-62 security guarantee is bypassed by the completed post-phase work. The old runtime acceptance is historical; a dedicated current-runtime settings-mutation security acceptance remains useful strengthening. See [Post-Phase-62 Security Review](development/post-phase-62-security-review.md).

## Compatibility-retirement decision

Legacy Basic compatibility remains explicitly transitional. `enforced` mode is the fail-closed target. Retirement requires a separate deployment-migration contract and is not unfinished Phase-62 work.

## Current work boundary

- Phase 62 is complete.
- Phase 63 has not started.
- TVScraper remains an unchanged upstream dependency.
- External series-artwork fallback uses deterministic provider identities only; no title/fuzzy lookup is introduced.
- Optional audit products, generic security administration and universal Outbox/idempotency infrastructure remain outside the completed Phase-62 scope.

## Exact next action

Complete the bounded route-derived audit-scope hardening and dedicated settings-mutation security tests, refresh the post-Phase-62 evidence, and begin Phase 63 only under a separate approved contract.
