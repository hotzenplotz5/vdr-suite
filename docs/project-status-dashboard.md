# VDR-Suite Project Status Dashboard

## Navigation

- [README](../README.md)
- [Current State](CURRENT.md)
- [Project Overview](project-overview.md)
- [Strict Roadmap](planning/roadmap.md)
- [Golden User Journeys](planning/golden-user-journeys.md)

## Purpose

This page is a capability dashboard, not a second operational-status authority. Exact current phase/branch/CI state belongs only in [Current State](CURRENT.md).

## Functionality at a glance

- **Live TV and programme guide:** browse Channels and EPG and use Suite-owned authenticated playback where supported.
- **Recordings:** browse, play, resume and manage Recordings; VDR-native marks and cutting remain under VDR authority.
- **Home and discovery:** see Now/Next, new Recordings, Movies/Genres and Series with canonical metadata and artwork.
- **Timers:** plan recordings using backend-aware TimerIntent orchestration; a broad polished Timer administration UI remains separately gated.
- **TV features:** Teletext, HbbTV and a bounded Legacy OSD bridge are implemented for their accepted domains.
- **Accounts, devices and integrations:** server-enforced permissions and public client contracts exist, but source-merged Device functionality is not proof of installed TV/yaVDR acceptance.

The table below separates completed foundations from separately gated or future capabilities. It is **not** a report of which features are installed on a particular device.

## Platform capability view

| Area | Durable assessment | Ownership / boundary |
| --- | --- | --- |
| Core daemon and SQLite | Established | Repository-owned persistence/migrations; architecture guards enforce SQL boundaries. |
| Backend registry/access policy | Established | Backend identity/scope and server-owned access decisions. |
| Channels and EPG | Established | Suite read models; VDR remains native schedule authority. |
| Recordings / Recordings 2 | Established | Canonical Recording identity, browse/detail/actions and native editing integrations. |
| Metadata, people, artwork and Genres | Established | Persisted normalized evidence; TVScraper/provider storage remains private/read-only. |
| Identity, RBAC and accountability | Completed foundation | Phase 62. |
| Backend Agent / multi-site trust | Completed foundation | Phase 63. |
| Timer orchestration | Completed numbered domain | Phase 64 `TimerIntent -> TimerAssignment -> NativeTimerBinding`. |
| Streaming Gateway / MediaSession | Completed numbered domain | Phase 65 Recording/Live playback and normalized playback semantics. |
| Media Home / Browse | Completed numbered domain plus merged post-phase hardening | Phase 66 plus post-phase Home rebuild closeout. |
| Native Recording marks/cutting | Completed non-numbered capability | VDR remains canonical marks/cutter authority. |
| Teletext / HbbTV | Completed numbered domain | Phase 67 / ADR-0054 / Phase-67 closeout. |
| Legacy OSD compatibility | Completed numbered domain | Phase 68 / ADR-0047 / Phase-68 closeout. |
| Stable public API/SDK | Completed numbered domain | Phase 69 foundation; post-69 R1–P1 source sequence merged (P1 #524), Unified Acceptance Runner #526 merged. Public API v1 Integrationsabschluss and exact-candidate real-yaVDR/VIDAA acceptance remain open; see [central matrix](development/public-v1-final-hardware-acceptance.md). |
| Additive Account and Device Recording R2 APIs | Separately gated expansion | Later Account contracts and Recording R2/Device source must be tracked independently; merged code does not imply hardware or external-client acceptance. |
| Recommendation/content graph | Later vision, NOT STARTED | Phase 70. |

## Current Home capability

The current merged Home surface includes responsive shell/navigation, Now/Next, newly recorded content, Movies/Genres, Series -> seasons -> episodes, canonical portrait artwork, manual Series hierarchy/artwork overrides and canonical folder-card native metadata/artwork. The later Home rebuild remains within Phase-66 ownership contracts and is documented in [Post-Phase-66 Home Rebuild Closeout](development/post-phase66-home-rebuild-closeout.md).

## Product acceptance view

- Live TV reaches real picture/sound through authorized MediaSession ownership and cleans old routes on replacement.
- Recording playback reports seek/range/growing capabilities truthfully and preserves canonical position semantics.
- Timer creation preserves intent/assignment/native-binding safety and authoritative readback.
- Home browse remains useful independently of preview startup and retains canonical metadata/artwork/playback owners.
- Failure remains classified without blind duplicate mutation or silent provider switching.
- Phase-67 Teletext/HbbTV, Phase-68 Legacy OSD and Phase-69 Public API/client compatibility are completed for their accepted scopes. Phase 70 is not started.

See [Golden User Journeys](planning/golden-user-journeys.md).

## Status rules

- **Operational truth:** [Current State](CURRENT.md).
- **Execution order:** [Strict Roadmap](planning/roadmap.md).
- **Stable architecture:** [Target Platform Architecture](architecture/target-platform-architecture.md) and ADRs.
- **Historical proof:** completed closeouts.
- **Product proof:** Golden User Journeys plus required real-system acceptance.

## Links

- [Current State](CURRENT.md)
- [Post-Phase-66 Home Rebuild Closeout](development/post-phase66-home-rebuild-closeout.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Strict Roadmap](planning/roadmap.md)
- [Completed Phases](development/completed-phases.md)

## Continue: choose your topic

You have seen the functionality overview. **Next, [choose the topic you want to explore](index.md#choose-your-topic)** — such as Live TV, Recording playback, permissions, TV pairing, API or installation.

For technical depth (optional), open [Current Architecture State](development/current-architecture-state.md), [ADRs](adr/index.md), [Phase Map](planning/phase-map.md) or [historical acceptance evidence](history/index.md). For exact current deployment and acceptance claims use [CURRENT](CURRENT.md).

## Back

- [Back to Documentation Index](index.md)
- [Back to README](../README.md)
