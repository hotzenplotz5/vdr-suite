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


## Additional P7 runtime SQL-writer inventory (incremental)

A separate test (tools/test_packaging_p7_sql_writer_inventory.py) surveys core runtime
C++ source and include files and api/rest/src for CREATE/ALTER/DROP TABLE, INDEX,
TRIGGER and VIEW, REINDEX and schema-version PRAGMAs. Adjacent C++ SQL string
literals are joined. Unknown source-file owners fail the inventory regression.
SQL in tests, tools and historical SQL fixtures is outside this runtime scope.

The 33 named owners and GenreIndex .inc remain covered in the original harness.
BackendRuntimeGeneration.cpp is indirectly exercised through BackendAgentRepository.
The additional TimerAssignmentSetRevisionRepository.cpp path is exercised with
an in-memory synthetic legacy assignment epoch 7, idempotent backfill, three
triggers and an epoch 8 insert through real C++. The real
EmbeddedBackendLifecycleService initializer is now also exercised using a
synthetic row surviving fresh C++ process reopens and backup/restore. This is
not historical release evidence. Nine further writers remain PENDING following the verified CI #65 and #67
Series artwork and Timer reassignment exercises (see ledger below):

- core/agent/src/BackendAgentCommandDelivery.cpp
- core/agent/src/BackendAgentCommandReservation.cpp
- core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp
- core/agent/src/BackendAgentNativeTimerModifyAssignment.cpp
- core/agent/src/BackendAgentRecordingCutAssignment.cpp
- core/agent/src/BackendAgentRecordingMarksModifyAssignment.cpp
- core/agent/src/BackendAgentRecordingCutReconciliationRepository.cpp
- core/agent/src/BackendAgentRecordingMarksModifyReconciliation.cpp
- core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp

This file-owner guard does not fingerprint newly added statements inside an already
classified source, exhaust dynamic SQL, or cover data-modifying migrations.
Those are separate open P7 gates. Do not infer production upgrade support.

## Series artwork legacy-column validation candidate (P7)

The existing `core/daemon/tests/test_series_artwork_backend_settings_service.cpp`
invokes `SeriesArtworkBackendSettingsService::ensureSchema()` against a
synthetic pre-TMDB `backend_series_artwork_overrides` table. The P7 extension
seeds a prior override (`revision=7`), re-invokes the real C++ initializer,
and checks that the original identity, URL, revision, and default values in
all five added columns remain unchanged. The same test exercises targeted
EPG scraper cache invalidation, including backend and media-type isolation.
The packaging Noble runner now executes the existing
`test-suite-bridge-epg-metadata-resolver` Make target, which includes this
actual C++ service test. This is a *candidate CI gate*, not verified CI
success or a historical daemon-produced database. Keep the source PENDING
until the new exact-head CI completes and the migration evidence is reviewed.

The service also modifies cache rows during explicit provider-setting updates;
its delete statements are not startup-schema migration. The service does
not establish forward-schema rejection or real manual-install compatibility.

## Timer reassignment legacy-column candidate (P7)

The existing end-to-end C++ regression
`core/timers/tests/test_timer_assignment_reassignment_service.cpp` exercises
the actual `TimerAssignmentReassignmentRepository.cpp` through
`TimerAssignmentReassignmentService::reassign()` and
`TimerAssignmentRepository::findControlledReplacement()`. The added
test creates a real controlled-replacement row, removes only the later
`old_operation_revision` column in its **in-memory synthetic fixture**, then
replays the request so the real C++ `ensureReassignmentSchema()` executes the
legacy `ALTER TABLE ADD COLUMN ... DEFAULT ''`. It verifies durable evidence,
old assignment/epoch/backend/reason identity, the empty default, SQLite foreign
keys, no duplicate handover record, repeated read idempotence, and removal of
TEMP expectation-table/trigger objects. Existing cases cover valid
`before_dispatch` and `verified_absent` transitions as well as denial of
stale/unknown results.

The disposable Ubuntu 24.04 P7 package CI now invokes
`make test-phase64-timer-reassignment-failover` on the actual C++ sources.
Until the new exact-head CI passes, this is **implemented candidate coverage**,
not a passed migration proof. Even after success, this remains synthetic,
in-memory SQLite evidence rather than a former production daemon-created
database. The TEMP triggers enforce atomic per-request fencing but are not
persistent schema migration objects; do not equate their execution with a
supported manual-install upgrade or production-release approval.

## Verified regression status and next agent candidate

- **CI #65**, exact source SHA `c1338fd1841afae7c2de4e1ff8089054a296f0cc`,
  5/5 jobs SUCCESS: real Series-artwork legacy columns C++ regression.
- **CI #67**, exact source SHA `21bf12350ec751cf022d717a105009aa9b50399a`,
  5/5 jobs SUCCESS: real Timer reassignment synthetic legacy-column test,
  native fencing/replay cases and transient-trigger cleanup. These two DDL
  source files are now classified as **exercised**, not production-migration
  approved. Synthetic in-memory migration remains distinct from manual-install
  database provenance and persistent WAL/restore testing.

The next **candidate** validates the actual
`BackendAgentCommandRepository::ensureSchema()` path in
`BackendAgentCommandDelivery.cpp` through the existing phase-63 C++ test:
a synthetic prior `backend_agent_command_results` table without
`result_evidence`, a preserved real command-result record, and an obsolete
dormant-capability trigger are restored through real C++ `ALTER TABLE` and
`DROP TRIGGER`. The test also verifies the restored row/default, foreign-key
consistency, idempotent initialization and an in-memory SQLite backup into a
second independent connection. It is connected to experimental Noble CI via
`make test-phase63-command-delivery-runtime`, but remains **PENDING** until
exact-head CI success. An in-memory SQLite backup does not prove WAL handling
under concurrent writes or upgrade support for production databases.
