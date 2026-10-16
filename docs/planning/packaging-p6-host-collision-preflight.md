# P6/P8 — existing installation collision preflight

Status: read-only preflight and isolated tests **VERIFIED** in [CI #94](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38874068291), all five jobs SUCCESS, exact code SHA `baad4705f9a7db37b4c207ae0d0a5319d80d2d41`; **not an authorized host installer**.

The canonical three Debian binary packages provide a VDR-Suite daemon with
web assets, an independently buildable SuiteBridge plugin and a backend agent.
Before using them on an existing yaVDR system, run
`tools/packaging_host_collision_preflight.py` as an ordinary user with no
`sudo`. It prints JSON; it never installs, creates files, changes permissions,
backs up databases, invokes nginx, or starts/stops/reloads services.

The tool derives exact package target filenames from the root `debian/*.install`
manifests and refuses unknown changes to critical target paths. It reports
hard blockers for unowned package destination files, destinations owned by a
different package, unreadable dpkg ownership and special/symlink targets. A
same-package target is *review required*, not an authorization to upgrade.
It separately notes older `/usr/local` daemon binaries, the older frontend
tree, VDR-Suite persistent state, systemd overrides and the current ABI-specific
SuiteBridge .so candidate files.

Nginx deserves special treatment: the daemon package currently owns
`/etc/nginx/snippets/vdr-suite.conf`; the snippet is **not activated
automatically**, but a manual file already present at that exact path can
collide with dpkg. The scanner looks for obvious active
`/vdr-suite` location/include declarations in limited nginx configuration
files. Even if none are seen, nginx configuration remains REVIEW_REQUIRED:
this is not a full nginx include/config parser, and the tool does not run
`nginx -T` or reload the service. It never prints secrets or file contents.

Results:
- `BLOCKED`: a detected collision or unable-to-establish ownership; stop.
- `REVIEW_REQUIRED`: potential overlap or existing runtime state; require
  inspection and explicit operator-approved migration plan.
- `NO_KNOWN_CONFLICT`: scanner found none of its known conditions; this is
  **not** proof of clean installation or of safe service startup.

All results have `install_approved: false`. They neither test actual old
manual-install databases nor prove daemon startup, schema compatibility,
recovery, startup order, active site configuration or VDR ABI compatibility.
Keep daemon and VDR service starts disabled during experimental packaging;
never change an existing nginx site or `/usr/local` path as an automatic fix.

For offline development, `--root` accepts a fake directory tree. For the
live yaVDR host, default `--root /` is **read-only**, should be invoked from
the verified separate packaging checkout and needs no administrator rights.

Focused regression: `python3 -m unittest discover -s tools -p
'test_packaging_host_collision_preflight.py'`.

Next: add approved migration policies for each observed collision, enforce
the read-only gate before any P8 installer writes, and check the actual
`nginx -t` / VDR services only under separate live acceptance authorization.

Release boundary: CI validates the read-only scan and disposable package
build/upgrade tests, **not** a real yaVDR installation. The current Debian
daemon package still supplies `/etc/nginx/snippets/vdr-suite.conf`; a
pre-existing unowned file at that path is a deliberate `BLOCKED` result.
The new P8 entry point runs this scan before its `--install` operation,
but **always refuses** installation until approved transition policy exists.
Direct `apt`/`dpkg` installation remains unguarded; do not bypass the P8
entry-point gate. See [the P8 installation gate](packaging-p8-install-gate.md). Existing active nginx includes and older
manual binaries remain operator-managed until an approved transition exists.
