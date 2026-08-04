# VDR-Suite Current Project Status

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main baseline:
89b023ca6758f7ba8f08f75831c2ccdba77a0b08

Merged foundation:
PR #135 - Add manual recording metadata search and assignment
Squash commit: 89b023ca6758f7ba8f08f75831c2ccdba77a0b08
Final source head: 37b06f6e97ee00cefd8b6704f6cd6ed1cf9d2be7
Source CI: VDR-Suite CI #7144, run 30941248988, all five jobs successful
Real yaVDR acceptance: repeated recording-folder navigation passed

Active bounded post-Phase-62 feature:
PR #136 - Add manual recording cast ingestion and search integration
Branch: agent/manual-recording-cast-search
State: open Draft; implementation and validation in progress

Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Phase 62 state:
completed and merged through PR #117

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
none; Phase 63 has not started

Phase 63-67 runtime:
not advanced
```

The active PR #136 is not Phase 63. It is an explicitly approved, limited continuation of the manual Recording metadata workflow built on merged PR #135.

## Final accepted Phase 62 runtime

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

This remains the durable historical Phase-62 completion evidence for its accepted candidate. It is not byte-for-byte acceptance of later daemon builds and is not rewritten by the current feature.

## Completed Phase 62 scope

- persistent actor, device, session and credential identity;
- Legacy Basic compatibility, optional Managed Basic and browser sessions;
- strict credential precedence and lifecycle resolution;
- exact actor permission/backend grants and fixed exact-scope roles;
- protected central mutations and explicit Safe POST classification;
- browser-session issue/logout, CSRF, lifetime, issuer binding, concurrency, idle expiry and terminal cleanup;
- append-only pre-dispatch accountability;
- browser lifecycle outcomes;
- protected mutation success/failure outcomes with non-secret context continuity;
- guarded CI and real-yaVDR acceptance evidence.

## Completed post-Phase-62 work

- PR #118 corrected TVScraper genre classification, snapshot consistency and low-latency continuation.
- PR #123 corrected EPG artwork resolution beneath public base paths.
- PR #132 added guarded series-artwork fallback, TVmaze/TMDB providers, secure backend settings, provider cache/materialization/cleanup, TVScraper series identity preservation and cover/poster preference.
- PR #135 added backend-only manual movie/series/season/episode candidate selection, immutable evidence, relationship-locked assignments, revision-safe replacement/withdrawal and bundled folder readback.
- Direct commits `96b97378` and `2d04a963` corrected and tested channel-detail text layout beside artwork.

PR #135 was merged only after its exact final head and all five jobs of CI #7144 were reverified. The real yaVDR acceptance specifically confirmed that repeated folder/subfolder/back/reopen navigation remained fast after the N+1/schema-loop correction.

## Active post-Phase-62 feature: manual cast and search

Draft PR #136 extends only manually selected TMDB movies:

- credits are loaded backend-side only after one exact movie is selected;
- movie and cast persistence is atomic;
- a valid empty cast is distinct from provider failure;
- people are canonical Suite-owned metadata entities with provider-qualified TMDB person IDs;
- recording-person relations belong to the concrete assignment revision;
- reassignment and withdrawal preserve history while changing the active read model;
- existing person search and global search consume locally persisted manual titles, original titles and actors;
- automatic TVScraper people become effective again after withdrawal;
- ordinary recording, folder and search GETs never call TMDB;
- set-based reads and SQLite trace tests guard against one query per recording/person and repeated schema DDL.

Architecture and validation contracts:

- [ADR-0052](../adr/ADR-0052-manual-recording-cast-ingestion-search.md)
- [Manual Recording Cast Ingestion and Search Integration](manual-recording-cast-search.md)
- [Backend-Scoped Global Search](../architecture/global-search.md)

PR #136 must remain Draft until complete green CI on one exact final head and successful real yaVDR assignment/search/restart/reassignment/withdrawal acceptance.

## Security review

No authentication, authorization, CSRF, fixed Read-only-role or cross-backend-write contract is intentionally changed.

Manual movie cast acquisition reuses:

- route-authoritative backend scope;
- permission `metadata.recording.assign`;
- browser CSRF validation;
- protected-mutation accountability;
- the existing managed backend TMDB credential resolver.

Read-only, wrong-backend and invalid-CSRF requests must fail before provider access. Tokens, provider URLs, private paths and actor references must not enter public recording detail or search results.

Because post-Phase-62 work changes daemon behavior, the original Phase-62 runtime fingerprint remains historical rather than current. See [Post-Phase-62 Security Review](post-phase-62-security-review.md).

## Compatibility-retirement decision

Legacy Basic remains an explicitly transitional deployment mode. `enforced` is the fail-closed target. Removal requires a future deployment-migration contract and is not unfinished Phase-62 work.

## Development rules

- Root-level `AGENTS.md` remains binding.
- Verify the current `main`, branch, PR and CI state before repository changes.
- Evaluate CI only for the exact final feature head.
- Do not treat historical acceptance hashes as proof for changed daemon fingerprints.
- Do not start Phase 63 while PR #136 is the active explicitly approved feature block.
- Do not modify TVScraper upstream or write into TVScraper-owned databases/caches.
- Never commit or print credentials, cookies, CSRF secrets, provider tokens or secret-bearing process environments.
- Keep PR #136 Draft until the user explicitly approves readiness after real-system testing.

## Exact next action

1. Complete focused code, SQL, REST, security, detail, frontend and documentation validation for Draft PR #136.
2. Obtain all five successful VDR-Suite CI jobs on one exact final PR head, including production daemon build and packaging/install staging.
3. Update the Draft PR body with the exact head, CI evidence, architecture, security and performance contracts.
4. Install that exact head on yaVDR and execute the documented cast/title/person/restart/reassignment/withdrawal checklist.
5. Keep Phase 63 unstarted until this bounded feature is either merged after explicit approval or otherwise closed.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Manual Recording Metadata Assignment](manual-recording-metadata-assignment.md)
- [Manual Recording Cast Ingestion and Search Integration](manual-recording-cast-search.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
- [Agent Workflow Rules](../../AGENTS.md)
