# VDR-Suite

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. VDR remains the native runtime authority; VDR-Suite owns backend scope, policy, orchestration, persistent read models and client-facing contracts.

## Start here

- [Current State](docs/CURRENT.md) — sole repository authority for volatile operational status
- [New Chat Handoff](docs/NEW-CHAT-HANDOFF.md) — mandatory entry point for a new VDR-Suite chat
- [Strict Roadmap](docs/planning/roadmap.md) — binding forward execution order and phase gates
- [Phase Map](docs/planning/phase-map.md) — compact phase-number map
- [Current Project Status](docs/development/current-status.md) — stable narrative context
- [Target Platform Architecture](docs/architecture/target-platform-architecture.md)
- [Architecture Decision Records](docs/adr/index.md)
- [Completed History](docs/development/completed-phases.md)
- [Agent Workflow Rules](AGENTS.md)

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
