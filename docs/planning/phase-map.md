# VDR-Suite Phase Map

## Purpose

This file is the canonical compact phase-number map. Detailed completed history belongs in [Completed Phases](../development/completed-phases.md); strict forward order and gates belong in the [Roadmap](roadmap.md); volatile status belongs only in [Current State](../CURRENT.md).

Completed history is never renumbered. Bounded hardening after a completed phase remains non-numbered unless the roadmap is explicitly changed before the next phase starts.

## Completed phase ranges

| Range | Status | Track | Result |
| --- | --- | --- | --- |
| Phase 1.x-60 | Completed | Core / backend / frontend foundations | Platform, VDR adapters, multi-backend runtime, actions, SearchTimer, packaging, frontend platform and Recordings 2. |
| Phase 61 | Completed | Suite Metadata and Genre Platform | Persistent Recording/EPG metadata, people, Genres and query-only browse. |
| Phase 62 | Completed | Identity, RBAC and Accountability | Persistent identities, scoped authorization, browser-session security and evidence. |
| Phase 63 | Completed | Backend Agent and Secure Multi-Site Runtime | Secure Agent lifecycle, fenced native execution and provider ownership. |
| Phase 64 | Completed | Timer Intent and Multi-Backend Orchestration | Durable intent/assignment/binding, fulfillment, reconciliation and failover. |
| Phase 65 | Completed | Streaming Gateway and Media Sessions | Recording/Live playback, delivery/output policy and normalized playback semantics. |
| Phase 66 | Completed | Media Home and Browse Experience | Responsive Home, browse/preview, Continue Watching, discovery/history and Golden journeys. |
| Phase 67 | Completed | Broadcast Companion Services: Teletext and HbbTV | Teletext and HbbTV discovery/session/runtime accepted on the supported real yaVDR/browser deployment. |

The compact `Phase 1.x-60` row preserves these historical completed subranges for coverage and traceability: `Phase 1.x-7.x`, `Phase 8.x`, `Phase 9.x-29.x`, `Phase 30.x-44.x`, `Phase 45.x`, `Phase 46.x`, `Phase 47.x-50.50`, `Phase 51.x-55.6`, `Phase 56`, `Phase 57`, `Phase 58.0-58.90b`, `Phase 59.00-59.15e`, and `Phase 60.1-60.15`.

Phase 58 remains a historical umbrella label only.

## Current position

```text
Latest completed numbered runtime phase:
Phase 69 - Public API and Client Compatibility Hardening

Current active numbered runtime phase:
none - Phase 70 - Recommendation and Content Knowledge Graph not started

Next strict numbered runtime phase:
Phase 70 - Recommendation and Content Knowledge Graph

Current active cross-cutting productization workstream:
Multiuser / Account and Backend Access Administration

Current Multiuser architecture slice:
MU.5 - Administration architecture contract - ADR-0067 [COMPLETED]

Current Multiuser runtime slice:
none - successor MU.9 slice not yet selected

Latest completed Multiuser runtime slice:
MU.9A - Account Administration Read UI [IMPLEMENTATION MERGED - PR #418 / EXACT MERGED-MAIN YAVDR PASS / HOSTED CI GREEN]

Completed MU.7 sub-slice:
Grant-set read + desired-state grant mutation [COMPLETED - REAL YAVDR PASS / PR #413 MERGED]

Next product slice:
successor MU.9 slice - exact scope not yet selected

Completed Phase-67 verticals:
Teletext + HbbTV discovery/application-session/presentation-media runtime
```

Phase 66 closed through PR #264. Post-phase Home/Recording work subsequently merged through the accepted Home rebuild without creating a new numbered phase. See [Post-Phase-66 Home Rebuild Closeout](../development/post-phase66-home-rebuild-closeout.md).

## Numbered forward sequence

| Order | Phase | Status | Track | Primary completion direction |
| ---: | --- | --- | --- | --- |
| 1 | Phase 64 | Completed | Timer Intent and Multi-Backend Orchestration | Reliable Timer orchestration and controlled failover. |
| 2 | Phase 65 | Completed | Streaming Gateway and Media Sessions | Authenticated Recording/Live playback and stable playback semantics. |
| 3 | Phase 66 | Completed | Media Home and Browse Experience | Responsive Home and accepted Golden journeys. |
| 4 | Phase 67 | Completed | Broadcast Companion Services: Teletext and HbbTV | Teletext and HbbTV Journeys 8/9 accepted. |
| 5 | Phase 68 | Completed | Legacy OSD Compatibility Bridge | 68.A-G accepted through fenced allowlisted native OSD input; Golden Journey 10 accepted. |
| 6 | Phase 69 | Completed | Public API and Client Compatibility Hardening | 69.A-F accepted; eight stable public-v1 contracts have exact accepted reference-client coverage. |
| 7 | Phase 70 | Next — not started | Recommendation and Content Knowledge Graph | Requires its own accepted runtime ADR before implementation. |

## Phase 64 compact boundary

`TimerIntent -> TimerAssignment -> NativeTimerBinding` with managed native fulfillment, readback/reconciliation and controlled failover. Broad Timer UI remains cross-cutting.

## Phase 65 compact boundary

```text
private media source
  -> explicitly owned provider / ProviderStreamLease
  -> least-transformation adaptation
  -> Streaming Gateway / MediaSession
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
```

Phase 65 is closed. Growing-Recording seek and Live-TV timeshift remain truthful deferred capabilities.

## Phase 66 compact boundary

Binding architecture: ADR-0058. Numbered Phase 66 completed responsive Home, Live Hero/deferred preview, Continue Watching, Recording discovery, history, accessibility and desktop/mobile Golden acceptance.

Later non-numbered work strengthened the same ownership model: Home performance, Series progressive metadata/hierarchy, artwork previews, Genre/Movie presentation, EPG recovery and canonical folder artwork. It does not reopen Phase 66.

## Phase 67 compact boundary

Binding architecture: ADR-0054.

```text
Live Channel / ProgramEvent
  +--> TeletextService -> Page/Subpage
  +--> BroadcastApplication -> HbbTV Application Session
```

Phase 67 is completed. Teletext merged through PR #293 and the HbbTV discovery/session/presentation-media runtime merged through PR #300 / `5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30`. Phase 68 Legacy OSD has accepted 68.A semantic observation, 68.B Agent-local continuity, 68.C authenticated Agent transport, 68.D authorized bounded view sessions, 68.E bounded viewer bindings/multi-viewer delivery, 68.F exclusive controller leasing and 68.G fenced allowlisted native input. Phase 68 is completed; see [Phase 68 Closeout](../development/phase-68-closeout.md) and [Phase 68 Kickoff](../development/phase-68-legacy-osd-kickoff.md).

## Later phases

- Phase 68: Legacy OSD Compatibility Bridge — ADR-0047.
- Phase 69: Public API and Client Compatibility Hardening — ADR-0048.
- Phase 70: Recommendation / Content Knowledge Graph — requires its own accepted runtime ADR before implementation.

## Cross-cutting non-numbered milestones

- Multiuser / Account and Backend Access Administration **[ACTIVE — MU.9A MERGED / SUCCESSOR MU.9 SLICE NOT YET SELECTED]**;
- Broad Timer Product UI **[PLANNED — gated on Multiuser administration]**;
- Audit/Security/Operations surfaces;
- Legacy Basic retirement migration **[COMPLETED]**;
- first-party client family rollout;
- bounded post-phase correctness/performance hardening.

Durable Multiuser authority: [Post-Phase-69 Multiuser Productization Workstream](../development/post-phase69-multiuser-workstream.md).

Multiuser sequence:
`MU.0-MU.5 [DONE] -> MU.6 Account lifecycle [DONE] -> MU.6A lifecycle authority [DONE] -> MU.6B Account item/revision [DONE] -> MU.6C public lifecycle mutation [DONE] -> MU.6D Account CREATE/idempotency [DONE - PR #411] -> MU.7 grant administration [COMPLETED - REAL YAVDR PASS / PR #413 MERGED] -> MU.8 credential/session administration [IMPLEMENTATION COMPLETE - MU.8A RUNTIME ACCEPTANCE PENDING] -> MU.9 admin UI -> later pairing/profiles`.

MU.8 implementation history: `MU.8A Safe credential/session metadata [PR #415 / RUNTIME ACCEPTANCE PENDING] -> MU.8B Session Revoke [PR #416 / FOCUSED YAVDR PASS / HOSTED CI GREEN] -> MU.8C Human-password Credential Revoke [PR #417 / FOCUSED YAVDR PASS / HOSTED CI GREEN]`.

Durable retirement evidence: [Post-Phase-69 P2 Legacy Basic Retirement Closeout](../development/post-phase69-p2-legacy-basic-retirement-closeout.md). This active cross-cutting workstream does not start Phase 70.

## Product acceptance

- Phase 64: Timer scheduling/fail-closed engine journeys.
- Phase 65: Live-TV and Recording playback journeys.
- Phase 66: desktop/mobile Media Home journeys — accepted.
- Phase 67: Teletext Journey 8 and HbbTV Journey 9 — accepted.
- Phase 68: Legacy OSD compatibility journey.
- Phase 69: public/client compatibility hardening.

See [Golden User Journeys](golden-user-journeys.md).

## Verification

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

## Related documents

- [Current State](../CURRENT.md)
- [Roadmap](roadmap.md)
- [Phase 69 Closeout](../development/phase-69-closeout.md)
- [Post-Phase-69 Multiuser Productization Workstream](../development/post-phase69-multiuser-workstream.md)
- [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)
- [MU.5 Administration Architecture Acceptance](../development/post-phase69-mu5-administration-architecture-acceptance.md)
- [MU.6A Human Account Lifecycle Authority Foundation](../development/post-phase69-mu6-account-lifecycle-foundation.md)
- [MU.6B Public Account Item + Revision/ETag](../development/post-phase69-mu6b-public-account-item.md)
- [MU.6C Public Account Lifecycle Mutation](../development/post-phase69-mu6c-public-account-lifecycle-mutation.md)
- [Phase 67 Closeout](../development/phase-67-closeout.md)
- [Phase 67 Teletext Closeout](../development/phase-67-teletext-closeout.md)
- [Phase 66 Closeout](../development/phase-66-closeout.md)
- [Post-Phase-66 Home Rebuild Closeout](../development/post-phase66-home-rebuild-closeout.md)
- [Completed Phases](../development/completed-phases.md)
