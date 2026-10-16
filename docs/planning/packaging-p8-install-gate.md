# P8 — fail-closed installation entry point (unreleased)

Status: **pre-install gate implemented; no host installation authorized**.

## Source of truth

The only new P8 command entry point is
`tools/packaging_p8_installer.py`. It imports and runs
`tools/packaging_host_collision_preflight.py` directly; it does not copy the
P6 destination/ownership/nginx scanning logic or create a second package
builder. Existing package manifests in `debian/*.install`,
`debian/rules` and staging in `mk/install.mk` remain authoritative.

For a **read-only** inspection of the actual host, from the verified source
checkout as an ordinary user (no sudo):

```sh
python3 tools/packaging_p8_installer.py --check
```

For offline unit tests, `--root` points to an isolated fixture. This is
not a way to bypass the installation gate.

## Enforcement boundary

`--install` always evaluates the P6 inspection first and **always refuses**
the operation, including when the scanner returns `NO_KNOWN_CONFLICT`.
The JSON outcome contains `decision: BLOCKED`,
`install_approved: false`, the actual P6 verdict and public finding codes,
but no config contents or secrets. The command returns status 3. A scanner
error returns status 2 and performs no writes. It never invokes `apt`,
`dpkg`, `make install`, a service manager, nginx or a runtime deploy.
`--check` preserves the existing P6 diagnostic exit semantics: 2 for
`BLOCKED`, 0 for `REVIEW_REQUIRED` or `NO_KNOWN_CONFLICT`.
Exit status 0 from **inspection** is never installation approval.

The planned `--build` and `--uninstall` options are reserved and also
refuse execution (status 3); the existing independent package build/staging
flows are unchanged. Do not add a competing package builder.

**This is an entry-point guard, not a system-wide installation lock.**
Direct `apt install` / `dpkg -i` can still bypass it. No package
`preinst` or other effective pre-unpack gate exists yet. An approved
implementation must cover that bypass **before** anyone is told the packages
are safe for deployment over an old manual installation. The existing P6
scanner also skips directory/wildcard manifest entries and is not a full
nginx parser; neither its observations nor its clean result prove complete
ownership coverage.

## Required next P8 work

1. Establish an explicit per-path takeover policy, package-specific
   ownership/upgrade policy and VDR ABI/service compatibility matrix.
2. Ensure the real install dispatcher **and the direct dpkg unpack route**
   enforce protection before package-file replacement. Do not pretend a
   `postinst` check protects files during unpack. Avoid implementing two
   divergent scanners merely to work around package lifecycle order.
3. Determine the actual active systemd daemon binary, nginx route/include
   topology and older source/database provenance. Do not replace operator
   managed `/etc/nginx`, `/etc/default`, `/etc/vdr-suite` or `/usr/local`
   on the strength of a successful scanner run.
4. Preserve the P7 no-live-migration requirement and obtain separate
   authorization for service/DB operations. Only then implement guarded
   package installation and lifecycle acceptance.

Regression:
`python3 -m unittest discover -s tools -p 'test_packaging_p8_installer.py'`
uses private temporary directory trees only. The existing P6 scanner
regressions remain required. This is not productive installer acceptance.
