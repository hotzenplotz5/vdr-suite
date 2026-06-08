# Contributing to VDR-Suite

## Navigation

* [README](README.md)
* [Documentation Index](docs/index.md)
* [Project Overview](docs/project-overview.md)
* [Development Documentation](docs/development/index.md)
* [Architecture Documentation](docs/architecture/index.md)

---

## Introduction

Thank you for contributing to VDR-Suite.

VDR-Suite is a modern backend platform for VDR environments with a strong focus on maintainability, documentation quality, architecture transparency and long-term evolution.

Contributions should follow the documented architecture and development workflow.

## Before You Start

Please read:

* [Project Overview](docs/project-overview.md)
* [Current Project Status](docs/development/current-status.md)
* [Architecture Documentation](docs/architecture/index.md)
* [Architecture Decision Records](docs/adr/index.md)

## Development Workflow

Typical workflow:

```bash
git checkout phase-2-actions

make test

make test-docs
```

Implement changes.

Run all checks again:

```bash
make test

make test-docs
```

Commit only after all checks pass.

## Documentation Requirements

Documentation is treated as a first-class project artifact.

When adding or changing functionality:

* update relevant documentation
* update architecture documentation if architecture changes
* update ADRs when architectural decisions change
* update project status documentation when phases are completed

## Documentation Quality Gate

The documentation quality gate verifies:

* navigation sections
* back sections
* valid markdown links
* complete index structures
* documentation reachability from README.md

Run:

```bash
make test-docs
```

before committing documentation changes.

## Architecture Changes

Architecture-related changes should update:

* architecture documentation
* ADR documentation
* current status documentation

Avoid undocumented architectural changes.

## Commit Messages

Use concise commit messages.

Examples:

```text
Phase 11.2: add snapshot query service

Docs: update snapshot architecture

Fix: resolve HTTP runtime diagnostics serialization
```

## Pull Requests

Pull requests should include:

* purpose of the change
* affected components
* test results
* documentation updates

## Coding Principles

Preferred goals:

* simple designs
* explicit ownership
* clear service boundaries
* testability
* documentation first
* architecture consistency

---

## Back

* [Back to README](README.md)
