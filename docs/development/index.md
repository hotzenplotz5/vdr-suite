# Development Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)

## Purpose

This is the stable navigation page for implementation contracts and evidence. Volatile phase/branch/CI truth belongs only in [Current State](../CURRENT.md).

## Current orientation

- [Current State](../CURRENT.md)
- [Current Project Status](current-status.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Current Architecture State](current-architecture-state.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 66 Media Home and Browse Experience](phase-66-media-home-browse-experience.md)
- [ADR-0058 Media Home Architecture](../adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR-0054 Broadcast Companion Services](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)

## Completed and historical evidence

- [Completed Phases](completed-phases.md)
- [Latest Completed Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Home Performance Hardening](post-phase-66-home-performance-hardening.md)
- [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 65 Closeout](phase-65-closeout.md)
- [Phase 64 Final Closeout](phase-64-closeout.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)

Historical exact heads, CI runs, hashes and runtime evidence stay in their closeouts. Historical slice documents remain traceability records and do not authorize successor implementation.

## Current media/home position

Phase 65 - Streaming Gateway and Media Sessions is completed.

Phase 66 - Media Home and Browse Experience is completed. The later Home performance/Recording Discovery/Series metadata-artwork work and the final Home rebuild are also completed as non-numbered hardening; they do not reopen Phase 66. Use [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md) for the consolidated current Home truth.

Phase 67 - Broadcast Companion Services: Teletext and HbbTV is next but has not started. ADR-0054 defines its accepted architecture and a separate explicit runtime kickoff is required.

Growing-Recording seek and Live-TV timeshift remain truthful deferred media capability work and do not reopen Phase 65.

## Developer references

- [Developer Onboarding](developer-onboarding.md)
- [Build System State](build-system-state.md)
- [GitHub Actions Status Handoff](github-actions-status-handoff.md)
- [Web Client API Contract Snapshot](web-client-api-contract-snapshot.md)
- [Person API](person-api.md)

## Placement rules

- volatile operational truth -> `docs/CURRENT.md`
- stable architecture -> `docs/architecture/` and accepted ADRs
- future order and gates -> `docs/planning/`
- active bounded implementation contracts -> `docs/development/`
- completed exact evidence -> closeouts
- non-numbered hardening -> dedicated post-phase closeout without inventing a numbered phase

## Back

- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
