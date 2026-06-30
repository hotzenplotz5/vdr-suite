# Phase 56.53 - Manpage Skeletons

## Navigation

- [Development Index](index.md)
- [Phase 56.52 - make install Staging Target](phase-56.52-make-install-staging-target.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Implemented for Phase 56 packaging-readiness work.

---

## Purpose

Phase 56.53 adds the initial manual page skeletons required by the install and packaging boundary.

This phase only adds the manpage source files to the repository. It does not install them yet.

---

## Added Manpages

The following files were added:

- `docs/man/man8/vdr-suite-daemon.8`
- `docs/man/man5/vdr-suite.conf.5`
- `docs/man/man1/vdr-suite-dashboard.1`

---

## Manual Sections

The manual sections follow the package role split:

- man8: daemon and system administration
- man5: configuration file format
- man1: user command

---

## Package Mapping

The intended later package ownership is:

- `vdr-suite`: `vdr-suite-daemon.8` and `vdr-suite.conf.5`
- `vdr-suite-cli`: `vdr-suite-dashboard.1`

---

## Non-Goals

Phase 56.53 does not add manpage installation, compressed manpages, Debian packaging metadata, systemd units or a final configuration schema.

---

## Follow-Up

The next implementation step is Phase 56.54 - Manpage Install Target.

That phase should extend `mk/install.mk` and `test-install-staging` so the manpages are staged under `$(MANDIR)`.

---

## Back

- [Back to Development Index](index.md)
