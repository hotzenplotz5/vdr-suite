# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Repository, pull-request and runtime facts must be checked against the current `main` branch; do not repeat historical acceptance work without a directly relevant runtime change.

## Mandatory repository orientation at the start of every new chat

Before any substantive analysis, shell command, edit, commit, pull-request action or CI judgement, every new VDR-Suite chat must complete one full GitHub-backed repository orientation pass:

- query the current repository metadata, default branch, current `main` head and every active branch or pull request relevant to the requested work;
- inspect the complete current repository tree and file structure at least once, including the root governance/build files and the locations of `AGENTS.md`, `docs/NEW-CHAT-HANDOFF.md`, build entry points, test entry points, source trees and documentation trees;
- read this `docs/NEW-CHAT-HANDOFF.md` file completely from the current GitHub ref, never from a remembered excerpt or an older chat;
- read `AGENTS.md` completely and read every file currently listed under `Canonical reading` completely; follow the current links rather than an older copied list;
- read the complete contents of every source, build, test and documentation file that is materially relevant to the requested task before changing it; snippet-only or search-result-only understanding is insufficient for an edit;
- publish a visible status update that states the verified `main` SHA, active branch and pull request, confirms that the repository structure and canonical files were reviewed, and identifies the files relevant to the task;
- repeat the affected reads when the relevant branch head, pull-request head or repository structure changes during the chat;
- never use a local prompt, current working directory or path remembered from another chat as a substitute for this GitHub orientation.

No repository command block may be emitted until the canonical local checkout path and repository identity have been explicitly verified. If they are not verified, do not tell the user to run `git`, `make`, repository-relative `python3` commands or installation commands from an arbitrary directory; state that no safe local repository command can yet be supplied.

This orientation is mandatory once at the beginning of every new chat and is not satisfied by relying on previous-chat summaries, memory, partial file excerpts or historical repository state.

## Binding execution rules for every new chat

These rules are mandatory for every assistant continuing VDR-Suite work:

- work GitHub-first and perform repository reads, edits, commits, pushes, pull-request work and CI inspection through the connected GitHub tools whenever they can do so safely;
- do not hand the user downloadable patches, replacement files or shell-command workflows for work that can be completed directly on GitHub;
- continue autonomously through all already-approved steps of the active bounded workstream and do not stop after analysis, after an individual edit or after an intermediate commit;
- always provide visible status updates during every multi-step repository task: send one before substantive work begins and further short updates after each meaningful finding, edit, commit, test or CI-state transition; never work silently through multiple repository operations;
- create and push small coherent commits at meaningful checkpoints, using fast-forward-only history and never force-pushing or rewriting published history without explicit approval;
- use GitHub Actions as the normal repository validation path and do not invent a blanket prohibition on commits before tests; local compilation or focused local tests are only required when GitHub cannot establish the needed fact;
- do not stop until there is a usable, tested Draft PR for the approved workstream, unless a real project-rule, safety, compatibility or decision boundary is reached;
- never invent, guess or silently infer local paths, checkout locations, branches, hosts, users, installation targets, runtime state, test results or command prerequisites; use only values explicitly established by the user, the current repository, connected tools or this binding handoff;
- do not treat a temporary shell prompt, test directory, historical phase checkout or example path as the canonical local repository; the canonical checkout path must be explicitly verified before emitting `cd`, and when it is not verified no repository-relative local commands may be emitted;
- never use `set -e`, `set -o errexit`, `bash -e` or `sh -e` in commands supplied to the user, and never change the error-handling options of the user's interactive shell;
- select GitHub Actions evidence by change impact rather than waiting mechanically for every job: wait only for jobs that can validate files or behavior changed by the current diff, and do not wait for daemon build, packaging, frontend or other unrelated jobs when those components were not changed;
- do not prescribe installation, daemon replacement or service restart when installable runtime, frontend or packaging content did not change and the user did not explicitly request deployment;
- end every final VDR-Suite repository response with a copyable `Lokaler Bau, Test und Installation` command block tailored to the exact active branch and change; include only synchronization, build, test, installation and service commands relevant to the actual diff, and never claim that local commands were executed unless they actually were;
- never mark a Draft PR ready, merge it, close it, change its base or alter review state without explicit user approval.

These requirements reinforce [Agent Workflow Rules](../AGENTS.md). They are not optional chat preferences and must not be replaced by assistant-invented process gates.

## Permanent operational safety invariants

These exact operational safety invariants are append-only and must not be removed, shortened, weakened or overridden by a later handoff refresh, status rewrite, documentation cleanup or assistant preference. A change is permitted only when the user explicitly orders the named rule to be changed.

- At the start of every new chat, complete the full GitHub-backed repository orientation: inspect the current complete file structure, read this handoff and `AGENTS.md` completely, and read every current `Canonical reading` file completely before substantive repository work.
- Always provide visible status updates during repository work; never work silently through multiple repository operations or wait until the final response to report progress.
- Never invent or infer a local checkout path, host, branch, user, installation target or runtime state.
- Never promote a temporary test directory, historical phase checkout or example path to the canonical checkout.
- Never emit repository-relative local commands until the canonical checkout path and repository identity are explicitly verified.
- Never use `set -e`, `set -o errexit`, `bash -e` or `sh -e` in user-facing shell blocks and never mutate the user's interactive shell options.
- Wait only for GitHub Actions jobs that can validate components changed by the current diff; unrelated daemon, packaging, frontend or documentation jobs are not gates.
- Do not prescribe installation, daemon replacement or service restart when installable content did not change and deployment was not explicitly requested.
- Do not claim local execution from GitHub Actions evidence.

The repository handoff consistency guard must fail when any of these invariants is missing or weakened.

## Required final local command block

Every final VDR-Suite repository response must end with a ready-to-copy shell block under the heading `Lokaler Bau, Test und Installation`.

The block must be derived from verified current facts and the current repository targets rather than copied blindly from an old chat.

- Include `cd` only when the canonical checkout path and repository identity are explicitly verified. Never copy a temporary test directory or historical phase path from a shell prompt.
- When the canonical checkout path is not verified, do not emit `git`, `make`, repository-relative `python3`, installation or service commands. State explicitly that no safe local repository command block can be supplied until the checkout is identified.
- Include `git fetch origin`, switching to the exact active branch and a fast-forward-only pull only when branch synchronization is relevant and the verified checkout is already selected.
- Do not use `set -e`, `set -o errexit`, `bash -e` or `sh -e`; keep commands as separate interactive-shell-safe lines.
- Include only build, focused test, full test, install-staging, installation and service commands that are relevant to the current diff and requested local validation.
- Do not require `make daemon`, daemon installation or service restart when daemon/runtime/installable content did not change.
- Do not require waiting for every GitHub Actions job. Record which jobs are relevant to the changed files and stop waiting once the required jobs are conclusively successful or failed.
- When installation is not relevant, state that no installation or service restart is required instead of emitting those commands.

Commands that were only supplied for the user to run must be described as such. A GitHub Actions result must never be presented as proof that these local commands were executed on the user's machine.

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

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
none; Phase 63 is planned but not started

Phase 63-67 runtime:
not advanced
```

There is no active Phase-62 PR or Phase-62 branch workflow. PR #117 was merged as `f9e5f88bc223a2ce8a30fdbf4596893b34bc1551`.

## Current post-Phase-62 baseline

The current `main` baseline before Draft PR #133 is:

```text
main @ ebc9d8ebb35c12b24a27e831b8f7872c610c7354
```

Relevant completed post-Phase-62 work:

- PR #118: TVScraper classification and refresh corrections;
- PR #123: public-base-path-safe EPG artwork resolution;
- PR #132: guarded series-artwork fallback with TVmaze/TMDB, secure per-backend settings, TVScraper series identity preservation and poster/cover preference;
- direct channel-detail layout correction `96b97378` plus regression test `2d04a963`.

PR #132 was merged as:

```text
441e5febf7d3ab0121a585ce1176a8e5a7c67ce0
```

Its final feature head passed VDR-Suite CI #6982 with all five jobs successful. Real yaVDR operation additionally proved persisted `provider=tmdb` fallback assets and successful frontend delivery. The channel-detail text-layout correction was installed and confirmed in the browser.

## Active bounded hardening

Draft PR #133 on `agent/series-artwork-route-scope-hardening` derives the series-artwork settings authorization and accountability backend from the authoritative route. The original request continues to the API handler, preserving its independent route/body mismatch rejection. Malformed nested or percent-encoded backend segments fail closed rather than inheriting scope from the JSON body.

The dedicated test is part of `test-security` and covers Admin allow, Read-only denial, wrong backend scope, missing and invalid CSRF, route/body mismatch scope, query strings, malformed route segments and route-scoped success/failure outcomes.

This work is bounded post-Phase-62 hardening. It does not reopen Phase 62, change Legacy Basic compatibility, modify TVScraper or start Phase 63.

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

The post-Phase-62 series-artwork settings POST is integrated into the Phase-62 protected-mutation model: dedicated permission, fixed Read-only denial, browser CSRF, pre-dispatch accountability and success/failure outcomes remain active. Draft PR #133 makes the route authoritative for authorization and accountability while the handler retains its mismatch rejection. Managed TMDB tokens remain in a private secret directory and are not returned by the API or copied into accountability events.

No known authentication, authorization, CSRF, Read-only-role or cross-backend write bypass was introduced by PRs #118, #123, #132 or the bounded hardening in Draft PR #133. See the dedicated [Post-Phase-62 Security Review](development/post-phase-62-security-review.md).

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and intentionally retained. `enforced` mode is the fail-closed target. Removing Legacy Basic requires a separate deployment-migration contract and is not unfinished Phase-62 work.

## Current work boundary

- Phase 62 is completed and must not be reopened merely to add optional security administration, audit products, universal idempotency, generic Outbox infrastructure or speculative credential lifecycle.
- Phase 63 begins only with a new bounded contract.
- TVScraper remains upstream and unchanged; VDR-Suite integrates through `vdr-plugin-suite-bridge`.
- External series-artwork fallback requires a deterministic series identity and does not perform title/fuzzy search.
- Unknown central POST routes remain subject to the Phase-62 fail-closed policy outside explicit Legacy Basic compatibility.

## Exact next action

Stabilize Draft PR #133 on its final GitHub Actions head and keep it Draft until the user explicitly approves a review-state or merge action. Start Phase 63 only after a separate approved contract.

## Credential and secret restrictions

Never print, store or commit Authorization headers, plaintext passwords, password hashes, cookies, CSRF tokens, raw session/verifier secrets, TMDB tokens, secret-bearing login responses or process environments.
