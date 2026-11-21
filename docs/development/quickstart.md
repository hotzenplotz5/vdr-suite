# Developer quickstart — clean checkout, build and first tests

## Scope

This is a **source/development** quickstart. It describes an isolated Linux checkout and its checks, **not** an installation or upgrade of a working VDR/yaVDR machine. The repository does not currently advertise a certified all-distribution dependency matrix.

- [User-oriented first steps](../getting-started.md)
- [Developer onboarding and architecture rules](developer-onboarding.md)
- [Make and test groups](../../mk/README.md)
- [Contributing](../../CONTRIBUTING.md)

## 1. Use a disposable Linux environment

The [GitHub CI fast regression](../../.github/workflows/ci.yml) currently uses an Ubuntu runner with C++ tools, SQLite, OpenSSL, curl development files and FFmpeg. In a **disposable development VM or container**, the corresponding package preparation is:

```bash
sudo apt-get update
sudo apt-get install -y build-essential make g++ sqlite3 libsqlite3-dev libcrypt-dev libcurl4-openssl-dev libssl-dev ffmpeg git python3
```

These are **CI-derived example dependencies**, not a promise that the entire suite or a real-VDR deployment has identical requirements on all Ubuntu/Debian releases. The commands modify the local development environment: **do not run them as a trial upgrade on your production VDR host**. Other Make targets and integrations may need additional tools.

## 2. Obtain and inspect the source

```bash
git clone https://github.com/hotzenplotz5/vdr-suite.git
cd vdr-suite
git status --short
git log -1 --format='%H %s'
```

Verify the repository remote, selected branch and current commit before investigating an issue or patch. Do not copy historic phase commit hashes from old notes as current `main`.

## 3. Run bounded, non-deploying checks

```bash
make test-docs
make -j2 --output-sync=target test-ci-fast
make daemon
```

These targets are backed by [CI](../../.github/workflows/ci.yml) and the [Make entrypoint](../../Makefile). They create local `.build/` artifacts and run local test fixtures; they do **not** deploy services or enroll VDR backends. A failing dependency or test should be diagnosed, not bypassed. For JavaScript/frontend tests use `make test-ci-frontend` after checking the relevant Node.js prerequisites in CI.

The intentionally real-system tests are separate: **do not invoke `make test-manual-real` against an unknown or production VDR**. The full `make test-all` includes a much broader local regression graph; begin with the focused target relevant to the files you plan to change.

## 4. Locate the code you need

| Task | Owner / first place to inspect |
| --- | --- |
| HTTP endpoints and public contracts | [Public API guide](../../api/README.md), `api/rest/`, [partial OpenAPI](../../api/openapi.json) |
| HTTP security and Device permissions | `core/security/`, [Current Architecture](current-architecture-state.md) |
| Daemon process composition | `apps/daemon/`, `core/daemon/` |
| VDR recording, playback, media tracks | `core/recordings/`, `core/media/` |
| Native VDR/Agent integrations | `core/vdr/`, `core/agent/` |
| Browser frontend | `web/frontend/` |
| Persistence | [Database boundaries](../../database/README.md), `core/sqlite/` |
| Test targets and build rules | [Make infrastructure](../../mk/README.md) |

This is a source-ownership map, not a claim that all potential plugins or clients in reserved directories are implemented.

## 5. A first contribution

Choose a small reproducible issue or documentation improvement. Read [CONTRIBUTING](../../CONTRIBUTING.md) for the contributor path and [AGENTS](../../AGENTS.md) for binding automation/repository-operation safety constraints. Make changes on a fresh branch, keep the diff focused, run the relevant tests, and open a PR reporting the exact head and checks actually completed.

Avoid deploying to a running yaVDR, modifying live databases/recordings, or treating earlier historical acceptance as proof of a new build. The [staged deployment contract](install-runtime-deployment-reliability.md) governs a separately approved installation.

## Back

- [Developer onboarding](developer-onboarding.md)
- [Development documentation index](index.md)
- [Documentation home](../index.md)
