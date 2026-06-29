# Phase 56.45 - Remaining ACTIONS_SRC Re-Audit

## Navigation

- [Development Index](index.md)
- [Phase 56.36 - Remaining ACTIONS_SRC Audit](phase-56.36-remaining-actions-src-audit.md)
- [Phase 56.41-56.44 - ACTIONS_SRC Batch Migration](phase-56.41-56.44-actions-src-batch.md)

---

## Status

Completed as an audit-only phase.

No production code, Makefile source-group definitions or test target migrations are changed in this phase.

---

## Purpose

Phase 56.45 re-audits the remaining Makefile users of the transitional `ACTIONS_SRC` aggregate after the Phase 56.41-56.44 batch migration.

The goal is to keep the next migrations deterministic by grouping the remaining targets by responsibility before changing more build rules.

---

## Audit Command

```bash
git grep -n '\$(ACTIONS_SRC)' -- Makefile mk docs

git grep -n -B2 -A10 '\$(ACTIONS_SRC)' -- Makefile
```

---

## Result Summary

Remaining `ACTIONS_SRC` users are now concentrated in Makefile test targets.

Historical documentation references remain in earlier Phase 56 documents and are intentionally not changed:

- `docs/development/phase-56.34-recording-action-target-migration.md`
- `docs/development/phase-56.35-recording-action-core-target-migration.md`

---

## Remaining Makefile Groups

### 1. VDR Timer Parser

- `test-vdr-timer-action-request-parser`

This target is unrelated to recording-action execution and should be split separately through a VDR timer parser source group.

### 2. REST Execution Controller

- `test-recording-action-execution-controller-safety-preview`
- `test-recording-action-execution-controller-execute-body-policy-gate`
- `test-recording-action-execution-controller-policy-execute`
- `test-recording-action-execution-controller-policy-safety`
- `test-recording-action-execution-controller`
- `test-recording-action-validation-controller`

These targets combine recording-action core services with REST controller code, backend registry support and, for preview or executor paths, explicit HTTP mock support.

A later source group should make this controller boundary explicit instead of depending on `ACTIONS_SRC`.

### 3. Registry, Capability and Executor Adapter Tests

- `test-recording-action-execution-service-registry-safety`
- `test-recording-action-backend-executor-registry-capabilities`
- `test-restfulapi-recording-action-executor-capabilities`
- `test-restfulapi-executor-preserves-http-error-status`
- `test-real-client-readonly-recording-action-executor-gate`

These targets validate registry, capability and executor-adapter behavior. They need focused migration because some use mock HTTP transport and some use the real `BasicHttpClient` boundary.

### 4. Legacy / Core Recording Action Tests

- `test-recording-action`
- `test-recording-action-backend-execution-path`
- `test-recording-action-execution-service`
- `test-recording-action-job-payload-factory`
- `test-recording-action-executor-interface`

These are older broad targets that still depend on the transitional aggregate. They should be split only after confirming whether each test needs core services, parser support, VDR cache support or no compiled source at all.

---

## Recommended Next Steps

Suggested follow-up phases:

1. Split REST execution controller test targets away from `ACTIONS_SRC`.
2. Split registry and capability targets into explicit recording-action executor support groups.
3. Split the VDR timer request parser target into its own VDR timer REST parser source group.
4. Re-audit legacy/core recording-action tests and remove the aggregate where safe.
5. Remove `ACTIONS_SRC` only after no Makefile target uses it.

---

## Verification

The audit phase should be verified with documentation and phase-map checks only:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
