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
Phase 63 Slice 1 in Draft PR #137; Phase 63 is not complete

Phase 64-67 runtime:
not advanced
```

Phase 62 remains completed and must not be reopened. Its historical runtime evidence remains authoritative for its accepted candidate.

## Current merged baseline

```text
main @ a125b702a6d3a7fe510a94c84dc1930d3b17a4c5
```

This is the merge of:

```text
PR #136 - Add manual recording cast ingestion and search integration
final source head: eb7afa4e6cc5998614ae28b06a1c0c75e85bea41
CI: VDR-Suite CI #7228
run ID: 30981621649
result: all five jobs successful
real yaVDR acceptance: completed before merge
```

PR #136 completed the bounded manual selected-movie cast/title/person search workflow. It is historical merged foundation, not active work.

## Active Phase 63 Slice 1

```text
PR #137 - Add backend agent enrollment and lease foundation
branch: agent/phase63-backend-agent-foundation
base: main @ a125b702a6d3a7fe510a94c84dc1930d3b17a4c5
state: open Draft
```

The exact current PR head, diff, mergeability and CI must always be read from GitHub before work resumes. Do not copy a head SHA from this Handoff as current proof after later commits.

PR #137 is the first bounded runtime slice of Phase 63. It must remain Draft and must not be marked Ready or merged until the exact final head has complete green CI, has been installed and tested on yaVDR, and the user explicitly approves readiness.

### Slice architecture

- Administrator-authorized one-time enrollment binds a technical Agent identity to one existing Backend.
- Runtime Agent actor/device/credential identities reuse the Phase-62 security repositories and remain distinct from browser/user credentials.
- The first machine protocol accepts exactly `vdr-suite-agent/1` over protected outbound HTTPS.
- A newly accepted process instance receives a monotonically increasing backend generation; obsolete instances and generations are fenced.
- Heartbeats renew only a Control-Plane-clock lease. Online/stale/offline Agent status is derived from persisted lease facts.
- Read-only capabilities are allowlisted, bounded, revisioned and contain no endpoint, URL, path or credential.
- Agent-initiated credential rotation is self-scoped, generation-fenced, transactional and restart-safe after ambiguous transport results.
- Revocation invalidates the lease and permits a replacement Agent enrollment while retaining revoked history.
- Existing append-only accountability and central authorization are reused; no parallel security/audit system is introduced.
- The packaged Agent stores identity/recovery state as 0600 under a systemd-owned 0700 state directory and runs under a hardened systemd unit.
- A local administration utility exposes redacted status and accountable revocation; the guarded runtime harness performs the complete yaVDR acceptance without manual SQLite inspection.
- Agent lease state does not overwrite existing direct-adapter `BackendNode.online`; provider ownership/selection is deferred.

Hard exclusions are VDR-native writes, snapshot/change ingestion, command/result queues, provider selection, public provider URLs, streaming, OSD and Phase-64-or-later runtime.

Authoritative contract:

- [Phase 63 Backend Agent Foundation](development/phase-63-backend-agent-foundation.md)
- [Phase 63 Backend Agent Runtime Acceptance](development/phase-63-backend-agent-runtime-acceptance-runbook.md)
- [ADR-0039 Backend Agent and Control Plane Boundary](adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040 Backend Lifecycle, Generation, Lease and Health](adr/ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041 Authentication, Agent Trust and Multi-Site Transport](adr/ADR-0041-authentication-agent-trust-multi-site-transport.md)

## Phase 62 completion evidence

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

- Enrollment creation is a centrally authorized and accountable Control-Plane transition.
- Enrollment consumption can create only the pre-bound technical Agent identity.
- Runtime Agent credentials cannot act as browser users or unrestricted administrators.
- Backend binding, credential generation, Agent instance and backend generation are enforced server-side.
- Rotation is limited to the Agent's own bound Backend and invalidates the lease atomically.
- Read-only capabilities are observations, never grants.
- Bootstrap/runtime secrets, hashes/verifiers, Authorization headers, cookies, CSRF values, provider URLs, local secret paths and secret-bearing process environments must not be printed, committed or copied into public responses/accountability events.
- TVScraper remains unchanged upstream; do not write to TVScraper-owned databases or caches.

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and intentionally retained. `enforced` mode is the fail-closed target. Removing Legacy Basic requires a separate deployment-migration contract and is not unfinished Phase 62.

## Current work boundary

- Phase 62 is completed and must not be rewritten.
- PR #136 is merged into the current `main` baseline.
- PR #137 is the only active approved Phase-63 slice.
- Keep PR #137 Draft until exact-head CI, real yaVDR acceptance and explicit user approval.
- Do not add VDR-native mutation, command/result flow, snapshot ingestion, provider selection or later-phase runtime to Slice 1.
- Do not create a second BackendRegistry, authorization service, accountability store or job system.
- Unknown central POST routes remain subject to the Phase-62 fail-closed policy outside explicit Legacy Basic compatibility.
- Do not add unrelated refactors or cosmetic rewrites.

## Exact next action

1. Read PR #137 and its exact current head from GitHub.
2. Inspect the complete current diff and CI jobs for that exact head.
3. Fix only evidence-backed lifecycle, persistence, security, transport, packaging or documentation defects inside the binding Slice-1 contract.
4. Run and observe focused tests, architecture guards, production daemon/Agent builds, packaging/install staging, documentation checks and Make inventory on one exact final head.
5. Update the Draft PR body with exact-head CI, architecture/security boundaries and real yaVDR checklist.
6. Keep PR #137 Draft.
7. Install the exact final head and execute `phase63-backend-agent-runtime-acceptance` from the guarded runbook; do not replace it with manual SQLite inspection.
8. Do not advance into another Phase-63 slice without a new bounded contract.

## Command presentation contract

Every shell command intended for the user to copy or execute must be presented inside a normal fenced Markdown code block, preferably tagged `bash`.

- Never place executable commands in prose, inline-code fragments, writing blocks, generated UI controls or custom code-block formats with IDs or metadata.
- Keep explanations outside the code block.
- Put complete, directly executable command sequences inside the code block.
- Use separate code blocks for logically separate steps when that improves safe execution.
- Preserve explicit checkout-path and repository-identity verification; never hide required setup in surrounding prose.
- When the user asks for build, test, installation, rollback or diagnostic commands, the final answer must contain those commands in ordinary copyable Markdown code blocks.

## Binding daemon build and installation manifest

Every installation answer must be generated for the exact requested branch or pull request. Generic installation instructions and commands copied from another PR are forbidden.

Before producing the command block, the agent must:

- resolve the exact requested PR and branch from GitHub;
- resolve the exact current PR head SHA immediately before presenting the commands;
- inspect the root `Makefile`, all included install/build makefiles such as `mk/install.mk`, relevant packaging and systemd files, and any component-specific Makefile changed or required by that exact head;
- determine the exact build targets and whether `vdr-plugin-suite-bridge` must be rebuilt and installed;
- never infer installation options, target names, plugin paths or service names from an older branch, older PR, README excerpt or previous chat.

The answer must use the heading `## Lokaler Bau, Test und Installation`, followed by at most one short sentence and exactly one ordinary fenced Markdown `bash` block without IDs, attributes or metadata.

For the established yaVDR checkout, the mandatory daemon flow has this shape:

```bash
cd /home/yavdr/vdr-suite

git switch <exact-branch>
git pull --ff-only origin <exact-branch>

git rev-parse HEAD
test "$(git rev-parse HEAD)" = "<exact-head-sha>" || exit 1

make clean
make -j2 --output-sync=target <exact-required-build-targets>

systemctl stop vdr-suite-daemon
make install PREFIX=/usr
systemctl daemon-reload
systemctl restart vdr-suite-daemon

systemctl is-active vdr-suite-daemon
systemctl --no-pager --full status vdr-suite-daemon
```

The placeholders above define the required structure only. In a user-facing answer they must always be replaced with the exact branch, exact current head SHA and exact build targets for the requested branch or PR. Never leave placeholders in executable instructions.

Additional binding rules:

- `git pull --ff-only origin <exact-branch>` is mandatory for the established checkout.
- The exact SHA guard is mandatory and must abort before build or installation when the checkout does not match the verified PR head.
- Use `make clean` followed by `make -j2 --output-sync=target` with only the targets actually required by that branch or PR.
- Do not add `sudo` merely as a style preference; preserve the established host execution context unless the user explicitly requests a non-root form.
- Stop the daemon before `make install PREFIX=/usr`, then reload systemd, restart the daemon and show both `is-active` and the full service status.
- Do not add package-manager commands, dependency bootstrapping, a second clone, backups, rollback scripts, HTTP checks, browser checks or unrelated diagnostics unless explicitly requested or proven necessary by an observed failure.
- Keep the answer branch-/PR-specific and as short as the complete safe flow permits.

### Conditional SuiteBridge plugin installation

The plugin must not be rebuilt merely because it exists in the repository.

- First inspect the exact PR diff and component dependency contract.
- When the PR does not change `vdr-plugin-suite-bridge` and the runtime change does not require a new plugin binary or contract, omit all plugin build and installation commands.
- When the plugin is required, inspect the exact-head `vdr-plugin-suite-bridge/Makefile`, VDR `pkg-config` values and the established yaVDR service layout before writing commands.
- Add the exact plugin clean/build/install and required VDR service stop/restart/status commands to the same branch-/PR-specific Bash block.
- Never guess `VDRDIR`, `LIBDIR`, `APIVERSION`, destination paths or the VDR service unit name, and never reuse plugin commands from another PR without verifying them against the requested head.

This manifest overrides any tendency to provide generic setup instructions, a fresh-system installation tutorial, commands from a previous PR, or prose instead of one directly copyable branch-/PR-specific shell block.

## Binding branch- and PR-specific installed-result acceptance manifest

A successful build, file installation and `active (running)` service state prove only that deployment completed. They do not prove that the requested PR behavior works. Every installation answer must therefore be followed by a branch- or PR-specific acceptance section derived from the exact current head.

Before writing the acceptance steps, the agent must inspect:

- the exact PR diff and changed components;
- the current feature, ADR and runtime-acceptance documents;
- changed REST routes, persistence/schema behavior, frontend paths, services and plugin contracts;
- the closest existing regression behavior that the PR could unintentionally break.

The user-facing answer must use the heading `## Prüfung des installierten Ergebnisses`. Shell diagnostics must remain in ordinary fenced `bash` blocks. Browser, UI and functional actions may be a concise numbered checklist outside the shell block. Do not merge functional checks into the installation block when that would hide required user actions.

Every acceptance plan must cover the layers that are relevant to that exact PR:

1. **Installed identity and startup:** verify the checked-out exact head, installed binary or asset identity where the repository provides a reliable method, service state and absence of new startup errors.
2. **Positive feature path:** exercise the behavior introduced or changed by the PR using a real representative resource.
3. **Readback and persistence:** reload the UI or API, restart the affected service, and confirm the result survives without repeating the mutation or requiring an external provider read.
4. **Search and presentation:** verify every changed read model, detail view, search path or frontend rendering affected by the PR.
5. **Replacement and withdrawal semantics:** when the feature supports reassignment, deletion, withdrawal, rollback or fallback, verify the old active result disappears and the documented fallback becomes effective.
6. **Authorization and failure boundaries:** verify the relevant Read-only, wrong-scope, CSRF, provider-failure or invalid-input denial paths without exposing secrets.
7. **Adjacent regression:** repeat the nearest established workflow whose performance or correctness could be affected, such as folder navigation, restart behavior or automatic metadata fallback.
8. **Evidence:** record the exact source head, CI run, installed build identity when available, redacted test resource and observed result.

Only include layers that the exact PR can affect, but never omit persistence/restart or the primary regression boundary merely to shorten the answer. When a test requires credentials, cookies, CSRF values, tokens or private paths, instruct the user through the normal UI or a redacted safe procedure; never request or print those values.

For PR #136, the authoritative real-system checklist is [Manual Recording Cast Ingestion and Search Integration](development/manual-recording-cast-search.md#real-yavdr-acceptance-checklist). At minimum it requires:

- repeated recording-folder/subfolder/back navigation remains fast;
- candidate search remains fast before selecting a movie;
- assigning a known TMDB movie displays actor and character names;
- reload and `vdr-suite-daemon` restart preserve title, cast and character data;
- manual title, original title and actor are found through the existing global and person searches;
- reassignment removes the former movie's active actors;
- withdrawal restores automatic TVScraper/native title and people;
- Read-only and wrong-backend assignment attempts are denied;
- responses and accountability evidence expose no token, provider URL, local artwork path or actor reference.

Never describe an acceptance item as passed merely because the daemon started or automated CI is green. Mark it passed only after the user has actually executed the exact-head test and supplied or confirmed the observed result. Keep the PR Draft until all required real-system acceptance items are complete and the user explicitly approves readiness.

## Credential and secret restrictions

Never print, store or commit Authorization headers, plaintext passwords, password hashes, cookies, CSRF tokens, raw session/verifier secrets, TMDB tokens, secret-bearing login responses or process environments.
