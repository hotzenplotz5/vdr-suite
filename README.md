# VDR-Suite

[![VDR-Suite CI](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml)

## Start Here

Current state:

- [Current State](docs/CURRENT.md)

Project overview:

- [Projektübersicht Deutsch](docs/project-overview.de.md)
- [Project Overview English](docs/project-overview.en.md)
- [Documentation Index](docs/index.md)

---

## What is VDR-Suite?

VDR-Suite is a modern multi-backend platform for VDR.

It provides a snapshot-based runtime, backend-neutral services, REST APIs, metadata foundations, search foundations and future automation features for VDR environments.

VDR remains the source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Current Verified State

Current completed project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Phase 56 completed:

- library and module boundary documentation
- build source-group cleanup
- staged install target using `DESTDIR`
- daemon and CLI install paths
- initial manpages
- manpage staging
- install manifest and package file contract
- package prerequisite audit

Current staging command:

```bash
make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
```

Current verification command:

```bash
make test-install-staging
```

Full current details are kept in [docs/CURRENT.md](docs/CURRENT.md).

---

## Current Runtime Entry Points

```text
/usr/sbin/vdr-suite-daemon
/usr/bin/vdr-suite-dashboard
```

`vdr-suite-dashboard` is currently the CLI dashboard command. The future web/mobile frontend is a later product layer.

---

## Current Architecture Boundary

The REST API is the application-facing API boundary.

Core modules remain internal implementation boundaries.

```text
Core modules may not depend on api/rest.
The REST API layer may depend on core modules.
```

---

## Documentation

Use these entry points instead of starting from historical phase notes:

- [Current State](docs/CURRENT.md)
- [Documentation Index](docs/index.md)
- [Development Documentation](docs/development/index.md)
- [ADR Index](docs/adr/index.md)
- [Roadmap](docs/planning/roadmap.md)

Historical implementation notes remain in `docs/development/phase-*`.

---

## Next Work

Next work block:

```text
DOC-RESET-1 - Documentation Usability Reset
```

After that:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```
