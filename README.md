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

The current numbered platform sequence is defined only by the [Strict Roadmap](docs/planning/roadmap.md). In particular, Phase 65 Streaming follows completion of the reliable Phase-64 Timer orchestration engine; a broad polished Timer UI is separately gated on account/backend access management and is not a prerequisite for Streaming.

Do not copy exact branch heads, active PR numbers or CI checkpoints from this README. Those facts change quickly and belong only in [Current State](docs/CURRENT.md), with live GitHub state re-read before any action.

## Project-decision rule

A chat discussion is not a binding VDR-Suite project decision until the decision is represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract. Repository documents and exact accepted runtime evidence take precedence over conversational memory.
