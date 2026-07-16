# Make and Test Infrastructure Audit

Date: 2026-07-16

Status: active maintenance baseline

## Purpose

This audit establishes a reproducible baseline for the GNU Make build and test
infrastructure before ADR-0042 and the next runtime phases. It does not remove
product tests or alter product behavior.

## Confirmed findings

1. The top-level `Makefile` has grown into a large mixed build, test, helper and
   cleanup entry point.
2. Public test groups such as `test-ci-fast` and `test-vdr` are extended from
   multiple included Make fragments, so their effective contents cannot be
   understood from one file.
3. GitHub Actions runs documentation checks, `test-ci-fast` and the daemon
   build, but does not run a canonical complete test group.
4. Frontend contract tests and install staging exist as useful test surfaces,
   but need explicit CI group ownership.
5. Some tests, including the RESTfulAPI recording trash contract, exist outside
   a clearly authoritative public test group.
6. Production source aggregates contain files with `Mock` or `Test` naming.
   Their runtime role must be classified before they are retained, renamed or
   removed from production linkage.
7. Source aggregates such as `VDR_SRC`, `REST_ROUTER_SRC` and `DAEMON_SRC`
   duplicate substantial wiring and increase fixture drift risk.
8. The manual `/tmp` cleanup list and repeated per-test compilation are
   maintenance and build-time debt, not reasons to delete tests.

## Audit tool

`tools/audit_make_test_inventory.py` scans:

- the top-level `Makefile`;
- every `mk/**/*.mk` fragment;
- all declared Make targets and dependencies;
- C++ and JavaScript test source files;
- public test-group reachability;
- repeated public group definitions;
- duplicate linker-flag lines;
- test-support-looking production source references;
- unresolved Make path references requiring source/destination classification.

The `--check` mode fails for hard structural errors such as a target recipe
being implemented in multiple Make fragments. Migration findings remain
warnings until the consolidation assigns an explicit target state.

## Required consolidation order

1. Keep the inventory audit green in CI.
2. Define one authoritative test-group fragment.
3. Add explicit frontend and packaging CI groups.
4. Establish a real `test-all` group and classify manual real-VDR tests.
5. Split source aggregates into production and test-support sets.
6. Split large test fragments by domain.
7. Move generated binaries into a repository build directory.
8. Replace the manual cleanup list.
9. Remove only targets, aliases or fragments proven redundant after the new
   groups and guards cover them.

## Non-goals

- no CMake migration;
- no product behavior change;
- no REST API change;
- no deletion of safety, permission, mutation or real-backend tests without a
  separate evidence-based decision;
- no broad source removal based only on file naming.
