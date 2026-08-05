# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Repository, pull-request and runtime facts must be checked against the current `main` branch and the exact active PR head; do not repeat historical acceptance work without a directly relevant runtime change.

## Canonical reading

- [Current State](CURRENT.md)
- [Current Project Status](development/current-status.md)
- [Phase 63 Slice-1 Closeout](development/phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](development/phase-63-observation-ingestion.md)
- [Phase 63 Backend Agent Foundation](development/phase-63-backend-agent-foundation.md)
- [Phase 63 Backend Agent Runtime Acceptance](development/phase-63-backend-agent-runtime-acceptance-runbook.md)
- [Manual Recording Metadata Assignment](development/manual-recording-metadata-assignment.md)
- [Manual Recording Cast Ingestion and Search Integration](development/manual-recording-cast-search.md)
- [Post-Phase-62 Security Review](development/post-phase-62-security-review.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
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

Completed Phase-63 slice:
Phase 63 Slice 1 - Backend Agent Enrollment and Lease Foundation
PR #137 merged and exact-head real yaVDR acceptance passed

Current active numbered runtime phase:
Phase 63 Slice 2 - Read-only Observation and Snapshot Ingestion Foundation
Draft PR #138 defines the binding contract

Phase 63 is not complete

Phase 64-67 runtime:
not advanced
```

Phase 62 remains completed and must not be reopened. Its historical runtime evidence remains authoritative for its accepted candidate.

## Current merged baseline

```text
main @ a9620179a442155f0860ef3182ca39186ac46a57
```

This is the squash merge of:

```text
PR #137 - Add backend agent enrollment and lease foundation
accepted source head: bba51455552bab0f1a06c680369c508858b2384b
accepted tree: 575f49a197cda9ad02da4035b437ee1c32bed2d6
CI: VDR-Suite CI #7256
run ID: 31001478896
result: all five jobs successful
real yaVDR acceptance: PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS
```

The merged main baseline also includes completed PR #135 manual Recording metadata assignment and PR #136 manual selected-movie cast/title/person search integration.

## Completed Phase 63 Slice 1

Slice 1 established and accepted:

- administrator-authorized one-time Agent enrollment into an existing Backend;
- persistent technical Agent actor/device/credential identity;
- exact `vdr-suite-agent/1` protocol compatibility;
- Agent-process-instance and backend-generation fencing;
- heartbeat, lease and online/stale/offline state;
- bounded read-only capabilities;
- reconnect and restart reconciliation;
- credential rotation, revocation and replacement enrollment;
- protected outbound HTTPS transport and protected local state;
- hardened systemd/package/install contracts;
- redacted status/revocation administration;
- guarded exact-head real yaVDR acceptance without manual SQLite inspection.

Final accepted evidence:

```text
PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS
HEAD=bba51455552bab0f1a06c680369c508858b2384b
CONTROL_PLANE_URL=https://192.168.178.38/vdr-suite
CREDENTIAL_GENERATION=2
VDR_NATIVE_STATE_UNCHANGED=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=/var/backups/vdr-suite-phase63-20260805T114111Z-bba51455552b
```

Slice 1 implemented no domain observations, snapshots, commands, results, VDR-native execution or provider selection. Agent lifecycle state does not replace existing direct-adapter `BackendNode.online` authority.

## Active Phase 63 Slice 2 contract

```text
PR #138 - Define read-only agent observation ingestion contract
branch: agent/phase63-observation-ingestion-contract
base: main @ a9620179a442155f0860ef3182ca39186ac46a57
state: open Draft
```

The exact current PR head, diff, mergeability and CI must always be read from GitHub before work resumes. Do not copy a head SHA from this Handoff as current proof after later commits.

PR #138 is intentionally a contract/closeout PR. It contains no observation runtime yet. It must remain Draft until one exact final head has complete green CI and the user explicitly approves readiness.

### Slice-2 architecture

- Existing Agent technical authentication remains the only Agent HTTP authentication.
- Every accepted observation is bound to Backend ID, Agent ID, Agent process instance and backend generation.
- `snapshotGeneration`, `producerSequence` and `resourceRevision` are independent axes.
- A `completeSnapshot` establishes the bounded baseline for one observation domain.
- A `changeBatch` advances only at the exact next producer sequence.
- Equivalent replay is idempotently acknowledged; conflicting replay is rejected.
- Missing baseline or sequence gaps return `resync-required`; continuity is never guessed.
- Immutable receipt/fact evidence and the current ingestion cursor commit atomically through Suite-owned repositories.
- Repository code owns SQLite; HTTP handlers and Agent client code do not issue direct SQLite statements.
- The first bounded runtime domain is `backend-health`.
- Observation evidence retains provenance without credentials, Authorization headers, private provider addresses or secret-bearing diagnostics.
- Existing direct-adapter state remains authoritative until provider ownership/selection receives a separate contract.

Hard exclusions are command inbox/results, VDR-native mutation, provider ownership/selection, public Agent/provider URLs, TimerIntent/Phase-64 runtime, Streaming Gateway and OSD work.

Authoritative contract:

- [Phase 63 Observation and Snapshot Ingestion](development/phase-63-observation-ingestion.md)
- [Phase 63 Slice-1 Closeout](development/phase-63-slice-1-closeout.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [ADR-0039 Backend Agent and Control Plane Boundary](adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040 Backend Lifecycle, Generation, Lease and Health](adr/ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041 Authentication, Agent Trust and Multi-Site Transport](adr/ADR-0041-authentication-agent-trust-multi-site-transport.md)

## Phase 62 completion evidence

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

This evidence closes Phase 62. It is historical evidence for that accepted runtime fingerprint, not a claim that every later daemon build is byte-identical.

## Current security position

- Runtime Agent credentials cannot act as browser users or unrestricted administrators.
- Backend binding, credential generation, Agent instance and backend generation are enforced server-side.
- Observation domains are read-only capability declarations, never grants.
- Snapshot generation, producer sequence and resource revision are validated independently.
- Revoked or stale-generation Agents cannot advance current ingestion.
- Bootstrap/runtime secrets, hashes/verifiers, Authorization headers, cookies, CSRF values, provider URLs, local secret paths and secret-bearing process environments must not be printed, committed or copied into public responses/accountability events.
- TVScraper remains unchanged upstream; do not write to TVScraper-owned databases or caches.

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and intentionally retained. `enforced` mode is the fail-closed target. Removing Legacy Basic requires a separate deployment-migration contract and is not unfinished Phase 62 or Phase 63.

## Current work boundary

- Phase 62 is completed and must not be rewritten.
- Phase 63 Slice 1 is merged and accepted.
- PR #138 is the only active approved Phase-63 contract slice.
- Keep PR #138 Draft until exact-head CI and explicit user approval.
- Do not add command/result flow, VDR-native mutation, provider ownership/selection or later-phase runtime to Slice 2.
- Do not create a second BackendRegistry, authorization service, accountability store or job system.
- Do not require manual SQLite inspection for acceptance.
- Unknown central POST routes remain subject to the Phase-62 fail-closed policy outside explicit Legacy Basic compatibility.
- Do not add unrelated refactors or cosmetic rewrites.

## Exact next action

1. Read PR #138 and its exact current head from GitHub.
2. Inspect the complete current diff and CI jobs for that exact head.
3. Fix only evidence-backed contract, sequencing, persistence-boundary, security, architecture or documentation defects.
4. Run and observe the Phase-63 observation contract guard, existing Phase-63 harness guard, documentation checks, Make inventory and all required PR CI jobs on one exact final head.
5. Update the Draft PR body with exact-head validation and retained hard exclusions.
6. Keep PR #138 Draft until explicit approval.
7. After the contract PR is accepted, start a separate bounded runtime implementation with `backend-health` complete-snapshot and exact-next change ingestion only.

## Command presentation contract

Every shell command intended for the user to copy or execute must be presented inside a normal fenced Markdown code block, preferably tagged `bash`.

- Never place executable commands in prose, inline-code fragments, writing blocks, generated UI controls or custom code-block formats with IDs or metadata.
- Keep explanations outside the code block.
- Put complete, directly executable command sequences inside the code block.
- Preserve explicit checkout-path and repository-identity verification.
- Do not use `set -e` or another user-facing errexit wrapper; use explicit error handling.

## Binding daemon build and installation manifest

Every installation answer must be generated for the exact requested branch or pull request. Generic installation instructions and commands copied from another PR are forbidden.

Before producing the command block, resolve the exact PR head and inspect the root `Makefile`, relevant included makefiles, packaging/systemd files and changed component contracts. Determine whether the daemon, standalone Agent or SuiteBridge plugin actually needs rebuilding. Never infer target names or paths from an older PR.

The answer must use the heading `## Lokaler Bau, Test und Installation`, followed by at most one short sentence and exactly one ordinary fenced Markdown `bash` block without IDs, attributes or metadata.

For the established yaVDR checkout, preserve this structure while replacing every placeholder with exact branch/head/targets:

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

The placeholders define required structure only and must never remain in user-facing executable instructions.

### Conditional SuiteBridge plugin installation

The plugin must not be rebuilt merely because it exists in the repository. Inspect the exact PR diff and dependency contract. Add plugin build/install and VDR service commands only when the exact head requires a changed plugin binary or local contract. Never guess VDR paths, API version or service unit.

## Binding installed-result acceptance manifest

A successful build, installation and active service prove deployment only. Every installation answer must include `## Prüfung des installierten Ergebnisses` with exact-PR checks for:

1. installed identity and startup;
2. positive feature path;
3. readback, persistence and restart;
4. changed search/presentation paths;
5. replacement, withdrawal or resynchronization semantics where applicable;
6. authorization and failure boundaries;
7. adjacent regression;
8. exact-head redacted evidence.

Never describe acceptance as passed merely because CI is green or a service started. Mark it passed only after the exact-head runtime test was actually executed and its result supplied or confirmed.

## Credential and secret restrictions

Never print, store or commit Authorization headers, plaintext passwords, password hashes, cookies, CSRF tokens, raw session/verifier secrets, TMDB tokens, enrollment tokens, Agent credential secrets, secret-bearing login responses or process environments.