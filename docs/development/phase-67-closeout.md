# Phase 67 Closeout — Broadcast Companion Services: Teletext and HbbTV

**Phase 67 is completed.**

## Accepted repository identity

- Teletext vertical: PR #293, merge `d92e7907637122368908ce4c7564a0332db9c487`.
- HbbTV discovery/session/presentation-media vertical: PR #300.
- Accepted PR #300 head: `c7a2efaf81b4182f73ed44454ff78a19560bd94e`.
- PR #300 merge to `main`: `5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30`.
- Hosted CI: VDR-Suite CI run #9014 / `35528054941` — all six jobs successful.

This closeout records the numbered Phase-67 boundary.

## Delivered product boundary

Phase 67 provides explicit Teletext and HbbTV Broadcast Companion domains without using Legacy OSD as the primary product contract.

### Teletext

- normalized backend/channel/service/page/subpage identity;
- private provider transport behind Suite-owned contracts;
- backend-generation and embedded-lifecycle fencing;
- authorized reads;
- first-party 25 x 40 / 1000-cell rendering and navigation;
- Live-TV companion integration;
- accepted Golden Journey 8.

Teletext-specific evidence remains in [Phase 67 Teletext Closeout](phase-67-teletext-closeout.md).

### HbbTV

- normalized broadcast-application discovery with backend/service/application provenance;
- explicit authorized `BroadcastApplicationSession` ownership;
- normalized bounded input instead of raw key/browser/plugin commands;
- session-scoped presentation and media state for the first-party Live-TV HbbTV surface;
- backend-generation, discovery-revision, channel/application-context and session fencing;
- deterministic stale-context rejection/close behavior;
- preservation of Phase-65 MediaSession ownership for Suite-owned media;
- no public raw URL, JavaScript, browser-command or provider-control tunnel;
- accepted real yaVDR/browser launch/use/close/relaunch behavior on the supported deployment profile;
- accepted Golden Journey 9.

The supported deployment evidence is intentionally narrower than universal HbbTV compatibility.

## Phase-67 acceptance gate

| Gate | Result | Evidence |
| --- | --- | --- |
| Real Teletext service through Suite domain contracts | PASS | PR #293 and Teletext closeout |
| Deterministic page/subpage navigation and bounded provider ownership | PASS | Teletext tests + real acceptance |
| Real HbbTV application discovery | PASS | PR #300 discovery/provider/session contracts |
| Suite-owned HbbTV launch/close session boundary | PASS | PR #300 session runtime + real browser acceptance |
| No public raw plugin/browser command API | PASS | architecture/security/static guards |
| Channel/backend/application stale-context fencing | PASS | session/discovery runtime tests and accepted real behavior |
| Phase-65 media ownership preserved | PASS | HbbTV media runtime and ownership tests |
| Representative yaVDR/broadcast acceptance and cleanup | PASS | supported real deployment acceptance |
| Golden Journeys 8 and 9 | PASS | Teletext closeout + HbbTV real browser acceptance |

## Recording playback regression hardening carried by PR #300

PR #300 also repaired regressions found during real-system validation: the composed Recording fallback controls are install-staging guarded, prewarm remains paused/silent, and HLS restart-seek only activates from truthful duration/resume evidence while preserving proven resume capability across replacement. These fixes preserve completed Phase-65 playback ownership and do not redefine Phase 67 as a Recording-playback phase.

## Retained boundaries

This closeout does not claim pixel-perfect compatibility with every broadcaster or proprietary HbbTV extension, unrestricted arbitrary-web browsing, a universal public browser-engine contract, Legacy OSD as the implementation of Teletext/HbbTV, Phase-69 public API stabilization, Phase-70 recommendation work, or unrelated cross-cutting Timer/administration milestones.

## Next numbered phase

```text
Phase 67 - Broadcast Companion Services: Teletext and HbbTV [COMPLETED]
  -> Phase 68 - Legacy OSD Compatibility Bridge [NEXT; NOT STARTED]
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

Phase 68 requires its own explicit runtime start.

## Related documents

- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [ADR-0054 Broadcast Companion Services](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [Phase 67 Teletext Closeout](phase-67-teletext-closeout.md)
