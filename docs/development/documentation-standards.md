# Documentation Standards

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Developer Onboarding](developer-onboarding.md)

---

## Purpose

This document defines documentation rules for VDR-Suite.

---

## Required Sections

Every markdown document should contain:

- a top-level `#` heading
- a `## Navigation` section
- a `## Back` section

---

## Index Rules

Every new markdown document must be linked from the local directory index.

Every documentation file under `docs/` must remain reachable from the root `README.md`.

---

## Validation

Run:

```bash
make test-docs
```

This validates:

- markdown document structure
- local directory indexes
- reachability from the root README

---

## Definition of Done

Documentation changes are complete only when:

- the new or changed document follows the required structure
- the relevant local index links the document
- `make test-docs` passes

---

## Back

- [Back to Development Index](index.md)
- [Back to Developer Onboarding](developer-onboarding.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
