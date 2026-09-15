# VDR-Suite Current Project Status

## Status ownership

Exact operational phase state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context and must not become a second copy of active PR tips or transient CI state.

## Platform position

Latest completed numbered runtime phase: **Phase 66 - Media Home and Browse Experience**.

Current active numbered runtime phase: **none; Phase 67 has not started**.

Next strict numbered runtime phase: **Phase 67 - Broadcast Companion Services: Teletext and HbbTV**.

Phase 66 is completed. Later Home/Recording performance and correctness work is non-numbered post-phase hardening and is also completed for the merged accepted scopes. The consolidated evidence is in [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md).

## Durable foundation

The current platform builds on these completed numbered foundations:

- **Phase 62 - Identity, RBAC and Accountability Foundation** — persistent identity, backend-scoped authorization, browser-session/CSRF protection and append-only accountability;
- **Phase 63 - Backend Agent and Secure Multi-Site Runtime** — Agent identity, generation/lease fencing, observation/command handling and explicit provider ownership;
- **Phase 64 - Timer Intent and Multi-Backend Orchestration** — `TimerIntent -> TimerAssignment -> NativeTimerBinding`, managed native fulfillment, authoritative reconciliation and controlled failover;
- **Phase 65 - Streaming Gateway and Media Sessions** — authenticated Recording/Live MediaSessions, least-transformation delivery/output policy and normalized persistent playback ownership;
- **Phase 66 - Media Home and Browse Experience** — responsive first-party Home, Live hero/preview, Continue Watching, Recording discovery, history and Golden desktop/mobile acceptance.

## Post-Phase-66 delivered state

The post-phase work did not create a new numbered phase. It hardened and completed product behavior on top of the accepted Phase-66 ownership model.

### Home performance and lifecycle

Merged work preserves immediate browse behavior while reducing unnecessary rebuilds and duplicated work:

- Hero browsing does not rebuild programme rails;
- same-backend warm Home return reuses complete projections where valid;
- Recording Discovery/Series work is progressively fenced and avoids global metadata fan-out;
- recent movie expansion preserves the mounted rail and position;
- EPG cache timeout recovery and preview-cache fast paths prevent avoidable stale/fallback presentation.

### Home rebuild / current presentation

The merged post-Phase-66 Home rebuild establishes a coherent current presentation across:

- Home shell and responsive navigation;
- `Was läuft jetzt` / `Was läuft danach`;
- newly recorded content;
- Movies / recent movies / Genres;
- Series -> seasons -> episodes;
- canonical TVScraper/native artwork priority;
- manual Series hierarchy and artwork overrides;
- pilot/miniseries, TV-film and special handling;
- reset-to-automatic hierarchy semantics;
- canonical folder-card metadata/artwork without per-Recording browser metadata reads.

The final folder-poster regression was accepted on the real yaVDR/browser path: `The Exorcist` again shows the canonical red TVScraper portrait in Home -> Aufnahmeordner -> Horror instead of a Recording still.

### Series metadata/artwork follow-up

The Phase-66 closeout historically recorded an unresolved Series metadata/artwork projection symptom. That item is no longer current. Subsequent merged work added explicit unsettled/settled metadata semantics, bounded retries, progressive/scoped completion, hierarchy repair and retained canonical Series ownership. The later Home rebuild completed manual hierarchy/artwork behavior and protected the accepted no-fan-out performance boundary.

### Native Recording editing

Post-Phase-66 native Recording marks/cutting is also merged and documented separately in [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md). VDR remains the canonical marks/cutter authority.

## Forward ordering

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 Media Home and Browse Experience [COMPLETED]
  -> Phase 67 Broadcast Companion Services: Teletext and HbbTV [NEXT; NOT STARTED]
  -> Phase 68 Legacy OSD Compatibility Bridge
  -> Phase 69 Public API and Client Compatibility Hardening
  -> Phase 70 Recommendation and Content Knowledge Graph
```

Phase 67 requires a separate explicit runtime kickoff. Accepted ADR-0054 defines the domain-first Teletext/HbbTV architecture, but accepted planning is not implementation authorization.

## Retained deferred boundaries

The following are not unfinished Phase 65/66 work:

- user-visible growing-Recording seek where unsupported capability remains truthful;
- Live-TV timeshift;
- Legacy OSD compatibility (Phase 68);
- stable public `/api/v1` client contract (Phase 69);
- recommendation/content graph runtime (Phase 70);
- broad Timer Product UI and account/backend access administration as cross-cutting product milestones.

## Related documents

- [Current State](../CURRENT.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 65 Closeout](phase-65-closeout.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Phase 62 Closeout](phase-62-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
