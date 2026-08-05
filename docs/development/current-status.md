# VDR-Suite Current Project Status

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main baseline:
a9620179a442155f0860ef3182ca39186ac46a57

Latest merged bounded runtime slice:
Phase 63 Slice 1 - Backend Agent Enrollment and Lease Foundation
PR #137 - Add backend agent enrollment and lease foundation
Accepted source head: bba51455552bab0f1a06c680369c508858b2384b
Accepted tree: 575f49a197cda9ad02da4035b437ee1c32bed2d6
Merge commit: a9620179a442155f0860ef3182ca39186ac46a57
Source CI: VDR-Suite CI #7256, run 31001478896, all five jobs successful
Real yaVDR acceptance: PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS

Active numbered runtime slice:
Phase 63 Slice 2 - Read-only Observation and Snapshot Ingestion Foundation
Draft PR #138 - Define read-only agent observation ingestion contract
Branch: agent/phase63-observation-ingestion-contract
State: Draft contract/closeout; runtime implementation not yet included

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

## Active Phase 63 Slice 2

The binding contract is [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md).

Draft PR #138 establishes the smallest safe read-only ingestion boundary:

- existing `vdr-suite-agent/1` technical authentication;
- backend, Agent, Agent-instance and backend-generation fencing;
- bounded observation domains declared by accepted read-only capabilities;
- independent snapshot generation, producer sequence and resource revision;
- complete baseline before changes;
- exact-next sequence acceptance;
- equivalent replay idempotency;
- conflicting replay and stale-generation rejection;
- explicit `resync-required` on gaps or missing baseline;
- atomic immutable receipt/fact plus ingestion-cursor persistence;
- repository-owned SQLite;
- redacted accountability and diagnostics;
- initial bounded `backend-health` implementation domain.

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
- Keep PR #138 Draft until its exact-head CI is green and the user explicitly approves readiness.
- Do not cross the documented Slice-2 exclusions.
- Never commit or print credentials, cookies, CSRF secrets, provider tokens or secret-bearing process environments.
- No manual SQLite inspection is part of runtime acceptance.

## Exact next action

1. Complete and validate the Slice-1 closeout, Slice-2 contract, status/architecture updates and static contract guard in Draft PR #138.
2. Obtain all required CI jobs on one exact final head.
3. Keep PR #138 Draft until explicit approval.
4. After this contract PR is accepted, begin the smallest runtime implementation with `backend-health` complete-snapshot and exact-next sequence ingestion only.

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
