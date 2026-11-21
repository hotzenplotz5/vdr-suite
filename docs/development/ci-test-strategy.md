# CI Test Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Build System State](build-system-state.md)

---

## Purpose

This document defines the intended CI test tiers for VDR-Suite.

The goal is to keep push and pull-request feedback fast while preserving full historical regression coverage through scheduled and manual workflows.

---

## Problem

The historical `make test` and `make test-vdr` targets are valuable but large.

Running both targets on every push makes normal development feedback slow, especially because many tests compile dedicated single-test binaries and several broad targets overlap by domain.

The solution is not to delete coverage, but to run the right level of coverage at the right time.

---

## CI Tiers

### Fast Push / Pull Request CI

Workflow:

```text
.github/workflows/ci.yml
```

Runs on:

```text
push
pull_request
```

Required checks:

```text
make test-docs
make test-phase
make test-ci-fast
make daemon
```

Purpose:

- verify documentation integrity
- verify phase consistency
- compile and run the fast architecture regression set
- compile the daemon
- keep normal feedback fast

---

### Full Regression CI

Workflow:

```text
.github/workflows/full-regression.yml
```

Runs on:

```text
workflow_dispatch
nightly schedule
```

Required checks:

```text
make test
make test-vdr
```

Purpose:

- preserve historical regression coverage
- catch broad cross-domain breakage
- validate larger target groups before releases, tags or major merges

The full regression jobs may run in parallel because they are independent target groups.

---

## Make Targets

### test-ci-fast

`test-ci-fast` is the normal GitHub Actions push target.

It includes:

- the existing `test-fast` architecture/core smoke group
- `test-api-router`
- `test-restful-api-vdr-timer-action-executor`
- current SearchTimer preview EPG cache tests

This target should stay intentionally small and representative.

### test

`make test` remains the broad non-VDR-domain regression target.

It should not be removed and should remain available for manual and scheduled full regression.

### test-vdr

`make test-vdr` remains the broad VDR/SearchTimer/domain regression target.

It should not be removed and should remain available for manual and scheduled full regression.

---

## Maintenance Rules

- Do not add every new test to push CI by default.
- Add a representative phase-specific test to `test-ci-fast` only when it protects the active development focus.
- Keep heavy historical tests in `make test` or `make test-vdr` unless they are obsolete and intentionally removed in a dedicated cleanup phase.
- Prefer adding narrow domain groups over expanding one giant push target.
- Before a release or tag, run the full regression workflow.

---

## Related Commands

Fast local development check:

```text
make test-docs
make test-phase
make test-ci-fast
make daemon
```

Heavy local check when needed:

```text
make test
make test-vdr
```

---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
