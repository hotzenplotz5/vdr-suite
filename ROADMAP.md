# VDR-Suite Roadmap

## Navigation

- [README](README.md)
- [Current State](docs/CURRENT.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Implementation Dependency Map](docs/planning/implementation-dependency-map.md)
- [Golden User Journeys](docs/planning/golden-user-journeys.md)
- [Architecture Gap Matrix](docs/planning/architecture-audit-gap-matrix.md)
- [ADR Index](docs/adr/index.md)
- [Completed History](docs/development/completed-phases.md)

## Purpose

This root file is only the compact roadmap entry point.

The authoritative execution order, phase prerequisites, coherent verticals and exit criteria live in [docs/planning/roadmap.md](docs/planning/roadmap.md). Volatile completed/active/next phase status lives in [docs/CURRENT.md](docs/CURRENT.md). Historical exact implementation evidence belongs in closeouts and [Completed History](docs/development/completed-phases.md).

## Current position

```text
Latest completed numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Completed Phase-65 product verticals:
65.A - Existing-Recording playback
65.B - Live-TV playback
65.C - Recording delivery performance and media output/transcode settings

Current Phase-65 product vertical:
65.D - Client playback abstraction

Accepted Phase-65.D bounded work:
65.D.1 - Persistent Browser Playback Shell
65.D.2 - Recording Playback Controls and Seek
normalized Recording audio/subtitle selection
browser-local Volume/Mute
continuous-fMP4 browser MSE forward-buffer control
compatibility timeline ownership
exact non-zero HLS Recording resume synchronization

Active Phase-65.D architecture gate:
ADR-0056 playback semantic consolidation
```

Phase 65 is active. The earlier planning label `65.C - Recording seek and growing-recording semantics` is superseded by the implementation history. Phase 65.C actually continued from completed-Recording startup/progressive delivery into the backend-scoped media-transcode/output policy and Web settings accepted through PR #208. Truthful range/seek/growing capability remains a Phase-65 invariant, but it is not the 65.C product label.

The old roadmap's separate 65.D compatibility-escalation block was absorbed by the demonstrated compatibility/performance work completed inside 65.C. The replacement **65.D - Client playback abstraction remains active**. PR #219 closed the continuous-fMP4 browser MSE forward-buffer/backpressure gap. PR #220/#221 closed compatibility timeline drag ownership and established the sync-safe/fail-closed exact non-zero HLS Recording resume boundary.

The next coherent 65.D work is governed by [ADR-0056](docs/adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md) and the [Phase 65.D Playback Semantics Consolidation Contract](docs/development/phase-65d-playback-semantics-consolidation.md): normalized `MediaPlaybackContract`, canonical playback-owner lifecycle publication, continuity/discontinuity plus presentation-generation semantics, and classified playback failures. Read-only diagnostics follow semantic correctness; shared fMP4/MSE helper deduplication is technical debt rather than a phase gate.

Completed-Recording arbitrary time-seek and stop/resume are accepted for the supported progressive-fMP4 and HLS restart-seek profiles. Growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain deferred and must stay truthful.

## Revised strict forward sequence

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [ACTIVE]
  -> Phase 66 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 67 - Legacy OSD Compatibility Bridge
  -> Phase 68 - Public API and Client Compatibility Hardening
  -> Phase 69 - Recommendation and Content Knowledge Graph
```

Phase 66 is backed by accepted ADR-0054. Runtime remains blocked until Phase 65 closes and Phase 66 is explicitly started.

## Cross-cutting product milestones

The following are deliberately not inserted as numbered phases:

- Account and Backend Access Administration;
- Broad Timer Product UI;
- Audit/Security/Operations product surfaces;
- Legacy Basic retirement;
- first-party browser/TV/native/Kodi client rollout.

The Broad Timer Product UI depends on the completed Phase-62 identity/RBAC foundation, the completed Phase-64 Timer engine and the still-required account/backend access administration product boundary. It may proceed alongside Phase 65 without blocking Streaming.

## Roadmap rule

Completed phases and accepted Phase-65 verticals/slices are not renumbered or reopened merely because optional product surfaces or still-deferred capabilities remain.

Future not-yet-started phases may be reordered only through explicit repository planning/architecture reconciliation. Provider additions, diagnostics, administration products and frontend work must be classified explicitly rather than smuggled into a completed phase or vertical.

For all details, use the [Strict Roadmap](docs/planning/roadmap.md).
