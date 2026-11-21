# SearchTimer runtime mutation policy wiring

## Phase

Phase 54.0 - SearchTimer runtime mutation policy wiring

## Motivation

The yaVDR live check proved that the SearchTimer workflow can be planned and mapped,
but direct `/api/searchtimers` runtime mutation was still reported as unavailable.
The next step is to wire the Create/Update/Delete services into `DaemonRuntime`
without accidentally enabling production backend mutation.

## Runtime boundary

The daemon now constructs the SearchTimer Create/Update/Delete services, request parsers,
and result serializers. The API router receives a runtime mutation policy executor
instead of the raw RESTfulAPI command executor.

The policy executor is closed by default. It returns a controlled failure message
instead of delegating to the real RESTfulAPI executor. This makes the runtime path
observable while preserving the production safety boundary.

## Expected live behaviour

Direct calls to `/api/searchtimers`, `/api/searchtimers/update`, and
`/api/searchtimers/delete` should no longer fail with `searchtimer create service unavailable`.
They should fail with the explicit runtime mutation policy gate message until a later
phase adds an operator-controlled enablement policy.

## Safety result

No real VDR SearchTimer mutation is enabled by this phase. The raw RESTfulAPI executor
remains available internally for already existing diagnostic paths, while the public
SearchTimer mutation API receives the closed policy executor.

## Verification

- `make test-search-timer-runtime-mutation-policy-executor`
- `make test-search-timer-create-request-parser`
- `make test-search-timer-update-request-parser`
- `make test-search-timer-delete-request-parser`
- `make test-api-router`
- `make daemon`
- `make test-docs`
- `make test-phase`

## Next phase

Phase 54.1 should add an explicit operator-controlled runtime enablement policy and a
safe local live smoke procedure for creating and deleting `TEST-Amerika-title-only`.


## Navigation

- [Development index](index.md)
- [Current status](current-status.md)
- [Completed phases](completed-phases.md)


## Back

Back to [development index](index.md).
