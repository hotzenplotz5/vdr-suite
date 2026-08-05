# VDR-Suite Current Project Status

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main baseline:
24b1d7938ddaa15834a8da6323a270761868f4ba

Latest merged bounded contract slice:
Phase 63 Slice 2 - Read-only Observation and Snapshot Ingestion Foundation
PR #138 - Define read-only agent observation ingestion contract
Accepted source head: 0207c0cbc01f167139b5d6483680f9a280c05160
Merge commit: 24b1d7938ddaa15834a8da6323a270761868f4ba
Source CI: VDR-Suite CI #7275, run 31006387349, all five jobs successful
Runtime change: none; contract and guards only

Active numbered runtime slice:
Phase 63 Slice 2 - Backend Health Observation Ingestion Runtime
Draft PR #139 - Add backend health observation ingestion runtime
Branch: agent/phase63-backend-health-ingestion-runtime
State: Draft runtime implementation; exact-head CI and real yaVDR acceptance pending

Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 63 Slice 2; Phase 63 is not complete

Phase 64-67 runtime:
not advanced
```

## Final accepted Phase 63 Slice 1 runtime

```text
PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=bba51455552bab0f1a06c680369c508858b2384b
accepted_tree=575f49a197cda9ad02da4035b437ee1c32bed2d6
merge_commit=a9620179a442155f0860ef3182ca39186ac46a57
source_ci_run_number=7256
source_ci_run_id=31001478896
control_plane_url=https://192.168.178.38/vdr-suite
credential_generation=2
vdr_native_state_unchanged=yes
daemon_active=yes
agent_active=yes
evidence_directory=/var/backups/vdr-suite-phase63-20260805T114111Z-bba51455552b
```

This is durable historical evidence for the accepted Slice-1 candidate. Later Agent or daemon changes require their own exact-head CI and runtime proof.

## Completed post-Phase-62 work

- Remote/Live Overlay hardening (#110), backend-scoped Global Search (#111), TVScraper and artwork corrections (#118, #123, #132).
- PR #135 added backend-only manual movie/series/season/episode selection, immutable evidence, relationship-locked assignment/withdrawal and bundled folder readback.
- PR #136 added atomic selected-movie cast persistence, canonical people identities, effective title/actor search integration, reassignment/withdrawal history and provider-free set-based read paths.
- PR #137 added controlled Backend Agent enrollment, technical identity, protected transport, generation/instance fencing, lease lifecycle, read-only capabilities, credential rotation/revocation/replacement, hardened local state and guarded real-system acceptance.

## Completed Phase 63 Slice 1

The binding historical contract is [Phase 63 Backend Agent Enrollment and Lease Foundation](phase-63-backend-agent-foundation.md); the accepted evidence is recorded in [Phase 63 Slice-1 Closeout](phase-63-slice-1-closeout.md).

Completed scope:

- administrator-authorized one-time enrollment bound to an existing Backend;
- persistent technical Agent actor/device/credential identity;
- exact Agent protocol validation and generation fencing;
- heartbeat, lease and derived lifecycle status;
- bounded read-only capabilities;
- reconnect/resync disposition;
- self-scoped central authorization for credential rotation;
- atomic credential replacement and restart-safe lost-response recovery;
- Agent revocation and replacement enrollment with retained history;
- accountability repository and redacted request/correlation evidence reuse;
- outbound HTTPS runtime, protected local state, systemd-owned 0700 storage, hardened unit, manpages and install staging;
- local status/revocation administration and guarded yaVDR acceptance with failed-run cleanup.

Slice 1 did not implement domain observations, snapshots, commands, results, native execution, provider selection, public provider URLs, streaming, OSD or Phase-64 work. Agent lifecycle state does not replace existing direct-adapter availability authority.

## Active Phase 63 Slice 2 runtime

The binding contract is [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md).

Draft PR #139 implements the first bounded `backend-health` domain:

- existing `vdr-suite-agent/1` technical authentication only;
- backend, Agent, Agent-instance and backend-generation fencing;
- complete snapshot baseline and exact-next change sequencing;
- equivalent replay idempotency, conflicting replay rejection and explicit `resync-required`;
- atomic immutable receipt plus ingestion-cursor persistence in Suite-owned SQLite repositories;
- protected Agent-side observation lineage and pending-envelope retry after ambiguous transport results;
- backward-compatible migration of protected Agent identity state;
- redacted administrative observation cursor readback;
- upgrade-safe real-system acceptance that preserves the existing active Agent identity.

The runtime remains read-only and does not replace direct-adapter `BackendNode.online` authority.

## Security review

No browser/user authentication, CSRF, fixed Read-only-role or cross-backend policy is weakened. Agent credentials remain distinct technical identities and receive no user/admin authority. Observation ingestion is fail-closed around existing central authentication, authorization and accountability boundaries.

Bootstrap/runtime secrets, hashes/verifiers, Authorization headers, provider credentials and secret-bearing diagnostics are never normal output. Observation payloads do not become hidden authority over direct-adapter facts.

## Compatibility-retirement decision

Legacy Basic remains transitional. `enforced` is the fail-closed target. Removal requires a separate deployment-migration contract and is not unfinished Phase 62 or Phase 63.

## Development rules

- Root-level `AGENTS.md` remains binding.
- Verify current `main`, branch, PR and CI state before repository writes.
- Evaluate CI only for the exact final feature head.
- Do not treat historical acceptance hashes as proof for changed daemon fingerprints.
- Keep PR #139 Draft until exact-head CI and real yaVDR acceptance are green and the user explicitly approves readiness.
- Do not cross the documented Slice-2 exclusions.
- Never commit or print credentials, cookies, CSRF secrets, provider tokens or secret-bearing process environments.
- No manual SQLite inspection is part of runtime acceptance.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can perform the complete bounded operation safely.

Use local edits first only when the change requires:

- a workaround because the GitHub connector blocks a file operation;
- repository-local generation, compilation or tests that cannot be performed through the connector;
- a coherent multi-file patch that must be validated locally before publication.

Never replace a complete repository file from a truncated fetch. After each GitHub file update, inspect the resulting commit diff before treating the change as correct.

## Exact next action

1. Complete the upgrade-safe `backend-health` runtime acceptance harness and authoritative status updates in Draft PR #139.
2. Obtain all required CI jobs on one exact final head.
3. Build and install that exact head on yaVDR, preserving the existing Agent identity.
4. Execute the guarded upgrade-safe acceptance and retain redacted evidence.
5. Keep PR #139 Draft until exact-head acceptance and explicit approval.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Phase 63 Slice-1 Closeout](phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md)
- [Phase 63 Backend Agent Foundation](phase-63-backend-agent-foundation.md)
- [Phase 63 Backend Agent Runtime Acceptance](phase-63-backend-agent-runtime-acceptance-runbook.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Agent Workflow Rules](../../AGENTS.md)
