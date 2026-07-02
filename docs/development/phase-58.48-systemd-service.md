# Phase 58.48 - systemd Service Installation

## Navigation

- [Development Index](index.md)
- [Current Project Status](current-status.md)

---

## Status

Implemented during Phase 58 frontend/live-parity work.

---

## Purpose

Phase 58.48 adds a first systemd unit for the installed `vdr-suite-daemon`.

The immediate operational goal is to stop relying on `/tmp/vdr-suite-daemon` and manual `nohup` sessions during real yaVDR testing. The daemon is installed to `/usr/sbin/vdr-suite-daemon`, the frontend is installed to `/usr/share/vdr-suite/web/frontend`, and systemd can now manage the runtime process after host reboot.

---

## Installed Unit

Source unit:

```text
packaging/systemd/vdr-suite-daemon.service
```

Installed unit path with package-style install:

```text
/lib/systemd/system/vdr-suite-daemon.service
```

The unit starts:

```text
/usr/sbin/vdr-suite-daemon
```

and exports:

```text
VDR_SUITE_FRONTEND_ROOT=/usr/share/vdr-suite/web/frontend
```

---

## Runtime Ordering

The unit declares:

```text
After=network-online.target vdr.service
Wants=network-online.target
Requires=vdr.service
```

This keeps the daemon ordered after the local VDR service and network-online target for the current yaVDR deployment model.

---

## Restart Policy

The unit uses:

```text
Restart=on-failure
RestartSec=3
KillSignal=SIGTERM
TimeoutStopSec=15
```

This matches the existing daemon SIGTERM shutdown behaviour and avoids the previous manual `kill -9` workflow.

---

## Install Integration

`mk/install.mk` now includes:

```text
SYSTEMDUNITDIR ?= /lib/systemd/system
install-systemd
```

`make install` depends on `install-systemd`, but it does not enable or start the service. Activation remains an explicit host-admin step:

```bash
systemctl daemon-reload
systemctl enable --now vdr-suite-daemon.service
```

---

## Staging Verification

`make test-install-staging` verifies the staged unit exists:

```text
/tmp/vdr-suite-pkgroot/lib/systemd/system/vdr-suite-daemon.service
```

The test remains DESTDIR-only and does not mutate the live host system.

---

## Non-Goals

Phase 58.48 does not add Debian maintainer scripts, automatic service enabling, package dependencies, a dedicated runtime user, or final hardening rules.

Those belong to later packaging/hardening work.
