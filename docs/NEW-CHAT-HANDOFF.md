# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Repository, pull-request and runtime facts must be checked against the current `main` branch and the exact active PR head; do not repeat historical acceptance work without a directly relevant runtime change.

## Canonical reading

- [Current State](CURRENT.md)
- [Current Project Status](development/current-status.md)
- [Manual Recording Metadata Assignment](development/manual-recording-metadata-assignment.md)
- [Manual Recording Cast Ingestion and Search Integration](development/manual-recording-cast-search.md)
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

## Current merged baseline

The current verified merged basis for the active feature is:

```text
main @ 89b023ca6758f7ba8f08f75831c2ccdba77a0b08
```

This is the squash merge of:

```text
PR #135 - Add manual recording metadata search and assignment
final source head: 37b06f6e97ee00cefd8b6704f6cd6ed1cf9d2be7
CI: VDR-Suite CI #7144
run ID: 30941248988
result: all five jobs successful
```

The exact final PR #135 head was installed on the real yaVDR system. Repeated recording-folder, subfolder, back and reopen navigation was confirmed fast after the bundled manual-assignment readback removed the N+1/schema-loop regression.

The old `agent/manual-recording-metadata-assignment` branch is historical and must not be used as the base for new work.

## Active bounded post-Phase-62 feature

```text
PR #136 - Add manual recording cast ingestion and search integration
branch: agent/manual-recording-cast-search
base: main @ 89b023ca6758f7ba8f08f75831c2ccdba77a0b08
state: open Draft
```

The exact current PR head, diff, mergeability and CI must always be read from GitHub before work resumes. Do not copy a head SHA from this Handoff as current proof after later commits.

PR #136 is an explicitly approved, limited continuation of PR #135. It is not Phase 63. It must remain Draft and must not be marked Ready or merged until the exact final head has complete green CI, has been installed and tested on yaVDR, and the user explicitly approves readiness.

### Feature architecture

- TMDB credits are loaded backend-side only after the user selects one exact movie candidate.
- Candidate search, folder navigation, recording detail, global search and dedicated person search never load credits.
- A maximum of 128 cast members is parsed and persisted, matching the existing recording-person contract.
- Technical provider failure aborts the whole assignment.
- A valid provider response with an empty cast is persisted as complete and is not treated as failure.
- Movie assignment, immutable evidence, canonical people, provider-qualified person IDs and recording-person relations are written in one `BEGIN IMMEDIATE` transaction.
- People reuse `suite_metadata_entities(media_type='person')` and `suite_metadata_entity_external_ids` with provider `tmdb`, namespace `person` and the TMDB person ID.
- The same TMDB person is deduplicated across recordings.
- Recording-person relations belong to the concrete manual assignment revision and retain actor role, character and cast order.
- Reassignment supersedes the old assignment without deleting its evidence or relations.
- Withdrawal removes manual title and people from the active read model without deleting history.
- Active relationship-locked manual title, original title and cast extend the existing global and person search paths.
- Automatic TVScraper people are suppressed only for a recording with an active manual assignment and become effective again after withdrawal.
- Recording detail reuses the existing public person shape and does not expose internal entity IDs, provider URLs, local paths or actor references.
- Folder readback uses one backend-scoped assignment-and-cast query.
- Person search and global recording search use constant count/page query contracts, not one query per recording or person.

Authoritative design and validation documents:

- [ADR-0052](adr/ADR-0052-manual-recording-cast-ingestion-search.md)
- [Manual Recording Cast Ingestion and Search Integration](development/manual-recording-cast-search.md)
- [Backend-Scoped Global Search](architecture/global-search.md)

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

The active manual selected-movie workflow remains inside the Phase-62 protected-mutation model:

- permission `metadata.recording.assign`;
- route-authoritative exact backend scope;
- Admin grant and fixed Read-only denial;
- browser CSRF;
- pre-dispatch accountability and post-dispatch outcomes;
- existing backend-scoped managed TMDB credential resolver.

Read-only, wrong-backend and invalid-CSRF requests must be denied before provider access. Tokens, Authorization headers, cookies, CSRF values, actor references, provider URLs, local paths and secret-bearing process environments must not be printed, committed or copied into public responses or accountability events.

TVScraper remains unchanged upstream. Do not fork it and do not write to TVScraper-owned databases or caches.

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and intentionally retained. `enforced` mode is the fail-closed target. Removing Legacy Basic requires a separate deployment-migration contract and is not unfinished Phase-62 work.

## Current work boundary

- Phase 62 is completed and must not be rewritten.
- PR #135 is merged and is the only valid foundation for PR #136.
- PR #136 is the active approved post-Phase-62 feature block.
- Phase 63 has not started and must not start while this feature block remains active.
- The feature must not alter Phase-63 runtime contracts.
- TVScraper remains upstream and unchanged; VDR-Suite integrates through its existing boundaries.
- Unknown central POST routes remain subject to the Phase-62 fail-closed policy outside explicit Legacy Basic compatibility.
- Do not add unrelated refactors, cosmetic rewrites or a parallel manual-search/index architecture.

## Exact next action

1. Read PR #136 and its exact current head from GitHub.
2. Inspect the complete current diff and CI jobs for that exact head.
3. Fix only evidence-backed code, SQL, security, performance, frontend or documentation defects.
4. Run and observe the complete relevant repository tests, production daemon build, packaging/install staging, frontend tests, documentation checks and Make audit on one exact final head.
5. Update the Draft PR body with that exact head, CI run, architecture, security, performance and real yaVDR checklist.
6. Keep PR #136 Draft.
7. Install the exact final head and execute the real yaVDR cast/title/person/restart/reassignment/withdrawal acceptance checklist in [the feature document](development/manual-recording-cast-search.md).
8. Do not begin Phase 63 until this approved feature block has been explicitly accepted and completed or otherwise closed.

## Command presentation contract

Every shell command intended for the user to copy or execute must be presented inside a normal fenced Markdown code block, preferably tagged `bash`.

- Never place executable commands in prose, inline-code fragments, writing blocks, generated UI controls or custom code-block formats with IDs or metadata.
- Keep explanations outside the code block.
- Put complete, directly executable command sequences inside the code block.
- Use separate code blocks for logically separate steps when that improves safe execution.
- Preserve explicit checkout-path and repository-identity verification; never hide required setup in surrounding prose.
- When the user asks for build, test, installation, rollback or diagnostic commands, the final answer must contain those commands in ordinary copyable Markdown code blocks.

## Binding daemon build and installation manifest

When the user asks for the commands to build and install the VDR-Suite daemon, every new chat must use the following response contract as the authoritative default:

- Use the heading `## Lokaler Bau, Test und Installation`.
- Write at most one short introductory sentence, then provide exactly one ordinary fenced Markdown `bash` code block.
- The Bash fence must have no IDs, attributes, metadata or custom wrapper syntax.
- Put the complete directly executable command sequence in that one block; do not fragment it into a prose tutorial or many small code blocks.
- For the established development or yaVDR host, the required sequence is: enter the verified existing checkout, verify repository identity and a clean worktree, `git switch` to the requested branch, `git pull --ff-only origin <branch>`, verify the exact expected commit, `make clean`, build the daemon, stop `vdr-suite-daemon`, run `sudo make install PREFIX=/usr`, run `sudo systemctl daemon-reload`, then enable/start or restart the service and verify the installed daemon and service state.
- `git pull --ff-only` is mandatory in this established-checkout workflow. Do not silently replace it with a custom `git fetch` plus merge sequence when giving the user installation commands.
- Never include `apt-get update`, `apt update`, `apt-get install`, `apt install` or any other package-management command in the ordinary branch build-and-install instructions for the established host.
- Add package-management commands only when the user explicitly requests dependency installation or an actual build failure has demonstrated a missing dependency. Do not assume a fresh system and do not proactively refresh package lists.
- Do not clone a second checkout when the user is updating the established repository checkout. Use the verified existing checkout and pull the requested branch.
- Keep the requested scope narrow. Do not add CI test suites, backups, rollback procedures, HTTP checks, browser acceptance, TMDB checks, token handling or unrelated diagnostics unless the user explicitly asks for them.
- Do not turn the answer into a giant all-purpose shell script. Supply the shortest complete sequence that safely performs the requested daemon build and installation.
- Do not repeat the same commands in explanatory prose. The copyable Bash block is the primary deliverable.

The canonical established-host flow is therefore exactly: `git switch`, `git pull --ff-only`, `make clean`, daemon build, service stop, `sudo make install PREFIX=/usr`, `systemctl daemon-reload`, and service start or restart. No package installation and no `apt-get update` belong in that answer unless explicitly requested or proven necessary by a real dependency failure.

This manifest overrides any tendency to provide a fresh-system setup, dependency bootstrap, long step-by-step installation essay or generalized deployment script when the user asked only for the existing checkout to be updated, built and installed.

## Credential and secret restrictions

Never print, store or commit Authorization headers, plaintext passwords, password hashes, cookies, CSRF tokens, raw session/verifier secrets, TMDB tokens, secret-bearing login responses or process environments.
