# Phase 56.35 - Recording Action Core Target Migration

## Navigation

- [Development Index](index.md)
- [Phase 56.33 - Recording Action Source Group Split](phase-56.33-recording-action-source-group-split.md)
- [Phase 56.34 - Recording Action Target Migration](phase-56.34-recording-action-target-migration.md)

---

## Status

Completed.

---

## Purpose

Phase 56.35 continues the gradual migration away from the broad `ACTIONS_SRC` compatibility aggregate.

This phase migrates two safety-focused recording-action execution service tests to the narrower `RECORDING_ACTION_CORE_SRC` source group.

The goal is to keep proving that core recording-action logic can compile without the REST request parser and without the VDR/SearchTimer preview cache.

---

## Migrated Targets

The following Makefile targets were migrated:

```make
test-recording-action-execution-service-capability-safety
test-recording-action-execution-service-safety
```

Before:

```make
$(ACTIONS_SRC)
```

After:

```make
$(RECORDING_ACTION_CORE_SRC)
```

---

## Reasoning

Both tests exercise `RecordingActionExecutionService` safety behavior.

They do not require:

```text
RecordingActionValidationRequestParser.cpp
SearchTimerPreviewEpgCache.cpp
```

Therefore they should not pull the complete compatibility aggregate.

---

## Boundary Result

After this phase, four recording-action targets no longer use `ACTIONS_SRC`:

```text
test-recording-action-execution-result-json-serializer
test-recording-action-validation-request-parser
test-recording-action-execution-service-capability-safety
test-recording-action-execution-service-safety
```

The first, third and fourth targets now use only:

```make
$(RECORDING_ACTION_CORE_SRC)
```

The REST parser test explicitly uses:

```make
$(RECORDING_ACTION_CORE_SRC)
$(RECORDING_ACTION_REST_PARSER_SRC)
```

---

## Packaging Impact

This strengthens the future `vdr-suite-recording-action-validation` package candidate because multiple core tests now compile without REST/VDR side dependencies.

It also keeps the REST parser and VDR cache dependencies visible instead of hiding them in `ACTIONS_SRC`.

---

## Follow-up

Remaining follow-up work:

1. Inspect the remaining `ACTIONS_SRC` users.
2. Migrate pure core tests to `RECORDING_ACTION_CORE_SRC`.
3. Keep controller targets explicit about REST and VDR dependencies.
4. Keep real RESTfulAPI executor targets separate from core validation tests.
5. Remove `ACTIONS_SRC` only after all users are intentionally migrated.

---

## Verification

Local verification used for this phase:

```bash
make test-recording-action-execution-service-capability-safety
make test-recording-action-execution-service-safety
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
