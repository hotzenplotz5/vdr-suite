# P7 SQLite schema-owner inventory (experimental)

This ledger is intentionally **not** a production upgrade approval. It is
tracked by the read-only source regression
`tools/test_packaging_p7_schema_owner_inventory.py`. The checker inventories
C++ `Repository::ensureSchema()` implementations which create tables in
`core/*/src/*Repository[Schema].cpp`. The coverage label means the real C++
initializer is invoked in `test-packaging-p7-runtime-schema` on an isolated,
synthetic/historical fixture, with idempotent reopening and SQLite checks.
It does **not** mean a published package/database upgrade is supported.

## Inventory boundary

- **33** named repository C++ sources are exercised by the P7 rehearsal,
  including security identity/accounts, browser sessions, device pairing,
  credentials/grants/audit, media playback/session state, basic EPG, public
  recording identity/cache, series hierarchy, operations, and timers.
- Series artwork `origin` migration is exercised on synthetic pre-column
  data, alongside the provider negative cache.
- The `GenreIndexRepository` initializer is additionally exercised through
  `core/metadata/src/GenreIndexRepositorySchema.inc` and the
  `$(METADATA_GENRE_SRC)` make variable; it is counted separately from the 33 named C++ sources.
- **0** named `ensureSchema` sources remain unexercised in this narrow
  inventory. This is **not** complete SQL-writer or real-upgrade coverage.
- There are also table-creating helpers **outside** the named
  `ensureSchema` pattern (e.g. recording-cut reconciliation and timer
  reassignment). They need a separate full SQL-write-path survey before
  accepting the "all runtime schema owners" gate.

## Outstanding named initializers

None within the narrowly defined named-repository pattern. SQL writers in
other source files, and real historical/manual-install migration compatibility,
remain separate open workstreams.

## Separate acceptance gates

A source-derived coverage inventory can prevent unreviewed new
`ensureSchema` owners but cannot prove database compatibility. Before the
first supported package release, explicitly classify *manual-install*
source versions and real daemon-produced databases; verify quiesced SQLite
and WAL snapshot/restore; reject unknown/future markers and incompatible
binary/database pairs; and separately test systemd start/stop and admin
onboarding. There is no supported released earlier Debian/Ubuntu package
predecessor in the experimental `exp1 → exp2` sequence.
