# VDR-Suite Changelog

## 2026-10-11 — main documentation reconciliation (post-Phase 69)

The repository's `main` history has advanced beyond the July 2026 Phase-61 documentation entry below. Verified source baseline: `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (merge: R2 Recording Device audio/index/SRT/WebVTT). This section reports **source merged to `main`**, not installed binary identity, live yaVDR acceptance or Phase-69.F compatibility acceptance.

- Numbered Phase 62 identity/security, Phase 63 Agent, Phase 64 Timer orchestration, Phase 65 media sessions, Phase 66 Home, Phase 67 Teletext/HbbTV, Phase 68 Legacy OSD and Phase 69 public API/client hardening reached their bounded closeouts; Phase 70 remains not started.
- Post-Phase-69 Multiuser added Human Account lifecycle, administration UI, Device pairing bootstrap and administrator approval. See [Current State](docs/CURRENT.md) for accepted scope and runtime gates.
- R2 Public-v1 Recording gained backend-scoped canonical Recording identities, bounded collection/folder/genre browsing, now/next EPG and Device-owned Recording session creation/status/stop. Access is grant- and backend-scoped; the R2 candidate is **not automatically promoted** into the accepted Phase-69.F baseline.
- 2026-10-11 changes add bounded Device index-backed duration/preparing status, authorized audio/subtitle track inventory, canonical WebVTT/SRT rendering and Device-owned subtitle selection without exposing native Recording paths or Browser sessions. Existing VDR index files are never silently overwritten for regeneration.
- CI and merge history must not be confused with a real installed yaVDR daemon or Hisense audio/subtitle playback acceptance. Separate deployment authorization and runtime evidence remain necessary.

## Historical changelog entries

## Unreleased — documentation truth refresh

Repository documentation was reconciled against `origin/main` commit `44ae3102ab202ee0dfc974ee0bc9624b9219ad2d` on 2026-07-27.

Documentation changes:

- Phase 61 is recorded as completed and accepted rather than an open feature branch;
- PRs #102 through #108 are recorded as completed Post-Phase 61 Performance Hardening (B1-B4);
- PR #110 Remote/Live Overlay interaction hardening and PR #111 backend-scoped Global Search are recorded as completed cross-cutting features;
- Phase 62 Identity, RBAC and Accountability is the next strict runtime phase;
- current, planned, completed, historical, superseded and deferred states are separated;
- current architecture, parity, provider strategy and gap-matrix assessments were refreshed;
- stale Phase 46/49/58/60.15 current-position claims were removed from active entry points;
- completion/phase coverage guardrails were updated.

## Recent merged milestones

### Backend-scoped Global Search — PR #111

- Recording and EPG title/subtitle/person search for the selected backend;
- query-only SQLite read path with no provider lookup;
- deterministic pagination, detail-owner reuse and mobile timeout/stale-response handling;
- regression fixture with 174,164 EPG events.

### VDR Remote interaction hardening — PR #110

- only the pressed key moves;
- other keys remain visually available;
- internal in-flight guard prevents duplicate dispatch;
- browser calls remain within `VdrSuiteClientApi`.

### Post-Phase-61 Performance Hardening — PRs #102-#108

- faster EPG candidate and time-window queries;
- atomic EPG Genre evidence writes;
- no-op Recording Genre synchronization;
- no-op unchanged EPG upserts;
- throttled completed ETYPES cycles.

### Phase 61 — PR #100

- persistent backend-scoped Recording/EPG metadata, people and Genre platform;
- provider/derived evidence and explicit assignment states;
- query-only Genre browser and EPG Film/Serie/Dokumentation/Sport hierarchy;
- restart persistence and real-system acceptance.

## Authoritative history

- [Current State](docs/CURRENT.md)
- [Current Project Status](docs/development/current-status.md)
- [Completed Phases](docs/development/completed-phases.md)
- [Phase 61 Closeout](docs/development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Closeout](docs/development/post-phase-61-platform-runtime-closeout.md)

Future tagged releases can be summarized above this history without reintroducing branch-specific current-state claims.