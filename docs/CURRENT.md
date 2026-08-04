# VDR-Suite Current State

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Manual Recording Cast Feature](development/manual-recording-cast-search.md)
- [Post-Phase-62 Security Review](development/post-phase-62-security-review.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](adr/index.md)
- [Documentation Index](index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Current branch authority: main
Current merged main baseline:
89b023ca6758f7ba8f08f75831c2ccdba77a0b08

Merged foundation:
PR #135 - Add manual recording metadata search and assignment
Final head: 37b06f6e97ee00cefd8b6704f6cd6ed1cf9d2be7
CI: #7144 / 30941248988, all five jobs successful
Real yaVDR repeated-folder-navigation acceptance: successful

Active bounded post-Phase-62 feature:
PR #136 - Add manual recording cast ingestion and search integration
Branch: agent/manual-recording-cast-search
State: open Draft; not approved for Ready or merge

Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Previous completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Phase 62 state:
completed and merged through PR #117

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
none; Phase 63 is planned but not started

Phase 63-67 runtime:
not advanced
```

PR #136 is a limited continuation of the manual recording metadata workflow and is not Phase 63.

## Completed post-Phase-62 work

The current platform additionally includes:

- VDR Remote and Live Overlay hardening (#110);
- Backend-scoped Global Search (#111);
- TVScraper genre-classification and refresh corrections from PR #118;
- EPG artwork resolution under public base paths from PR #123;
- guarded series-artwork fallback from PR #132;
- TVmaze and TMDB provider integration without browser access to provider credentials;
- secure per-backend series-artwork settings and managed TMDB-token storage;
- deterministic TVDB/TMDB series identity preservation through SuiteBridge;
- series/season cover preference before episode images;
- poster-first TMDB selection with deterministic backdrop fallback;
- frontend correction keeping channel-detail text beside artwork on wide layouts;
- merged PR #135 with backend-only manual movie/series/season/episode candidate selection, immutable evidence, revision-safe relationship-locked assignments and withdrawal;
- bundled manual-assignment folder readback that removed the N+1/schema-loop navigation regression.

PR #135 was merged as `89b023ca6758f7ba8f08f75831c2ccdba77a0b08`. Its exact final feature head passed all five jobs in VDR-Suite CI #7144. Real yaVDR operation confirmed fast repeated recording-folder navigation.

## Active manual cast feature

Draft PR #136 adds selected-movie cast ingestion and local search integration:

- credits are acquired only after one exact TMDB movie is selected;
- candidate search, folder navigation, recording detail and search never fetch credits;
- movie assignment and cast persistence share one transaction;
- valid empty cast and technical provider failure are distinct outcomes;
- canonical Suite-owned person entities deduplicate through provider-qualified TMDB person IDs;
- actor role, character and cast order remain assignment-scoped evidence;
- active manual title, original title and actors participate in existing global and person search;
- active manual people override automatic people only for the affected recording;
- reassignment and withdrawal preserve history;
- withdrawal restores automatic TVScraper/native title and people;
- constant set-based SQL reads and trace tests protect navigation and search from per-record/per-person queries.

Authoritative feature documents:

- [ADR-0052](adr/ADR-0052-manual-recording-cast-ingestion-search.md)
- [Manual Recording Cast Ingestion and Search Integration](development/manual-recording-cast-search.md)
- [Backend-Scoped Global Search](architecture/global-search.md)

The feature is not complete until one exact final PR head has all five CI jobs successful and the real yaVDR assignment, detail, title search, actor search, restart, reassignment and withdrawal checklist has passed.

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

The manual selected-movie mutation continues to use `metadata.recording.assign`, route-authoritative backend scope, browser CSRF and protected-operation accountability. The existing backend-scoped managed TMDB credential resolver is reused. Read-only, wrong-backend and invalid-CSRF requests are denied before provider access. Tokens, actor references, provider URLs and private paths are excluded from public recording-detail and search payloads.

No known Phase-62 security guarantee is intentionally bypassed by the completed or active post-phase work. The old runtime acceptance is historical. See [Post-Phase-62 Security Review](development/post-phase-62-security-review.md).

## Compatibility-retirement decision

Legacy Basic compatibility remains explicitly transitional. `enforced` mode is the fail-closed target. Retirement requires a separate deployment-migration contract and is not unfinished Phase-62 work.

## Current work boundary

- Phase 62 is complete.
- PR #135 is the merged basis.
- PR #136 is the only active approved feature block.
- PR #136 must remain Draft until explicit real-system approval.
- Phase 63 has not started.
- TVScraper remains an unchanged upstream dependency.
- No writes are made to TVScraper-owned databases or caches.
- No Phase-63 runtime contract is changed by PR #136.

## Exact next action

Complete and stabilize Draft PR #136, obtain all five successful CI jobs on one exact final head, update its evidence, then execute the documented real yaVDR cast/title/person/restart/reassignment/withdrawal checklist. Do not start Phase 63 while this approved feature block remains active.
