# Packaging P0.2 — File ownership and safety gate

Status: SOURCE IMPLEMENTED, ISOLATED TESTS ADDED; clean Debian/Ubuntu staging and runtime acceptance still pending.

## Changes

The `tools/packaging_inventory.py` audit now emits schema 2. Along with ELF DT_NEEDED libraries, file hashes, package owner and symlink metadata it records POSIX permission bits. It only accepts staged package paths within the known `/usr`, `/lib/systemd/system`, `/etc/vdr-suite`, `/etc/default/vdr-suite-*`, `/var/lib/vdr-suite`, and `/var/cache/vdr-suite` namespaces. Unknown destinations remain errors.

Agent systemd service and `backend-agent.conf` map to `vdr-suite-backend-agent` (the older generic rule would have misattributed the unit). Configuration files are marked as `config_candidate`: this is not yet a definitive Debian conffile list.

Safety checks reject world-writable regular files, shipped private `/var/lib/vdr-suite/secrets/*` and `/var/lib/vdr-suite/backend-agent/*` state and symlinks resolving outside approved `/usr`, `/lib` and `/etc` roots. No target path is written by this audit except an explicitly selected JSON report.

Tests: `python3 -m unittest discover -s tools -p test_packaging_inventory.py`.

## Open findings

- Directory mode/ownership and `tmpfiles.d` lifecycle need dedicated checks before final `debian/*.install`.
- `config_candidate` paths need explicit dpkg conffile treatment, including handling old source installations and avoiding secret leaks.
- Symlinks within permitted namespaces may still be dangling or cross-package references; verify against both staged manifests.
- World-readable sensitive conffiles, special modes and file owners need further validation.
- No complete clean `DESTDIR` manifest, VDR-ABI compatibility build, `dpkg-shlibdeps` mapping or Ubuntu PPA build has yet been run.
- Runtime/agent component split is provisional pending clean install and real runtime verification.

## Next

Finish clean-root tests and stricter manifest acceptance, then start P2 canonical versioning. A Launchpad test PPA remains deferred until signed source package builds succeed locally.
