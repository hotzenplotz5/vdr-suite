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
Phase 67 - Broadcast Companion Services: Teletext and HbbTV

Current active numbered runtime phase:
Phase 68 - Legacy OSD Compatibility Bridge

Next strict numbered runtime phase:
Phase 69 - Public API and Client Compatibility Hardening

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
| 5 | Phase 68 | Active — 68.A | Legacy OSD Compatibility Bridge | Read-only OSD observation first; control remains later in Phase 68. |
| 6 | Phase 69 | Planned after Phase 68 | Public API and Client Compatibility Hardening | Stable `/api/v1` and independent-client contracts. |
| 7 | Phase 70 | Vision | Recommendation and Content Knowledge Graph | Explainable provenance-aware recommendations. |

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

Phase 67 is completed. Teletext merged through PR #293 and the HbbTV discovery/session/presentation-media runtime merged through PR #300 / `5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30`. Phase 68 Legacy OSD is active at 68.A read-only OSD observation; see [Phase 68 Kickoff](../development/phase-68-legacy-osd-kickoff.md).

## Later phases

- Phase 68: Legacy OSD Compatibility Bridge — ADR-0047.
- Phase 69: Public API and Client Compatibility Hardening — ADR-0048.
- Phase 70: Recommendation / Content Knowledge Graph — requires its own accepted runtime ADR before implementation.

## Cross-cutting non-numbered milestones

- Account and Backend Access Administration;
- Broad Timer Product UI;
- Audit/Security/Operations surfaces;
- Legacy Basic retirement migration;
- first-party client family rollout;
- bounded post-phase correctness/performance hardening.

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
- [Phase 67 Closeout](../development/phase-67-closeout.md)
- [Phase 67 Teletext Closeout](../development/phase-67-teletext-closeout.md)
- [Phase 66 Closeout](../development/phase-66-closeout.md)
- [Post-Phase-66 Home Rebuild Closeout](../development/post-phase66-home-rebuild-closeout.md)
- [Completed Phases](../development/completed-phases.md)
