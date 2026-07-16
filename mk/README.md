# Make Infrastructure

The top-level `Makefile` remains the compatibility entrypoint. Reusable source
aggregates, install rules, test recipes and public test groups are owned here.

## Public Test Entrypoints

All public test groups are defined only in `test-groups.mk`:

- `test-ci-fast` — required fast C++ and policy regression path
- `test-ci-frontend` — JavaScript, i18n and frontend ownership contracts
- `test-ci-packaging` — systemd contract plus complete install staging
- `test-vdr` — broad VDR domain and adapter suite
- `test-all` — aggregate local non-real suite
- `test-manual-real` — explicitly configured real-VDR tests only

Individual test recipes stay close to their domain. A fragment may add a leaf
test as a prerequisite of another domain target, but it must not redefine a
public group.

## Inventory Guard

`tools/check_make_test_manifest.py` checks:

- public group ownership and required reachability
- missing test source references
- orphan C++ and JavaScript test files
- test targets outside public groups
- test-support-looking sources in broad aggregates
- adjacent duplicate linker flag entries

CI runs the guard in strict mode. The current baseline is:

- 340 classified test targets
- 318 referenced C++ and JavaScript test source files
- zero ungrouped targets
- zero orphan test source files
- zero adjacent duplicate linker-flag entries

Three legacy names are explicitly documented as intentional runtime variants:
`TestHttpServer`, `MockVdrAdapter` and `TestLiveTransport`. New `Mock*` or
`Test*` sources in broad production aggregates remain strict failures unless a
reviewed rationale is added.

The CI uploads `make-test-audit-bundle`, containing the complete report and the
Make fragments used for that run.

## Current Migration Rule

Do not delete a test because it looks old. First prove one of the following:

1. the protected behavior no longer exists;
2. a stronger test covers the same contract;
3. the target references removed production code;
4. the test cannot be reached from any supported build or runtime path.

Production and test-support source aggregates will be separated incrementally
without changing daemon behavior in the same commit.
