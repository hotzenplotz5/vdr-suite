# Dependencies — historical sketch and current pointers

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Build Requirements](build-requirements.md)
- [Packaging Status](../packaging/README.md)
- [Current State](CURRENT.md)

## Current dependency authority

The old Phase-0–4 checklist below predates the implemented daemon, security, media and Backend Agent subsystems. It is **not a supported dependency manifest** and does not imply that RecTools or TVScraper are required runtime packages.

Use the current [Makefile](../Makefile), [make infrastructure](../mk/README.md) and [build-system guide](development/build-system-state.md) for build target dependencies. Installation/staging requirements and package readiness are documented in [packaging](../packaging/README.md). Private VDR providers and potential add-ons have separate ownership/compatibility contracts; the reserved [module directories](../modules/README.md) contain no installable add-ons in the audited tree.

## Preserved Phase-0–4 dependency sketch (historical)

- Phase 0: no runtime dependencies envisaged.
- Phase 1: SQLite.
- Phase 2: VDR, external RecTools and TVScraper were listed as candidates.
- Phase 3: REST framework was marked open.
- Phase 4+: not yet defined at the time.

These items record *the original early plan*, not current unimplemented features or prerequisites.

## Back

- [Documentation Index](index.md)
- [README](../README.md)
