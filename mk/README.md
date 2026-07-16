# Make Infrastructure

The root `Makefile` remains the public compatibility entrypoint. Source lists,
install rules, test recipes and public test groups are maintained in the
fragments under `mk/`. The root entrypoint only includes those owned fragments;
it does not carry domain recipes itself.


## Build Artifacts

Compiled test programs, tools and daemon binaries are written below the
repository-local `.build/` directory through `BUILD_DIR`. Compiler recipes use
`BUILD_CXX`, which creates that directory only when the recipe actually runs,
so `make -n` remains side-effect free. Runtime databases and external-system
smoke-test state may still use `/tmp` when their temporary lifecycle is part of
the test contract.

`make test-build-artifact-paths` verifies every Make `-o` output and is part of
`test-ci-fast`. `make clean` removes only `.build/` below the repository and the
legacy temporary test database; it refuses an unsafe external `BUILD_DIR`.

## Cached C++ Compilation

`BUILD_CXX` preserves the existing source-based target recipes while compiling
each C/C++ source into a reusable object under `$(OBJECT_CACHE_DIR)`. Cache keys
include the compiler identity, working directory, compile-relevant environment
and effective compile flags. Generated `.d` files track source and header
prerequisites, and per-object file locks make parallel builds safe. The original
argument order is retained for the final link.

The cache contract is covered by `check-cpp-object-cache`. The existing
`test-build-artifact-paths` gate depends on that check, so fast CI verifies both
object reuse and all compiler output paths.

The larger compatibility recipe sets are split by ownership:

- `smoke-targets.mk` — real-system helpers and basic database/CLI targets
- `recording-action-tests.mk` — Recording mutation and REST executor contracts
- `application-tests.mk` — services, dashboards and controller composition
- `domain-tests.mk` — backend, metadata classification and person/EPG domains
- `search-timer-*.mk` — SearchTimer API and workflow contracts
- `runtime-api-tests.mk` — REST routing, HTTP and daemon/runtime targets
- `maintenance-tests.mk` — cleanup, documentation and remaining query contracts
- `vdr-*-tests.mk` — VDR search, runtime/snapshot and timer/real-backend suites

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
