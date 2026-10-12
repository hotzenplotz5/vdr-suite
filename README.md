# VDR-Suite

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. VDR remains the native runtime authority; VDR-Suite owns backend scope, policy, orchestration, persistent read models and client-facing contracts.

## Documentation — start here

**New to VDR-Suite?** Open the [Documentation Home](docs/index.md). It gives you a short reading route, a topic map and direct answers to **where to look for what**.

A sensible first reading order:

1. [Project Overview](docs/project-overview.md) — what VDR-Suite does, what remains VDR's responsibility and where the platform is heading.
2. [Project Status Dashboard](docs/project-status-dashboard.md) — a compact view of capabilities and future product work, not an installed-host report.
3. [Choose your topic](docs/index.md#choose-your-topic) — go directly to Live TV, Recordings, permissions, TV pairing, API, installation or development.

If you want to go deeper **after choosing your topic**, continue with the [Current Architecture State](docs/development/current-architecture-state.md), [architecture decisions](docs/adr/index.md), [phase roadmap](docs/planning/roadmap.md) or [historical acceptance evidence](docs/history/index.md).

**Deutsch:** [Projektüberblick auf Deutsch](docs/project-overview.de.md).

Already know what you need? Go straight to [Public API and OpenAPI coverage](api/README.md), [Developer Onboarding](docs/development/developer-onboarding.md), [Current State](docs/CURRENT.md) (the sole repository authority for volatile operational status), or the [Strict Roadmap](docs/planning/roadmap.md). For decisions and evidence see the [Architecture Decision Records](docs/adr/index.md) and [completed history](docs/history/index.md).

For repository automation or a new ChatGPT work session, read [Agent Workflow Rules](AGENTS.md) and the [New Chat Handoff](docs/NEW-CHAT-HANDOFF.md) before changing anything.

## Architecture direction

The stable platform boundary is:

```text
clients
  -> VDR-Suite Control Plane / public Suite contracts
  -> Backend Agent
  -> explicitly owned local provider
  -> VDR native runtime
```

VDR remains authoritative for native VDR state and execution. VDR-Suite owns durable Suite identity, authorization, policy, orchestration, reconciliation and client-facing semantics. Private provider details such as RESTfulAPI, SVDRP, Streamdev or SuiteBridge do not become public client contracts merely because they are reachable.

## Phase direction

Phases 64–69 are completed for their accepted bounded scopes. The next strict numbered phase is Phase 70, which has not started. The binding sequence and prerequisites live in the [Strict Roadmap](docs/planning/roadmap.md); the current cross-cutting Multiuser/device work and operational acceptance state live only in [Current State](docs/CURRENT.md).

Post-Phase-69 Public-v1 Recording browse, Device-owned playback, validated index-duration status, audio-track inventory and WebVTT/SRT subtitle handling have subsequently been integrated into `main` as separately gated R2 work. **Merged code is not proof of installation or real Hisense acceptance**, and the R2 candidate must not be counted as accepted Phase-69.F API compatibility.

Do not copy exact branch heads, active PR numbers or CI checkpoints from this README. Those facts change quickly and belong only in [Current State](docs/CURRENT.md), with live GitHub state re-read before any action.

## Project-decision rule

A chat discussion is not a binding VDR-Suite project decision until the decision is represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract. Repository documents and exact accepted runtime evidence take precedence over conversational memory.
