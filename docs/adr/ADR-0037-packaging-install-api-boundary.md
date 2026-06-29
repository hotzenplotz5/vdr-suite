# ADR-0037: Packaging, Install Layout and API Boundary

## Status

Accepted for Phase 56 planning.

This ADR defines the packaging, installation and API boundary assumptions that Phase 56 must preserve before concrete `make install` targets and later Debian packaging are added.

---

## Context

Phase 56 prepares VDR Suite for a future packageable layout without introducing real Debian packaging yet.

The repository currently builds runtime programs, smoke helpers and tests directly from source groups. Several source groups are still being split so that runtime, API, core and test-only dependencies are no longer pulled through broad transitional aggregates.

At the same time the project needs a stable installation contract for later packages:

- which programs are installed
- which configuration files exist
- which manual pages are required
- which documentation is installed
- which tools stay optional
- whether internal C++ modules are installed as public libraries
- how `api/rest` differs from core/library code
- how `make install` must behave with `DESTDIR` and `PREFIX`

---

## Decision

Phase 56 treats C++ modules as internal build and architecture boundaries, not as public ABI-stable libraries.

No `vdr-suite-dev` package is promised in Phase 56.

No public C++ headers are installed as a developer contract in Phase 56.

The REST API layer is an application-facing HTTP boundary. It may depend on core modules, but core modules must not depend on `api/rest`.

The first packageable layout will install runtime programs, configuration examples, documentation and manpages, but not public C++ libraries.

---

## Core vs API Boundary

Core modules define what the system can do.

The REST API layer defines how external clients call those capabilities over HTTP.

Core modules include:

- `core/sqlite`
- `core/recordings`
- `core/vdr`
- `core/http`
- `core/runtime`
- `core/daemon`

The REST API module is:

- `api/rest`

Boundary rule:

```text
Core modules may not depend on api/rest.
The REST API layer may depend on core modules.
```

Practical meaning:

- Services, repositories, backend adapters, policies and domain objects live in core modules.
- Controllers, request parsers, route wiring and HTTP-facing response behavior live in `api/rest`.
- `api/rest` exposes core behavior; it does not own the business logic.

---

## Internal Library Candidates

The following names describe internal architecture boundaries only. They are not public package names in Phase 56.

```text
libvdr-suite-sqlite
libvdr-suite-recordings
libvdr-suite-recording-actions
libvdr-suite-vdr
libvdr-suite-http
libvdr-suite-runtime
libvdr-suite-rest-api
```

These names may help future static-library grouping or package-internal build organization, but they do not imply stable ABI, public headers or a `-dev` package.

---

## Package Layout Contract

The first packageable layout is split conceptually into runtime, CLI, documentation and optional tools.

### vdr-suite

```text
/usr/sbin/vdr-suite-daemon
/etc/vdr-suite/vdr-suite.conf
/usr/share/man/man8/vdr-suite-daemon.8.gz
/usr/share/man/man5/vdr-suite.conf.5.gz
```

Purpose:

- daemon runtime
- system configuration
- administrator manual page
- configuration manual page

### vdr-suite-cli

```text
/usr/bin/vdr-suite-dashboard
/usr/share/man/man1/vdr-suite-dashboard.1.gz
```

Purpose:

- command-line dashboard or administrative CLI
- user/admin command manual page

### vdr-suite-doc

```text
/usr/share/doc/vdr-suite/
/usr/share/doc/vdr-suite/api/
/usr/share/doc/vdr-suite/examples/
/usr/share/doc/vdr-suite/architecture/
```

Purpose:

- API documentation
- examples
- architecture and development notes relevant for operators and packagers

This package does not require a command manpage.

### vdr-suite-tools or vdr-suite-tests

Optional future package:

```text
Smoke tools
Regression tools
Real-VDR test helpers
```

If these tools are installed, they need either:

```text
/usr/share/man/man1/vdr-suite-smoke-tools.1.gz
```

or separate tool-specific manpages.

Phase 56 does not require installing these tools by default.

---

## Manpage Contract

Required manpages for the minimal installable layout:

```text
vdr-suite-daemon.8
vdr-suite.conf.5
vdr-suite-dashboard.1
```

Optional future manpages:

```text
vdr-suite-api.7
vdr-suite-smoke-tools.1
```

Repository storage convention:

```text
docs/man/man8/vdr-suite-daemon.8
docs/man/man5/vdr-suite.conf.5
docs/man/man1/vdr-suite-dashboard.1
```

Packages may compress installed manpages as `.gz` during packaging or install staging.

---

## make install Contract

Concrete install targets must support staged installation.

Required invocation shape:

```bash
make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
```

The install target must write under `$(DESTDIR)` and must not write directly to live system paths during package staging.

Required install variables:

```make
DESTDIR
PREFIX
BINDIR
SBINDIR
SYSCONFDIR
DATADIR
DOCDIR
MANDIR
```

Recommended defaults:

```make
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SBINDIR ?= $(PREFIX)/sbin
SYSCONFDIR ?= /etc
DATADIR ?= $(PREFIX)/share/vdr-suite
DOCDIR ?= $(PREFIX)/share/doc/vdr-suite
MANDIR ?= $(PREFIX)/share/man
```

Package build example:

```bash
make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
```

Expected staged paths:

```text
/tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-daemon
/tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-dashboard
/tmp/vdr-suite-pkgroot/etc/vdr-suite/vdr-suite.conf
/tmp/vdr-suite-pkgroot/usr/share/doc/vdr-suite/
/tmp/vdr-suite-pkgroot/usr/share/man/man8/vdr-suite-daemon.8
/tmp/vdr-suite-pkgroot/usr/share/man/man5/vdr-suite.conf.5
/tmp/vdr-suite-pkgroot/usr/share/man/man1/vdr-suite-dashboard.1
```

---

## Non-Goals

Phase 56 does not add:

- `debian/`
- `dpkg-buildpackage`
- maintainer scripts such as `postinst`, `prerm` or `postrm`
- final package dependency metadata
- a public C++ ABI promise
- a `vdr-suite-dev` package
- automatic systemd activation
- direct installation into the live host without `DESTDIR` testing

---

## Consequences

This ADR keeps packaging preparation safe and reversible.

It allows Phase 56 to add install layout documentation, manpage source files and staged install targets without prematurely freezing a public C++ library interface.

It also gives the source-group cleanup a concrete reason: runtime targets, REST API targets, core modules and test-only helpers must become separable enough that a future package can install only the intended artifacts.

---

## Follow-Up Work

Planned Phase 56 follow-up items:

- finish removal of transitional `ACTIONS_SRC` users
- add install layout contract documentation
- add manpage source files
- add `make install` staging contract
- add package prerequisite audit
- complete Phase 56 packaging readiness audit
