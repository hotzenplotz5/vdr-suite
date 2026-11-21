# Phase 56.55 - Install Manifest / Package File Contract

## Navigation

- [Development Index](index.md)
- [Phase 56.54 - Manpage Install Target](phase-56.54-manpage-install-target.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Implemented for Phase 56 packaging-readiness work.

---

## Purpose

Phase 56.55 defines the install manifest and package file ownership contract for the current staged install layout.

This is still a packaging-readiness contract. It does not introduce Debian packaging metadata or maintainer scripts.

---

## Canonical Staging Command

The canonical staging command remains:

    make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr

The staged root must be disposable and must not modify the live host filesystem.

---

## Package Candidate: vdr-suite

Runtime package candidate:

    /usr/sbin/vdr-suite-daemon
    /etc/vdr-suite/
    /usr/share/man/man8/vdr-suite-daemon.8
    /usr/share/man/man5/vdr-suite.conf.5

Purpose:

- daemon runtime entry point
- system configuration directory
- daemon manual page
- configuration manual page

---

## Package Candidate: vdr-suite-cli

CLI package candidate:

    /usr/bin/vdr-suite-dashboard
    /usr/share/man/man1/vdr-suite-dashboard.1

Purpose:

- command-line dashboard entry point
- user command manual page

---

## Package Candidate: vdr-suite-doc

Documentation package candidate:

    /usr/share/doc/vdr-suite/README.md
    /usr/share/vdr-suite/

Purpose:

- installed project documentation entry point
- future package data and example location

---

## Optional Future Package: vdr-suite-tools or vdr-suite-tests

Optional later package candidate:

    smoke and regression helper programs
    smoke tool manual pages or one grouped smoke-tools manual page

This package is not part of the current install staging contract.

---

## Explicit Non-Goals

Phase 56.55 does not add:

- Debian package metadata
- debian/control
- maintainer scripts
- systemd units
- package dependency resolution
- compressed manpages
- public C++ ABI promises
- vdr-suite-dev

---

## Current Staging Verification

`make test-install-staging` currently verifies:

- daemon binary exists and is executable
- dashboard command exists and is executable
- configuration directory exists
- README is installed into the documentation directory
- data directory exists
- all three manual pages are staged under `$(MANDIR)`

---

## Follow-Up

Next Phase 56 steps:

- Phase 56.56 - Package Prerequisite Audit
- Phase 56.57 - Phase 56 Completion Audit

After Phase 56 is complete, the documentation structure should be simplified so current state and historical phase records are separated more clearly.

---

## Back

- [Back to Development Index](index.md)
