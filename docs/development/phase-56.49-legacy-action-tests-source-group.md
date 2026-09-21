# Phase 56.49 - Legacy Recording Action Test Source Group Split

## Navigation

- [Development Index](index.md)
- [Phase 56.48 - VDR Timer Action Parser Source Group Split](phase-56.48-vdr-timer-parser-source-group.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Completed on branch `phase-56-legacy-action-tests-source-group`.

---

## Purpose

Phase 56.49 removes the remaining legacy/core recording action test targets from the transitional `ACTIONS_SRC` aggregate.

These targets validate the core recording action model, backend execution path, execution service, job payload factory and executor interface. They do not need REST parser sources or VDR cache sources.

---

## Source Group Added

`mk/action-job-sources.mk` now defines:

```make
RECORDING_ACTION_LEGACY_TEST_SRC := \
        $(RECORDING_ACTION_CORE_SRC)
```

The group is intentionally narrow and exists only to keep the old/core action tests explicit while the transitional aggregate is being removed.

---

## Migrated Targets

The following targets moved from `$(ACTIONS_SRC)` to `$(RECORDING_ACTION_LEGACY_TEST_SRC)`:

```text
test-recording-action
test-recording-action-backend-execution-path
test-recording-action-execution-service
test-recording-action-job-payload-factory
test-recording-action-executor-interface
```

---

## Boundary Result

The migrated legacy/core action tests no longer pull in:

```text
$(RECORDING_ACTION_REST_PARSER_SRC)
$(RECORDING_ACTION_VDR_CACHE_SRC)
```

They only use the recording-action core source group.

This makes the remaining source layout more compatible with the package and install boundary documented in ADR-0037.

---

## Verification

The following target checks were run locally before committing:

```bash
make test-recording-action
make test-recording-action-backend-execution-path
make test-recording-action-execution-service
make test-recording-action-job-payload-factory
make test-recording-action-executor-interface
```

The following documentation and phase checks were also run:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Follow-Up

After this phase, the intended next step is the final `ACTIONS_SRC` removal audit:

```text
Phase 56.50 - Remove Transitional ACTIONS_SRC Aggregate
```

That follow-up should prove that no Makefile or mk include file still needs the broad aggregate before deleting it from `mk/action-job-sources.mk`.

---

## Back

- [Back to Development Index](index.md)
