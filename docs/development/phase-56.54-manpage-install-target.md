# Phase 56.54 - Manpage Install Target

## Navigation

- [Development Index](index.md)
- [Phase 56.53 - Manpage Skeletons](phase-56.53-manpage-skeletons.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Implemented for Phase 56 packaging-readiness work.

---

## Purpose

Phase 56.54 extends the install staging target so the manual pages added in Phase 56.53 are installed into the staged package root.

The target still does not create Debian packages and does not install anything into the live host filesystem.

---

## Install Target Change

`mk/install.mk` now includes an `install-manpages` target and adds it to the default `install` dependency chain.

The target stages these files:

- `docs/man/man8/vdr-suite-daemon.8`
- `docs/man/man5/vdr-suite.conf.5`
- `docs/man/man1/vdr-suite-dashboard.1`

---

## Staged Paths

With the canonical staging command:

```bash
make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
```

the expected manpage paths are:

- `/tmp/vdr-suite-pkgroot/usr/share/man/man8/vdr-suite-daemon.8`
- `/tmp/vdr-suite-pkgroot/usr/share/man/man5/vdr-suite.conf.5`
- `/tmp/vdr-suite-pkgroot/usr/share/man/man1/vdr-suite-dashboard.1`

---

## Verification

`make test-install-staging` verifies the daemon, dashboard command, documentation directory, data directory and all three staged manpages.

The documentation and phase checks remain:

- `make test-docs`
- `make test-phase`
- `make test-phase-map-coverage`

---

## Non-Goals

Phase 56.54 does not add compressed manpages, Debian packaging metadata, maintainer scripts, systemd units or final package dependency metadata.

---

## Follow-Up

Next Phase 56 packaging steps:

- Phase 56.55 - Install Manifest / Package File Contract
- Phase 56.56 - Package Prerequisite Audit
- Phase 56.57 - Phase 56 Completion Audit

---

## Back

- [Back to Development Index](index.md)
