# Packaging P2–P7 implementation ledger

Status: **in development**, not released. Source-level implementation is distinct from tested packaging.

| Slice | Added now | Remaining release gate |
| --- | --- | --- |
| P2 | Root `VERSION` and consistency audit for plugin; deterministic version inputs planned | Integrate version into compiler flags, runtime reporting, Debian changelog, source archives and version tests |
| P3 | Existing `DESTDIR` install stays canonical | Separate daemon/agent payload rules, build from clean source tarball, build in sandbox |
| P4 | Initial Debian dependency declarations and ELF manifest audit | Verify actual build headers/libs and strict per-distribution VDR ABI |
| P5 | Experimental `packaging/debian` control and source format | Working `debian/rules`, `changelog`, copyright/licensing, manifests and `dpkg-buildpackage` |
| P6 | Existing systemd units identified; ownership-audit guard | Implement/test precise `debhelper` service lifecycle, conffiles, tmpfiles, remove vs purge and migration from manual installs |
| P7 | Safe upgrade policy documented | Backup/migration and downgrade guard behavior, real upgrade matrix, rollback tests |

## Essential release policy

- No package maintainer script may silently delete `/var/lib/vdr-suite`, secrets, recordings or a user-managed configuration.
- `remove` keeps state and configuration. A `purge` must not delete valuable user data by default without explicit opt-in/data export guidance.
- DB rollback means restoring a validated pre-upgrade backup, not running an undocumented reverse migration.
- No database mutation in `postinst` until a tested reversible migration sequence and explicit transactional safety are available.
- Do not turn on or restart VDR automatically on the production yaVDR target during experimentation.
- Existing manually installed files under `/usr/local` must be detected before package deployment; dpkg ownership does not protect these paths.
- Ubuntu Launchpad PPA only after source package and isolated installation/upgrade tests pass.

## Candidate release acceptance

1. Build and test in fresh Debian 13 and Ubuntu 24.04/26.04 containers/VMs as available.
2. `dpkg-buildpackage -us -uc` succeeds from a pristine source package.
3. `lintian`, shared library, and VDR ABI dependencies verified.
4. Install, reinstall, remove, purge, and upgrade preserve config and database state as specified.
5. Real yaVDR smoke test only under separate deployment approval.
