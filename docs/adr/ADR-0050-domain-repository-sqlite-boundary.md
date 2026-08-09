# ADR-0050: Domain Repository SQLite Boundary

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Component Architecture](../architecture/suite-components.md)
- [ADR-0002: SQLite as Central Metadata Database](ADR-0002-sqlite.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](ADR-0044-timer-intent-assignment-native-timer-model.md)

---

## Status

Accepted

Date: 2026-07-17

---

## Context

VDR-Suite uses SQLite as the daemon-owned durable store for jobs, recordings, EPG data, backend capability probes, the Phase 61 Suite metadata platform and later Control-Plane-owned durable orchestration state.

The repository already contains domain-specific persistence implementations outside `core/recordings/`:

- `core/vdr/src/EpgEventRepository.cpp`
- `core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp`
- `core/vdr/src/VdrRecordingCacheRepository.cpp`

These classes are repositories by responsibility. They own schema creation, SQL statements, binding, row decoding and backend-scoped persistence for their domain models.

The earlier architecture guard allowed direct SQLite access only below `core/sqlite/` and `core/recordings/src/`. That rule had two defects:

1. It rejected legitimate domain repository implementations.
2. It allowed every source file below `core/recordings/src/`, including services or helpers that were not repositories.

Phase 61 also introduced `core/metadata/tests/test_metadata_schema_contract.cpp`. That test uses the SQLite C API intentionally to validate real schema constraints, triggers and foreign-key behavior against a temporary database. It is not production persistence logic.

Phase 64 introduces Control-Plane-owned `TimerIntent` persistence under `core/timers/`. The Timer domain follows the same repository-only rule: direct SQLite belongs in `core/timers/src/*Repository.cpp`, not in Timer services, schedulers, controllers or generic helpers.

A broad exception for all of a domain source directory or all test directories would weaken the architecture boundary.

---

## Decision

Direct SQLite API usage is permitted only in narrowly defined roles.

### SQLite infrastructure

Generic connection, execution and database infrastructure belongs in `core/sqlite/`.

### Domain repositories

Domain SQL belongs in implementation files whose names end with `Repository.cpp` and which are located in an explicitly approved domain repository directory.

Approved directories are:

- `core/recordings/src/`
- `core/vdr/src/`
- `core/metadata/src/`
- `core/security/src/`
- `core/agent/src/`
- `core/timers/src/`

This permits the existing domain repositories and the Phase-64 TimerIntent repository. It does not permit direct SQLite access in similarly located services, controllers, schedulers, adapters, serializers, persistence helpers or generic helpers.

### SQLite and schema contract tests

A test may use the SQLite C API directly only when its exact repository path is registered as a database or schema contract test.

The initial registered schema-contract test is:

- `core/metadata/tests/test_metadata_schema_contract.cpp`

Additional exact tests may be registered only when they must inspect SQLite itself. Ordinary repository tests should use the repository plus the generic `Database` abstraction against a temporary or in-memory database.

There is no wildcard permission for `tests/`.

### Architecture guard

`tools/check_architecture.py` enforces the boundary using:

- SQLite infrastructure prefixes;
- approved domain repository prefixes plus the `Repository.cpp` filename contract;
- exact registered SQLite/schema contract tests and runtime exceptions;
- positive and negative self-check cases executed by `make test-architecture`.

The Timer-domain self-check explicitly permits `core/timers/src/TimerIntentRepository.cpp` while rejecting similarly located non-repository persistence helpers.

### Future additions

Another direct SQLite location requires all of the following:

1. A demonstrated persistence or schema-contract responsibility.
2. A narrow path or naming rule.
3. Positive and negative guard checks.
4. Synchronized architecture documentation.
5. No broader exception than the concrete requirement needs.

---

## Consequences

Positive:

- VDR-domain repositories remain colocated with their domain models.
- Suite metadata, security, Agent and Timer persistence can remain domain-owned without broad SQLite access.
- TimerIntent persistence can be introduced without granting Timer schedulers or services direct database access.
- Services and controllers cannot acquire SQLite access accidentally.
- Schema tests can verify real SQLite behavior without opening all test directories.
- Future boundary changes become explicit and reviewable.

Trade-offs:

- New repository domains require a deliberate guard update.
- Renamed repository files must retain the `Repository.cpp` contract.
- Exceptional schema tests must be registered individually.

---

## Rejected Alternatives

### Move all domain repositories into one persistence module

Rejected because EPG, Recording, Security, Agent and Timer persistence own different domain models and invariants. Colocation with the owning domain keeps those invariants explicit while the architecture guard prevents arbitrary SQL use.

### Allow all files below approved domain source directories

Rejected because services, schedulers, adapters and serializers would then be able to bypass repository boundaries.

### Allow all tests to use SQLite directly

Rejected because ordinary service and controller tests should exercise repositories or database abstractions rather than the SQLite C API.

### Ban direct SQLite use in schema tests

Rejected because selected schema contracts must inspect real query, constraint, trigger and foreign-key behavior.

---

## Related Decisions

- [ADR-0002: SQLite as Central Metadata Database](ADR-0002-sqlite.md)
- [ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation](ADR-0034-searchtimer-warm-epg-cache-and-change-invalidation.md)
- [ADR-0035: Lazy Recording Loading and Backend-Scoped Refresh](ADR-0035-lazy-recording-loading-and-backend-scoped-refresh.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](ADR-0044-timer-intent-assignment-native-timer-model.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Architecture Documentation](../architecture/index.md)
- [Back to Documentation Index](../index.md)
