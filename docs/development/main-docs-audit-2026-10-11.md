# Main documentation reconciliation — 2026-10-11

## Navigation

- [README](../../README.md)
- [Current State](../CURRENT.md)
- [Development Index](index.md)
- [Documentation Standards](documentation-standards.md)

## Source baseline and scope

- Repository: `hotzenplotz5/vdr-suite`.
- Main source checkpoint: `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (2026-10-11).
- Complete recursive Git tree was enumerated (3,766 entries); 736 files use the `.md`, `.rst` or `.adoc` suffix. Other source, config and binary assets are not counted as prose documentation.
- Documentation changes are confined to `docs/main-documentation-audit-2026-10-11` and must never modify production processes, databases or recording directories.
- The tree-wide filename inventory is **not** a claim that the body of every historical phase note has been reread. Historical ADRs, phase plans and accepted closeouts are preserved as timestamped evidence unless proven factually incorrect for their original checkpoint.

## Corrected source-of-truth drift

1. Root changelog had stopped at a 2026-07-27 Phase-61 documentation reconciliation. It now separates the July history from post-Phase-69 merged source developments.
2. Root README, ROADMAP, development/current-status and the new-chat handoff included stale or contradictory MU.9F current-slice statements despite MU.10B acceptance in `CURRENT.md`. They are aligned to `CURRENT.md` as the volatile status authority.
3. Public Device Recording playback and index/track/SRT documents described code as an unfinished isolated candidate. The R2 source is on `main`; exact installed yaVDR daemon and Hisense hardware acceptance remain **unverified by this GitHub-only audit**.
4. The legacy `docs/planning/project-progress.md` template still advertised Phase 58 as currently active and unimplemented identity/metadata functionality that is now accepted. Its snapshot has been reconciled to current phase foundations while preserving the generator delimiters. The implementation dependency map's present-tense Phase-65.D statements were recast as accepted history.
5. Planning index, architecture gap matrix and project overviews (English/German) also contained current-tense Phase-69 or MU.9F statements and were aligned with the authoritative closeouts. The old `docs/development/status/overview.md` now explicitly identifies itself as a historical early-daemon snapshot.
6. The historical Phase-69.F original eight accepted method/resource contracts remain frozen. Account extensions and the separately gated Recording candidate must not be counted as retroactive Phase-69.F acceptance (current matrix: nine accepted plus one R2 candidate).
7. VDR 2.7.9 index generation is potentially destructive to an existing index; the implemented guard refuses existing `index`, `index.vdr` or symlink paths. Nothing in this documentation change triggers index operations.

## Public API documentation bootstrap

- The previously empty `api/README.md` and `api/rest/README.md` now give developers a source-linked API entry point and REST implementation boundary.
- A **partial** OpenAPI 3.1 JSON contract covers only verified `GET /api/v1` and `GET /api/v1/capabilities` response models. Other implemented routes are not silently promoted to stable public-v1 contracts.
- A route-to-OpenAPI drift guard and complete current public-v1 reference remain open work; no CI/runtime acceptance is claimed for this documentation-only slice.

## Verification and limits

- Verified: relative Markdown link targets of the first 18 distinct edited documents (587 references; no missing target paths), exact GitHub `main` merge SHA, recursive file inventory, Device route implementations in `core/daemon/src/RecordingMediaHttpRuntime.cpp`, authorization classification in `core/security/include/SecurityHttpGate.h`, and matrix resource entries.
- Not verified here: a local `make test-docs` run (this environment has no checked-out repository), the installed daemon binary, host permissions, runtime HTTPS/Device route behavior or real VIDAA audio/subtitle acceptance.
- To finish a **full content-level audit of all 736 prose files**, each historical/archive document must be traversed for link resolution, stale current-tense language and contradictions against its time-specific baseline. This checkpoint covers the current-facing authority chain and the newly merged R2 documents; it does **not** assert that archive-wide content validation has already passed.
- Documentation validation must be performed on the final branch head before merge. No PR is approved or merged by this documentation commit.

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
- [README](../../README.md)
