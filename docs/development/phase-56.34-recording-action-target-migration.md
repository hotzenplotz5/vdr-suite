# Phase 56.34 - Recording Action Target Migration

## Navigation

- [Development Index](index.md)
- [Phase 56.33 - Recording Action Source Group Split](phase-56.33-recording-action-source-group-split.md)

---

## Status

Completed.

---

## Purpose

Phase 56.34 migrates the first Makefile targets away from the broad `ACTIONS_SRC` compatibility aggregate introduced in Phase 56.33.

The goal is to prove that the split source groups can be used by individual build targets without pulling in unrelated REST or VDR dependencies.

This phase intentionally keeps the migration small and safe.

---

## Migrated Targets

### Recording Action Execution Result JSON Serializer Test

Before:

```make
$(ACTIONS_SRC)
```

After:

```make
$(RECORDING_ACTION_CORE_SRC)
```

Reason:

```text
The serializer test only needs recording-action core sources.
It does not need the REST request parser.
It does not need the SearchTimer preview EPG cache.
```

### Recording Action Validation Request Parser Test

Before:

```make
$(ACTIONS_SRC)
```

After:

```make
$(RECORDING_ACTION_CORE_SRC) \
$(RECORDING_ACTION_REST_PARSER_SRC)
```

Reason:

```text
The parser test needs recording-action validation/result code and the REST parser.
It does not need the SearchTimer preview EPG cache.
```

---

## Boundary Result

This phase removes an unnecessary VDR dependency from two narrow tests:

```text
test-recording-action-execution-result-json-serializer
  -> RECORDING_ACTION_CORE_SRC

test-recording-action-validation-request-parser
  -> RECORDING_ACTION_CORE_SRC
  -> RECORDING_ACTION_REST_PARSER_SRC
```

The broader `ACTIONS_SRC` aggregate remains available for targets that still require further inspection.

---

## Packaging Impact

This is a build hygiene step toward these future package boundaries:

| Future package candidate | Impact |
| --- | --- |
| `vdr-suite-recording-action-validation` | First test target now proves the core group can compile without REST/VDR cache. |
| `vdr-suite-rest-implementation` | REST parser source is explicitly required only where needed. |
| VDR/SearchTimer preview package | VDR cache no longer leaks into the two migrated tests. |

---

## Follow-up

Remaining follow-up work:

1. Inspect additional `ACTIONS_SRC` users.
2. Migrate targets that only need core validation/result code.
3. Keep REST parser targets explicit.
4. Keep VDR cache dependency only for targets that actually need preview cache behavior.
5. Eventually remove `ACTIONS_SRC` once all users are migrated.

---

## Verification

Local verification used for this phase:

```bash
make test-recording-action-execution-result-json-serializer
make test-recording-action-validation-request-parser
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
