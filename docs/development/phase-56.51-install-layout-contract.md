# Phase 56.51 - Install Layout Contract

## Navigation

- [Development Index](index.md)
- [Phase 56.50 - Remove Transitional ACTIONS_SRC Aggregate](phase-56.50-remove-actions-src-aggregate.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Planned and documented for Phase 56 install-layout work.

---

## Purpose

Phase 56.51 defines the install layout contract for later packaging work.

This phase does not introduce Debian packaging yet. It only defines the filesystem layout and Make variables that the future make install staging target must follow.

The contract keeps runtime files, CLI tools, configuration, documentation and manpages separate so the later Debian package split can be implemented without changing the layout again.

---

## Install Invocation Contract

The canonical staging invocation is:

```bash
make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
```

The install target must never require direct installation into the live host filesystem during tests.

All install verification must use DESTDIR.

---

## Required Make Variables

The install logic must support the following variables:

```text
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

Package-style builds may override PREFIX=/usr.

---

## Runtime Package Layout

The future vdr-suite runtime package owns the daemon and system configuration:

```text
/usr/sbin/vdr-suite-daemon
/etc/vdr-suite/vdr-suite.conf
/usr/share/man/man8/vdr-suite-daemon.8
/usr/share/man/man5/vdr-suite.conf.5
```

The daemon binary belongs in SBINDIR because it is a service/runtime program, not an interactive user command.

---

## CLI Package Layout

The future vdr-suite-cli package owns the user-facing dashboard command:

```text
/usr/bin/vdr-suite-dashboard
/usr/share/man/man1/vdr-suite-dashboard.1
```

The dashboard command belongs in BINDIR.

---

## Documentation Package Layout

The future vdr-suite-doc package owns project documentation, examples and architecture notes:

```text
/usr/share/doc/vdr-suite/
```

Documentation installation must not be required for runtime operation.

---

## Optional Tools Layout

Smoke, regression and diagnostic helper binaries must not be installed by default into the runtime package.

If installed later, they must be owned by an optional package such as:

```text
vdr-suite-tools
vdr-suite-tests
```

The preferred manpage strategy for these helpers is one aggregate manual page:

```text
/usr/share/man/man1/vdr-suite-smoke-tools.1
```

---

## Non-Goals

Phase 56.51 does not add:

```text
debian/
dpkg-buildpackage integration
systemd unit installation
maintainer scripts
postinst or prerm logic
final package dependency metadata
public C++ development headers
vdr-suite-dev package
```

---

## Follow-Up

The next implementation step is:

```text
Phase 56.52 - make install Staging Target
```

That phase should implement a staging-only install target that follows this contract.

---

## Back

- [Back to Development Index](index.md)
