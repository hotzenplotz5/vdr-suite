# Phase 56.48 - VDR Timer Action Parser Source Group Split

## Navigation

- [Development Index](index.md)
- [Phase 56.47 - Recording Action Executor Adapter Source Group Split](phase-56.47-executor-adapter-source-group.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Completed on branch `phase-56-vdr-timer-parser-source-group`.

---

## Purpose

Phase 56.48 removes the VDR timer action request parser target from the transitional `ACTIONS_SRC` aggregate.

The target is not a recording-action target. It validates the REST parser for VDR timer operation requests and therefore belongs to the REST/VDR timer action boundary, not to the recording action source aggregate.

---

## Source Group Added

`mk/rest-sources.mk` now defines:

```make
VDR_TIMER_ACTION_REST_PARSER_SRC := \
        api/rest/src/VdrTimerActionRequestParser.cpp
```

This source group is intentionally narrow. It contains only the REST parser source needed by the timer action request parser test.

---

## Migrated Target

The following target moved away from `$(ACTIONS_SRC)`:

```text
test-vdr-timer-action-request-parser
```

Before this phase, the target pulled the broad recording-action aggregate and then added the timer parser explicitly.

After this phase, the target uses:

```make
$(VDR_TIMER_ACTION_REST_PARSER_SRC)
```

---

## Boundary Result

The timer parser test no longer pulls in:

```text
$(RECORDING_ACTION_CORE_SRC)
$(RECORDING_ACTION_REST_PARSER_SRC)
$(RECORDING_ACTION_VDR_CACHE_SRC)
```

This keeps VDR timer action REST parsing separate from recording-action implementation details.

The split also supports the ADR-0037 install and packaging boundary by keeping API-facing parser code distinct from unrelated core/action aggregates.

---

## Verification

The following target check was run locally before committing:

```bash
make test-vdr-timer-action-request-parser
```

The following documentation and phase checks were also run:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Follow-Up

Remaining `ACTIONS_SRC` work after this phase:

```text
legacy/core recording action targets
final ACTIONS_SRC removal audit
```

After the remaining users are migrated, the transitional `ACTIONS_SRC` aggregate can be removed from `mk/action-job-sources.mk`.

---

## Back

- [Back to Development Index](index.md)
