# Make Infrastructure

The root `Makefile` remains the public compatibility entrypoint. Source lists,
install rules, test recipes and public test groups are maintained in the
fragments under `mk/`.

## Public Test Entrypoints

- `make test-ci-fast` — required fast C++ and policy regressions
- `make test-ci-frontend` — JavaScript, i18n and frontend ownership contracts
- `make test-ci-packaging` — systemd contract and complete install staging
- `make test-vdr` — broad VDR domain and adapter suite
- `make test-all` — all non-real local test groups
- `make test-manual-real` — explicitly configured real-VDR tests only
- `make test-make-inventory` — strict Make/test topology audit

Only `mk/test-groups.mk` owns the canonical public groups. Individual test
recipes stay near their domain and must be reachable from at least one public
group.

## Strict Inventory

`tools/audit_make_test_inventory.py --check` blocks:

- missing or multiply defined canonical groups
- required tests missing from their public group
- test files without a Make reference
- Make references to missing test files
- test targets outside all public groups
- adjacent duplicate `$(LDFLAGS)` entries
- unclassified `Mock*` or `Test*` sources in broad runtime aggregates

The audit expands simple Make variables, so grouped lists such as
`CI_FAST_TESTS`, `VDR_TESTS`, `EXTENDED_LOCAL_TESTS` and `MANUAL_REAL_TESTS`
are part of the verified dependency graph.

Three legacy names are explicitly documented as intentional runtime variants:
`TestHttpServer`, `MockVdrAdapter` and `TestLiveTransport`. The router test
fixture also temporarily carries `MockVdrTimerActionExecutor` through the
compatibility aggregate. New exceptions require a reviewed rationale.

CI uploads a `make-test-audit-bundle` containing the human report, JSON report,
root Makefile and all Make fragments.

## Removal Rule

Do not delete a test because it looks old. First prove that the protected
behavior no longer exists, that a stronger test covers the same contract, or
that the target refers only to removed code. Real-VDR tests remain manual and
must never be pulled into generic CI accidentally.
