# VDR-Suite Changelog

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