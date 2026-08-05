# VDR-Suite Current Project Status

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main baseline:
a125b702a6d3a7fe510a94c84dc1930d3b17a4c5

Latest merged bounded feature:
PR #136 - Add manual recording cast ingestion and search integration
Final source head: eb7afa4e6cc5998614ae28b06a1c0c75e85bea41
Merge commit: a125b702a6d3a7fe510a94c84dc1930d3b17a4c5
Source CI: VDR-Suite CI #7228, run 30981621649, all five jobs successful
Real yaVDR acceptance: completed before merge

Active numbered runtime slice:
Phase 63 Slice 1 - Backend Agent Enrollment and Lease Foundation
Draft PR #137 - Add backend agent enrollment and lease foundation
Branch: agent/phase63-backend-agent-foundation
State: Draft; implementation and stabilization in progress

Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 63 Slice 1; Phase 63 is not complete

Phase 64-67 runtime:
not advanced
```

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

This is durable historical evidence for the accepted Phase-62 candidate, not a byte-for-byte fingerprint of later daemon builds.

## Completed post-Phase-62 work

- Remote/Live Overlay hardening (#110), backend-scoped Global Search (#111), TVScraper and artwork corrections (#118, #123, #132).
- PR #135 added backend-only manual movie/series/season/episode selection, immutable evidence, relationship-locked assignment/withdrawal and bundled folder readback.
- PR #136 added atomic selected-movie cast persistence, canonical people identities, effective title/actor search integration, reassignment/withdrawal history and provider-free set-based read paths.

## Active Phase 63 Slice 1

The binding contract is [Phase 63 Backend Agent Enrollment and Lease Foundation](phase-63-backend-agent-foundation.md).

Implemented scope under Draft PR #137:

- administrator-authorized one-time enrollment bound to an existing Backend;
- persistent technical Agent actor/device/credential identity;
- exact Agent protocol validation and generation fencing;
- heartbeat, lease and derived lifecycle status;
- bounded read-only capabilities;
- reconnect/resync disposition;
- self-scoped central authorization for credential rotation;
- atomic credential replacement, immediate lease invalidation and restart-safe lost-response recovery;
- Agent revocation and replacement enrollment with retained history;
- existing accountability repository and redacted request/correlation evidence;
- outbound HTTPS runtime, protected local state, hardened systemd unit, manpages and install staging.

Hard exclusions remain VDR-native writes, command/result queues, snapshot/change ingestion, provider ownership/selection, public provider URLs, streaming, OSD and Phase-64-or-later runtime. Agent lifecycle state does not replace the existing BackendRegistry/direct-adapter availability authority.

## Security review

No browser/user authentication, CSRF, fixed Read-only-role or cross-backend policy is weakened. Agent credentials are distinct technical identities and receive no user/admin authority. Enrollment, rotation and revocation are fail-closed around the existing central authorization/accountability boundaries. Bootstrap/runtime secrets, hashes/verifiers, Authorization headers, provider credentials and secret-bearing diagnostics are never normal output.

## Compatibility-retirement decision

Legacy Basic remains transitional. `enforced` is the fail-closed target. Removal requires a separate deployment-migration contract and is not unfinished Phase 62.

## Development rules

- Root-level `AGENTS.md` remains binding.
- Verify current `main`, branch, PR and CI state before repository writes.
- Evaluate CI only for the exact final feature head.
- Do not treat historical acceptance hashes as proof for changed daemon fingerprints.
- Keep PR #137 Draft until exact-head CI and real-system acceptance are complete and the user explicitly approves readiness.
- Do not cross the documented Slice-1 exclusions.
- Never commit or print credentials, cookies, CSRF secrets, provider tokens or secret-bearing process environments.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can perform the complete bounded operation safely.

Use local edits first only when the change requires:

- a workaround because the GitHub connector blocks a file operation;
- repository-local generation, compilation or tests that cannot be performed through the connector;
- a coherent multi-file patch that must be validated locally before publication.

Never replace a complete repository file from a truncated fetch. After each GitHub file update, inspect the resulting commit diff before treating the change as correct.

## Exact next action

1. Complete final local focused/build/package/document stabilization for Draft PR #137.
2. Push coherent commits fast-forward-only and evaluate all required CI jobs on one exact final head.
3. Update PR #137 with exact-head validation and security/architecture boundaries.
4. Install that exact head on yaVDR and execute the documented enrollment/connect/capability/lease/reconnect/rotation/revocation checklist.
5. Keep the PR Draft until the user explicitly approves readiness after real-system acceptance.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Phase 63 Backend Agent Foundation](phase-63-backend-agent-foundation.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Completed Phases](completed-phases.md)
- [Agent Workflow Rules](../../AGENTS.md)
