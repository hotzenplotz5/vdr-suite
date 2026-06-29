# Phase 56.50 - Remove Transitional ACTIONS_SRC Aggregate

## Navigation

- [Development Index](index.md)
- [Phase 56.49 - Legacy Recording Action Test Source Group Split](phase-56.49-legacy-action-tests-source-group.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Completed on branch `phase-56-remove-actions-src-aggregate`.

---

## Purpose

Phase 56.50 removes the transitional `ACTIONS_SRC` aggregate from the recording action source map.

The aggregate was originally kept as a compatibility bridge while recording-action, REST parser, VDR cache, executor, controller and legacy test targets were migrated to explicit source groups.

After the Phase 56.41-56.49 migrations, no Makefile or mk include user should depend on the broad aggregate anymore.

---

## Removed Source Group

`mk/action-job-sources.mk` no longer defines:

```make
ACTIONS_SRC := \
        $(RECORDING_ACTION_CORE_SRC) \
        $(RECORDING_ACTION_REST_PARSER_SRC) \
        $(RECORDING_ACTION_VDR_CACHE_SRC)
```

---

## Boundary Result

The recording-action build graph now uses explicit groups instead of the broad compatibility aggregate:

```text
RECORDING_ACTION_CORE_SRC
RECORDING_ACTION_REST_PARSER_SRC
RECORDING_ACTION_VDR_CACHE_SRC
RECORDING_ACTION_PREVIEW_SRC
RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC
RECORDING_ACTION_REST_CONTROLLER_SRC
RECORDING_ACTION_EXECUTOR_ADAPTER_SRC
RECORDING_ACTION_LEGACY_TEST_SRC
```

This makes target ownership clearer and reduces accidental coupling between unrelated layers.

The removal supports the package and install boundary documented in ADR-0037 by keeping source groups aligned with core, REST, VDR/cache and test-specific responsibilities.

---

## Audit

Before removal, the repository was checked for remaining users:

```bash
grep -R -n 'ACTIONS_SRC' Makefile mk || true
```

After removing the aggregate, the same audit was run again to verify that no `Makefile` or `mk/` reference remained.

---

## Verification

The following representative targets were run locally before committing:

```bash
make test-recording-action
make test-recording-action-validation-request-parser
make test-recording-action-preview-controller
make test-recording-action-execution-controller
make test-restfulapi-executor-preserves-http-error-status
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

With `ACTIONS_SRC` removed, the remaining Phase 56 work should return to package readiness and install layout:

```text
make install staging contract
runtime/CLI/doc/manpage install paths
package prerequisite audit
Phase 56 packaging readiness audit
```

---

## Back

- [Back to Development Index](index.md)
