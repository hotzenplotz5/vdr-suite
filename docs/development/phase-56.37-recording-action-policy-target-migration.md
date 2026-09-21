# Phase 56.37 - Recording Action Policy Target Migration

## Navigation

- [Development Index](index.md)
- [Phase 56.36 - Remaining ACTIONS_SRC Audit](phase-56.36-remaining-actions-src-audit.md)

---

## Status

Completed.

---

## Purpose

Phase 56.37 migrates the next safe group of recording-action Makefile targets away from the broad `ACTIONS_SRC` compatibility aggregate.

The migrated targets were selected from the Phase 56.36 audit as policy and safety targets that do not require REST parser, HTTP transport, RESTfulAPI executor, VDR backend registry or controller wiring.

---

## Migrated Targets

The following targets now use `RECORDING_ACTION_CORE_SRC` instead of `ACTIONS_SRC`:

- `test-recording-action-policy-gated-execute`
- `test-recording-action-backend-policy-safety`
- `test-recording-action-safety-contract`
- `test-recording-action-backend-policy-safety-json-contract`

The JSON contract target still explicitly adds:

```make
core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp
```

This keeps the serializer dependency visible instead of hiding it inside the transitional aggregate.

---

## Boundary Result

The policy and safety tests now compile against recording-action core sources only, with the safety-result JSON serializer added explicitly where needed.

This removes unrelated dependencies from these targets:

- `RecordingActionValidationRequestParser.cpp`
- `SearchTimerPreviewEpgCache.cpp`
- REST controller sources
- HTTP transport sources
- RESTfulAPI executor sources
- VDR backend registry sources

---

## Remaining ACTIONS_SRC Work

The following groups remain out of scope for this phase:

- RESTfulAPI executor targets
- REST controller targets
- preview service/controller targets
- real smoke helpers
- VDR timer parser target
- validation controller target

These still need explicit source groups or dedicated inspection before migration.

---

## Packaging Impact

This phase strengthens the future recording-action core package boundary.

More tests now prove that policy and safety logic can compile without REST/VDR/HTTP side dependencies.

---

## Verification

Local verification used for this phase:

```bash
make test-recording-action-policy-gated-execute
make test-recording-action-backend-policy-safety
make test-recording-action-safety-contract
make test-recording-action-backend-policy-safety-json-contract
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
