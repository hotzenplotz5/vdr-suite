# How to Read the VDR-Suite Documentation

## Navigation

- [Repository README](../README.md)
- [Documentation Index](index.md)
- [Current State](CURRENT.md)
- [Historical Documentation](history/index.md)
- [Pruning Review](development/documentation-pruning-review.md)

## Start with the question, not a phase number

For a first visit, read [Project Overview](project-overview.md) and the
[Project Status Dashboard](project-status-dashboard.md). They explain the
product and its capabilities without sending you straight into CI logs or phase
closeouts. The [Documentation Home](index.md) then offers paths by topic,
including playback, pairing, API, development, planning and historical evidence.
A [German introduction](project-overview.de.md) is available.

For deeper reading, choose one route:

- **Understanding the implementation:** [Current Architecture State](development/current-architecture-state.md) -> [Architecture Index](architecture/index.md) -> relevant [ADR](adr/index.md).
- **Building a client or contributing:** [API Guide](../api/README.md) or [Developer Onboarding](development/developer-onboarding.md) -> the matching implementation contract.
- **Checking project progress or planning:** [CURRENT](CURRENT.md) -> [Phase Map](planning/phase-map.md) -> [Strict Roadmap](planning/roadmap.md).
- **Tracing the rationale or acceptance:** [Historical Documentation](history/index.md) -> original phase/slice closeout.

This guide explains **which source to trust**, not a second catalogue of
deployed code. Branch/CI snapshots, exact installed builds and physical
TV/yaVDR acceptance are separate facts. The repository preserves historical
acceptance and design evidence even when its wording is no longer applicable
to a later implementation.

## Current sources: who owns which fact

| Question | Use this authority | What it proves |
| --- | --- | --- |
| Which phase, workstream, CI/PR or deployment is active? | [CURRENT](CURRENT.md) and fresh GitHub/host evidence | Volatile repository/acceptance state; GitHub source alone does not prove an installed daemon |
| What is the strict phase/dependency order? | [Strict Roadmap](planning/roadmap.md) and [Phase Map](planning/phase-map.md) | Binding future order, *not* an independent runtime-status source |
| What ownership is implemented? | [Current Architecture State](development/current-architecture-state.md) | Source-grounded implementation boundary |
| What platform is targeted? | [Target Platform Architecture](architecture/target-platform-architecture.md) and [canonical ADR index](adr/index.md) | Accepted design decisions, not automatic completion |
| Which UI/API operations are documented? | [API guide](../api/README.md) and [OpenAPI subset](../api/openapi.json) | Explicit **partial** source-reviewed Public-v1 contracts; no full API or real-device acceptance guarantee |
| What gaps remain? | [Architecture Gap Matrix](planning/architecture-audit-gap-matrix.md) and [Frontend Parity Register](planning/parity-audit-and-frontend-gap-roadmap.md) | Tracked gaps, subject to validation against current source |
| How do I contribute or verify builds? | [Developer Onboarding](development/developer-onboarding.md), [Documentation Standards](development/documentation-standards.md), [Build System](development/build-system-state.md), [CONTRIBUTING](../CONTRIBUTING.md), [AGENTS](../AGENTS.md) | Workflow/Make source guidance; not a production deployment command |
| What must the next chat read? | [New Chat Handoff](NEW-CHAT-HANDOFF.md) | Handoff context; re-check current GitHub ref before acting |

The [project overview](project-overview.md), its [German](project-overview.de.md)
and [English](project-overview.en.md) introductions, the [status dashboard](project-status-dashboard.md),
and root [ROADMAP](../ROADMAP.md) are **navigational or stable capability summaries**.
They must not become competing copies of CURRENT, acceptance results, or the
binding roadmap. The translated introductions are deliberate language variants,
not redundant files.

## Historical documentation — preserve as evidence

Read the [Historical Documentation Index](history/index.md) to distinguish:

- accepted phase and slice **closeouts** from current implementation instructions;
- old [roadmap sketches](roadmap/README.md) from the binding roadmap;
- early [architecture drafts](architecture/history/README.md), including
  [OSD and federation strategy snapshots](architecture/adr/index.md);
- [early milestone reports](development/milestones.md), [planning milestone
  snapshots](planning/milestones.md), and the historical
  [early status page](development/status/overview.md).

Do not rewrite an accepted historical checkpoint merely because later work
completed the previously pending capability. Add a dated supersession note,
link to the accepted successor, and preserve original proof.

## Document retirement and deletion — evidence, not filename guessing

See the [Pruning Review](development/documentation-pruning-review.md). The first
bounded cleanup removes only two low-information, unlinked development leaf
pages superseded by canonical contribution/process rules. Its findings identify
why the old phase roadmap and ADR/closeout records are **not** wholesale
deletion candidates. No statement here implies semantic line-by-line review of
all 739 Markdown files.

For any further deletion, first prove the replacement authority, unique-content
disposition, inbound link/code reference inventory, index/reachability
consequences, and exact-head GitHub CI. Keep incoming external URLs and Git
history in mind even if no internal references remain. Prefer an in-place
historical label when evidence is distinctive.

## Classification rule for new pages

Use **CURRENT** only for live project state; **planning** for future gates;
**architecture/ADRs** for stable owner contracts; **development** for source-backed
bounded implementations; **history / accepted closeouts** for fixed evidence.
Every overview should link rather than copy volatile milestone or CI details.

## Back

- [Documentation Index](index.md)
- [Repository README](../README.md)
