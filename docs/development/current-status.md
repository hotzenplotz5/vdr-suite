# VDR-Suite Current Project Status

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
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

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
none; Phase 63 has not started

Phase 63-67 runtime:
not advanced
```

PR #117 was merged as `f9e5f88bc223a2ce8a30fdbf4596893b34bc1551`. There is no active Phase-62 PR, Draft or local branch requirement.

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

This is the durable Phase-62 completion evidence for its accepted candidate. It is not a byte-for-byte acceptance of later daemon builds.

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
- Direct commits `96b97378` and `2d04a963` corrected and tested channel-detail text layout beside artwork.

PR #132 was merged as `441e5febf7d3ab0121a585ce1176a8e5a7c67ce0`. Its final head passed VDR-Suite CI #6982 with all five jobs successful. Real yaVDR evidence includes multiple persisted TMDB fallback assets and successful browser rendering.

## Security review

No known authentication, authorization, CSRF, fixed Read-only-role or cross-backend-write bypass was introduced by the post-Phase-62 work.

The new series-artwork settings POST is a protected mutation using `backend.settings.series-artwork.modify`, backend scope, browser CSRF, append-only pre-dispatch accountability and post-dispatch success/failure outcomes. The managed TMDB token is stored privately and is not returned by the API or included in accountability events.

Because PR #132 changed daemon routing and protected-mutation handling, the old Phase-62 runtime fingerprint remains historical rather than current. A focused route-derived audit-scope hardening and dedicated current-runtime settings-mutation security acceptance are recommended strengthening steps. See [Post-Phase-62 Security Review](post-phase-62-security-review.md).

## Compatibility-retirement decision

Legacy Basic remains an explicitly transitional deployment mode. `enforced` is the fail-closed target. Removal requires a future deployment-migration contract and is not unfinished Phase-62 work.

## Development rules

- Root-level `AGENTS.md` remains binding.
- Verify the current `main` head before repository changes.
- Prefer small coherent commits and evaluate CI on the final stabilization head.
- Do not treat historical acceptance hashes as proof for changed daemon fingerprints.
- Do not start Phase 63 without a separate bounded contract.
- Never commit or print credentials, cookies, CSRF secrets, provider tokens or secret-bearing process environments.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can complete the bounded operation safely and the complete current file has been read.

Use local edits first only when the change requires:

- compilation or generated-output inspection before committing;
- a multi-file transformation that cannot be represented safely through bounded GitHub updates;
- a workaround because the GitHub connector blocks a file operation.

Never infer a local checkout path. Repository-relative commands require a verified checkout path and verified repository identity.

## Exact next action

1. Complete route-derived authorization/audit scope for the series-artwork settings POST.
2. Add focused Admin, Read-only, backend-scope, CSRF, mismatch and outcome tests.
3. Refresh post-Phase-62 security evidence.
4. Start Phase 63 only under a separate approved contract.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
- [Agent Workflow Rules](../../AGENTS.md)
