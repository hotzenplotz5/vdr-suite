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
9. Establish the relevant validation for the exact candidate; hosted PR CI may provide the final result after the commit.
10. Commit only explicit files, never `git add .`.
11. Patch only after the intended change is understood.
12. If the diagnosis is uncertain, stop and ask instead of guessing.
13. For interactive terminal blocks, do not use `set -e` or `exit 1`.
14. Do not combine cleanup, patching, checks, commit and push in one large block.
15. Never require a patch file to be downloaded to a mobile device or manually transferred to the VDR server.
16. Deliver server-side changes through a direct repository commit or a repository-hosted Python patch script that runs after `git pull --ff-only`.
17. Do not use here-documents to transfer patches or complete source files. Prefer committed files or a checked Python patch generator.


## Navigation

* [README](README.md)
* [Documentation Index](docs/index.md)
* [Project Overview](docs/project-overview.md)
* [Development Documentation](docs/development/index.md)
* [Architecture Documentation](docs/architecture/index.md)
* [Documentation authority and history guide](docs/documentation-guide.md)
* [Repository agent/workflow rules](AGENTS.md)

---

## Introduction

Thank you for contributing to VDR-Suite.

VDR-Suite is a modern backend platform for VDR environments with a strong focus on maintainability, documentation quality, architecture transparency and long-term evolution.

Contributions should follow the documented architecture and development workflow.

## Before You Start

Start with the [Documentation Guide](docs/documentation-guide.md) to distinguish current and historical sources. Automated repository work additionally follows [AGENTS.md](AGENTS.md).

Please read:

* [Project Overview](docs/project-overview.md)
* [Current Project Status](docs/development/current-status.md)
* [Architecture Documentation](docs/architecture/index.md)
* [Architecture Decision Records](docs/adr/index.md)

## Development Workflow

Work on the **explicitly selected, verified current branch**, never an old phase name copied from a document. Read the current `main` and branch heads, `AGENTS.md`, and the relevant source before editing. Keep changes reviewable and use fast-forward-only updates.

Pick checks by scope: the [Make infrastructure](mk/README.md) describes supported target groups; documentation checks are wired through [`make test-docs`](mk/maintenance-tests.mk), and GitHub Actions verifies the PR's **exact commit**. A docs-only change does not require automatically rerunning the full C++ test graph locally or interacting with a live VDR host. Fix a failing check at its evidenced cause, then verify the final head in hosted CI.

Never deploy, restart services, modify VDR recordings/databases, alter credentials or change a protected branch without the separately required authorization and acceptance gates.

## Documentation Requirements

Documentation is treated as a first-class project artifact.

When adding or changing functionality:

* update relevant documentation
* update architecture documentation if architecture changes
* update ADRs when architectural decisions change
* update project status documentation when phases are completed

## Documentation Quality Gate

The implemented documentation gates verify Markdown headings and relative links, directory index rules, required root-README reachability, and further scoped API/ADR/phase contracts. They **do not** require literal `## Navigation` and `## Back` headings on every page, nor complete machine-readable OpenAPI coverage. See [Documentation Standards](docs/development/documentation-standards.md).

Run `make test-docs` in a verified checkout when appropriate, or use the repository's hosted **docs-check** on the exact PR head as the validation evidence. Do not report a check as passed solely because its command is documented.

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
