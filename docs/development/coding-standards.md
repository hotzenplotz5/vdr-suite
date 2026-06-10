# Coding Standards

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Developer Onboarding](developer-onboarding.md)

---

## Purpose

This document defines baseline coding expectations for VDR-Suite development.

---

## Development Principles

VDR-Suite development should stay incremental, testable and architecture-first.

Core expectations:

- inspect the existing repository structure before changing code
- read affected files before changing them
- preserve existing interfaces unless a phase explicitly changes them
- keep domain objects backend-neutral
- keep backend-specific logic behind adapter boundaries
- prefer small backward-compatible changes
- add focused tests for new behavior
- update documentation when architecture changes

---

## Backend Principles

Backend work should preserve the separation between runtime state, snapshots, adapters and external systems.

Current backend-related work should respect:

- `BackendNode` as backend identity and configuration model
- `BackendRegistry` as backend collection boundary
- snapshot services as frontend-facing read foundation
- adapter classes as backend-specific integration points
- capability checks before backend-specific behavior is exposed

---

## Test Expectations

For VDR-related work, use focused tests first:

```bash
make test-vdr
make test-docs
```

Use the full regression suite when changes affect shared infrastructure or milestone readiness.

---

## Back

- [Back to Development Index](index.md)
- [Back to Developer Onboarding](developer-onboarding.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
