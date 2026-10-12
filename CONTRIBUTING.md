# Contributing to VDR-Suite

Thanks for your interest in VDR-Suite! Small, focused contributions are welcome: a reproducible bug report, a test, clearer documentation or a bounded source-code improvement.

**New here?** You don't need to read all phase archives or architecture decisions before making your first contribution. Start with the [developer quickstart](docs/development/quickstart.md) in a disposable checkout.

## Your first contribution

1. Read [what the project does](docs/getting-started.md) and [which area owns your topic](docs/development/quickstart.md#4-locate-the-code-you-need).
2. Confirm a bug or document the improvement in a way someone else can reproduce. Distinguish a tested repository commit from what is actually installed on a VDR host.
3. Work on a fresh branch from the **currently verified** `main`; keep unrelated cleanups out of the change.
4. Run the smallest relevant test first. For a documentation-only change, use `make test-docs`; for affected runtime or client code, see [Make/test groups](mk/README.md).
5. Open a pull request describing **what changed, why, how it was tested, and what remains unverified**. Include the exact candidate commit and relevant risk/compatibility notes.

A PR does not require proof of a deployed real VDR when it only modifies non-deploying documentation or isolated tests. Conversely, green CI does not prove installed yaVDR or Hisense playback acceptance.

## Common source paths

| Area | Where to start |
| --- | --- |
| REST API / OpenAPI | [API guide](api/README.md), `api/rest/` |
| Native VDR domains and Backend Agent | [Core overview](core/README.md), `core/vdr/`, `core/agent/` |
| Browser interface | `web/frontend/` |
| Data models and persistence | [Database overview](database/README.md), `core/sqlite/` |
| Packaging and installation staging | [Packaging status](packaging/README.md) |
| Architecture and earlier decisions | [Current Architecture](docs/development/current-architecture-state.md), [ADRs](docs/adr/index.md) |
| Standalone Hisense TV app | [VIDAA repository](https://github.com/hotzenplotz5/vdr-suite-vidaa) |

## Build and test without touching a real VDR

Use an isolated environment and follow the [CI-derived prerequisites](docs/development/quickstart.md):

```bash
make test-docs
make -j2 --output-sync=target test-ci-fast
make daemon
```

These commands build or test the checked-out source; they do not install it into a live system. Choose targeted Make groups when possible; [`mk/README.md`](mk/README.md) describes the actual groups and deliberately separate real-system tests.

## Safety and review boundaries

Please inspect the exact code and tests before editing, keep changes reviewable, and do not bypass authorization, idempotency, backend ownership or rollback requirements. Do **not** deploy, restart services, alter live recordings/databases, reset credentials or modify a protected branch without separate explicit authorization.

**Automation and repository-operation rules:** [`AGENTS.md`](AGENTS.md) remains binding for automated assistants and scripted changes, including the detailed patch, terminal, provenance, testing and deployment constraints. These are intentionally not presented as 17 introductory hurdles for new humans.

[Documentation standards](docs/development/documentation-standards.md) explain source ownership, local links and historical evidence. Accepted closeouts and ADRs are not to be retrospectively rewritten as though they described the current installed system.

## Pull-request checklist

- [ ] Change is focused and includes a reason or reproducible problem.
- [ ] Documentation/contract updates match changed behavior where applicable.
- [ ] Relevant tests are identified, with **actual outcomes** or an explicit not-run reason.
- [ ] Runtime, data migration, permission and client-compatibility risks are acknowledged.
- [ ] No unverifiable release/installation/TV acceptance is claimed.

Use the [repository PR template](.github/PULL_REQUEST_TEMPLATE.md).

## Back

- [Getting started](docs/getting-started.md)
- [Developer quickstart](docs/development/quickstart.md)
- [Documentation index](docs/index.md)
