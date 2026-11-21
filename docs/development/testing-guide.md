# Testing Guide

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Developer Onboarding](developer-onboarding.md)

---

## Purpose

This guide documents the VDR-Suite test targets and when to use them during development.

---

## Test Targets

Common local targets:

- `make test-fast` - fast local smoke tests
- `make test-vdr` - VDR, backend, snapshot, adapter and registry tests
- `make test-docs` - documentation structure, local index and reachability checks
- `make test` - full regression suite

---

## Recommended Local Workflow

For VDR architecture or backend work:

```bash
make test-vdr
make test-docs
```

For documentation-only changes:

```bash
make test-docs
```

For full validation before a milestone:

```bash
make test
make test-docs
```

---

## Documentation Checks

Documentation validation is part of the project quality gate.

`make test-docs` verifies:

- required document sections
- local directory index completeness
- reachability from the root README

---

## Back

- [Back to Development Index](index.md)
- [Back to Developer Onboarding](developer-onboarding.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
