# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Repository, pull-request and runtime facts must be checked against the current `main` branch; do not repeat historical acceptance work without a directly relevant runtime change.

## Canonical reading

- [Current State](CURRENT.md)
- [Current Project Status](development/current-status.md)
- [Post-Phase-62 Security Review](development/post-phase-62-security-review.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](adr/index.md)
- [Agent Workflow Rules](../AGENTS.md)

## Stable project position

```text
Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 repository state:
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

There is no active Phase-62 PR or Phase-62 branch workflow. PR #117 was merged as `f9e5f88bc223a2ce8a30fdbf4596893b34bc1551`.

## Current post-Phase-62 baseline

The latest runtime/frontend baseline before this documentation refresh is:

```text
main @ 2d04a963054e9925f6b8cb12392b188a89e11f07
```

Relevant completed post-Phase-62 work:

- PR #118: TVScraper classification and refresh corrections;
- PR #123: public-base-path-safe EPG artwork resolution;
- PR #132: guarded series-artwork fallback with TVmaze/TMDB, secure per-backend settings, TVScraper identity preservation and poster/cover preference;
- direct channel-detail layout correction `96b97378` plus regression test `2d04a963`.

PR #132 was merged as:

```text
441e5febf7d3ab0121a585ce1176a8e5a7c67ce0
```

Its final feature head passed VDR-Suite CI #6982 with all five jobs successful. Real yaVDR operation additionally proved persisted `provider=tmdb` fallback assets and successful frontend delivery. The channel-detail text-layout correction was installed and confirmed in the browser.

## Phase 62 completion evidence

The historical Phase-62 runtime acceptance remains the durable completion evidence for its accepted candidate:

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
installed_daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
configuration_sha256=8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

This evidence closes Phase 62. It is historical evidence for that accepted runtime fingerprint, not a claim that every later daemon build is byte-identical.

## Current security position

The post-Phase-62 series-artwork settings POST is integrated into the Phase-62 protected-mutation model: scoped permission, Read-only denial, browser CSRF, pre-dispatch accountability and success/failure outcomes remain active. Managed TMDB tokens are stored in a private secret directory and are not returned by the API or copied into accountability events.

No known authentication, authorization, CSRF, Read-only-role or cross-backend write bypass was introduced by PRs #118, #123 or #132. See the dedicated [Post-Phase-62 Security Review](development/post-phase-62-security-review.md) for the evidence boundary and the small remaining audit-scope hardening recommendation.

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and intentionally retained. `enforced` mode is the fail-closed target. Removing Legacy Basic requires a separate deployment-migration contract and is not unfinished Phase-62 work.

## Current work boundary

- Phase 62 is completed and must not be reopened merely to add optional security administration, audit products, universal idempotency, generic Outbox infrastructure or speculative credential lifecycle.
- Phase 63 begins only with a new bounded contract.
- TVScraper remains upstream and unchanged; VDR-Suite integrates through `vdr-plugin-suite-bridge`.
- External series-artwork fallback requires a deterministic series identity and does not perform title/fuzzy search.
- Unknown central POST routes remain subject to the Phase-62 fail-closed policy outside explicit Legacy Basic compatibility.

## Exact next action

Keep `main` stable, finish the bounded route-derived audit-scope hardening and dedicated settings-mutation security tests, then refresh the post-Phase-62 security evidence. Start Phase 63 only after a separate approved contract.

## Credential and secret restrictions

Never print, store or commit Authorization headers, plaintext passwords, password hashes, cookies, CSRF tokens, raw session/verifier secrets, TMDB tokens, secret-bearing login responses or process environments.
