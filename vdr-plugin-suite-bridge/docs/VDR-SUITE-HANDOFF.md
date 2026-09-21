# VDR-Suite / SuiteBridge Coordination Handoff

## Status

```text
SUPERSEDED as a live status handoff
Historical coordination detail preserved separately
```

The former SB.10 coordination handoff is preserved unchanged at [Historical SB.10 Handoff](history/VDR-SUITE-HANDOFF-SB10.md). Its old branch, commit, binary and “next active slice” values remain useful evidence but are not current repository truth.

Use these current entry points before any coordinated SuiteBridge work:

- [VDR-Suite New Chat Handoff](../../docs/NEW-CHAT-HANDOFF.md)
- [VDR-Suite Current State](../../docs/CURRENT.md)
- [Current Architecture State](../../docs/development/current-architecture-state.md)
- [Strict Roadmap](../../docs/planning/roadmap.md)
- [Architecture Gap Matrix](../../docs/planning/architecture-audit-gap-matrix.md)
- [Plugin README](../README.md)
- [Plugin ADR-0001](ADR-0001-plugin-role-and-native-integration-strategy.md)
- [Plugin Roadmap](ROADMAP.md)

## Coordination rules that remain valid

- VDR Core remains native runtime authority.
- SuiteBridge is a private plugin/adapter boundary, not the public browser API or Suite domain model.
- Browser code never calls SuiteBridge or SVDRP directly.
- VDR locks/pointers do not cross into asynchronous database/network work.
- Capabilities, schema versions and degradation behaviour are explicit.
- Read-only observation and mutation contracts remain separate.
- No remote privileged mutation is enabled before Phase 62 authorization/accountability and Phase 63 generation/lease/fencing prerequisites exist.
- Plugin-side payload-bound changes must be coordinated end to end with transport, backend parser and tests.

## Current RMETA contract

At the 2026-07-27 repository baseline, current main consistently uses:

```text
maximum people: 128
maximum payload: 65,535 bytes
```

The 52-person `Pulp Fiction` regression remains complete within that bound. Draft PR #101's plugin-only 256-person / 256-KiB increase is not a compatible current contract and must not be merged piecemeal.

## Current metadata role

SuiteBridge may provide bounded TVScraper/VDR evidence to asynchronous Suite workers. VDR-Suite persists backend-scoped target bindings, people relations, provider evidence, Genre assignments and public read models. Normal Genre and Global Search GET requests do not call SuiteBridge.

## Before starting a new SuiteBridge slice

1. fetch and compare current `origin/main`;
2. verify the current plugin branch/tag/version instead of reusing historical SB.10 values;
3. classify work against Phase 62/63 prerequisites and active ADRs;
4. coordinate capability/schema/payload changes across plugin, transport, backend and tests;
5. require shared-object build, staged installation and real VDR acceptance where native behaviour changes;
6. update central Current/Handoff/Gap documents when implementation truth changes.