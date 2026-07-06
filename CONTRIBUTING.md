# Contributing to VDR-Suite

## Mandatory Project Rules

These rules are non-negotiable and must be checked before every implementation change.

1. No blind patches.
2. Inspect the relevant files before proposing or applying changes.
3. Prefer GitHub inspection for repository state and file context before local patch commands.
4. Prove the current state and the root cause first.
5. Explain the intended minimal change before patching.
6. Do not use `sed` for code changes unless it is clearly the safest and smallest tool for the job.
7. Do not generate CRAP patches, broad rewrites or unrelated cleanup.
8. Keep patches minimal, reviewable and tied to the proven cause.
9. Run the matching checks before commit.
10. Commit only explicit files, never `git add .`.
11. Patch only after the intended change is understood.
12. If the diagnosis is uncertain, stop and ask instead of guessing.
13. For interactive terminal blocks, do not use `set -e` or `exit 1`.
14. Do not combine cleanup, patching, checks, commit and push in one large block.


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
