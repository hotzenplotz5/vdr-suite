# Phase 56.52 - make install Staging Target

## Navigation

- [Development Index](index.md)
- [Phase 56.51 - Install Layout Contract](phase-56.51-install-layout-contract.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Implemented on branch `phase-56-make-install-staging-target`.

---

## Purpose

Phase 56.52 adds the first staging-only install target for vdr-suite.

The goal is not Debian packaging yet. The goal is to verify that the repository can create a package-root style filesystem tree under `DESTDIR` without installing directly into the live host filesystem.

---

## Install Makefile

A new include file was added:

```text
mk/install.mk
```

The main `Makefile` now includes it after the daemon source map and before local test groups.

The install logic defines the layout variables from Phase 56.51:

```text
PREFIX
BINDIR
SBINDIR
SYSCONFDIR
DATADIR
DOCDIR
MANDIR
INSTALL
```

---

## Install Targets

The following targets were added:

```text
install
install-runtime
install-cli
install-docs
test-install-staging
```

`install` depends on:

```text
install-runtime
install-cli
install-docs
```

---

## Runtime Install

`install-runtime` builds the daemon and installs it into the staged service binary directory:

```text
$(DESTDIR)$(SBINDIR)/vdr-suite-daemon
```

With package-style overrides, this becomes:

```text
/tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-daemon
```

The runtime target also creates the staged configuration directory:

```text
$(DESTDIR)$(SYSCONFDIR)/vdr-suite
```

With package-style overrides, this becomes:

```text
/tmp/vdr-suite-pkgroot/etc/vdr-suite
```

---

## CLI Install

`install-cli` builds the dashboard CLI and installs it into the staged user command directory:

```text
$(DESTDIR)$(BINDIR)/vdr-suite-dashboard
```

With package-style overrides, this becomes:

```text
/tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-dashboard
```

---

## Documentation Install

`install-docs` creates the staged documentation and data directories:

```text
$(DESTDIR)$(DOCDIR)
$(DESTDIR)$(DATADIR)
```

For the initial staging target, it installs `README.md` as the first documentation payload:

```text
$(DESTDIR)$(DOCDIR)/README.md
```

---

## Staging Test

The staging verification target is:

```bash
make test-install-staging
```

It removes `/tmp/vdr-suite-pkgroot`, invokes:

```bash
make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
```

and verifies the staged output paths with `test` checks.

Expected staged paths:

```text
/tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-daemon
/tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-dashboard
/tmp/vdr-suite-pkgroot/etc/vdr-suite
/tmp/vdr-suite-pkgroot/usr/share/doc/vdr-suite/README.md
/tmp/vdr-suite-pkgroot/usr/share/vdr-suite
```

---

## Verification

The following commands were run locally before committing:

```bash
make daemon
make dashboard-cli
make test-install-staging
find /tmp/vdr-suite-pkgroot -type f -o -type d | sort
```

The following documentation and phase checks were also run:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Non-Goals

Phase 56.52 does not add:

```text
Debian package metadata
systemd units
maintainer scripts
manpage installation
sample configuration payload
runtime config parser changes
live host installation tests
```

---

## Follow-Up

Next Phase 56 packaging steps:

```text
Phase 56.53 - Manpage Skeletons
Phase 56.54 - Install Manifest / Package File Contract
Phase 56.55 - Package Prerequisite Audit
```

---

## Back

- [Back to Development Index](index.md)
