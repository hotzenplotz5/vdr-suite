# VDR-Suite Roadmap — navigation

## Navigation

- [Project README](README.md)
- [Current State](docs/CURRENT.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Golden User Journeys](docs/planning/golden-user-journeys.md)
- [Planning Index](docs/planning/index.md)
- [Completed Phase Evidence](docs/development/completed-phases.md)

## Where to find the authoritative plan

This root page is a **navigation entrypoint**, not an independently maintained phase-status report. Its earlier duplicate lists of the completed/active/next phase and MU.6–MU.10 PR results have been consolidated into the owning records:

| Question | Canonical source |
| --- | --- |
| What is completed/active/next right now? | [CURRENT](docs/CURRENT.md), verified against the actual GitHub refs |
| What may be implemented next, in what order? | [Strict roadmap](docs/planning/roadmap.md) |
| Which numbered phases are complete? | [Phase map](docs/planning/phase-map.md) and [completed phase evidence](docs/development/completed-phases.md) |
| Where is Multiuser/Device work tracked? | [Multiuser workstream](docs/development/post-phase69-multiuser-workstream.md) and [CURRENT](docs/CURRENT.md) |
| What is the longer-term platform product direction? | [Platform productization roadmap](docs/planning/platform-productization-roadmap.md) |
| Which user-visible outcomes are acceptance gates? | [Golden User Journeys](docs/planning/golden-user-journeys.md) |

The historical closeout markers below are retained as **stable completed evidence** required by the repository's existing phase/security guards, not as a second living implementation-status list:

- Phase 69 - Public API and Client Compatibility Hardening [COMPLETED] — [Phase 69 Closeout](docs/development/phase-69-closeout.md).
- Legacy Basic retirement **[COMPLETED]** — [P2 Legacy Basic Retirement Closeout](docs/development/post-phase69-p2-legacy-basic-retirement-closeout.md).

Accepted historical closeouts, for example the [Phase-69 closeout](docs/development/phase-69-closeout.md) and [Legacy Basic retirement](docs/development/post-phase69-p2-legacy-basic-retirement-closeout.md), are immutable evidence of their own checkpoints. Additive Account/Device/Recording work does not retroactively alter earlier accepted phase scope.

The accepted Multiuser foundation also retains these **historical** milestone markers (not a live implementation queue): **MU.6C** public Account lifecycle, **MU.6D** Account CREATE/idempotency, **MU.7** permission-grant administration with **REAL YAVDR** acceptance, and its **MU.8** Account-security successor. Consult [the canonical Multiuser workstream](docs/development/post-phase69-multiuser-workstream.md) for exact slice evidence and [CURRENT](docs/CURRENT.md) for present acceptance gates.

**Maintenance rule:** Do not add a second living status register here. Update the primary source and link to it; verify current branch, CI and runtime claims separately.

## Back

- [README](README.md)
- [Documentation index](docs/index.md)
