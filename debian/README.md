# Experimental Debian source packaging

This root `debian/` directory is the canonical source-package location for `dpkg-buildpackage`. The historical `packaging/debian/` directory is documentation, not an alternative `debian/` source tree.

## Independent VDR plugin build

SuiteBridge can be built **without building the daemon or Backend Agent**:

```sh
make -C vdr-plugin-suite-bridge
```

For an experimental Debian/Ubuntu plugin-only binary package, from a clean test checkout with `vdr-dev` and other build prerequisites installed:

```sh
dpkg-buildpackage -B -Pplugin-only -us -uc
```

The `plugin-only` profile removes both server binary packages from `debian/control` and bypasses the server and agent build/install rules. It does **not** require or install the VDR-Suite daemon binary package. The SuiteBridge source still references shared internal headers in the same repository, so the entire source checkout is needed.

Without the profile, the source package attempts to build daemon, Backend Agent and SuiteBridge as separate **installable binary packages**. The plugin must carry the ABI-specific `${vdr:Depends}`, populated by the VDR debhelper sequence. The exact VDR `vdr.pc`, `libdir` and ABI must match each target Debian/Ubuntu series.

## Git and safe staging

`debian/rules` is executable (Git mode `100755` required by dpkg). Source metadata (`debian/control`, `rules`, `changelog`, `*.install`) remains tracked. Generated `.deb`, `.dsc`, `.changes`, buildinfo, `debian/tmp`, debhelper output and `libvdr-suitebridge.so.*` are ignored. Already tracked outputs are **not** automatically removed; check read-only with:

```sh
python3 tools/check_packaging_tracked_artifacts.py
python3 -m unittest discover -s tools -p 'test_packaging_*.py'
```

[Standalone plugin instructions](../vdr-plugin-suite-bridge/BUILDING.md) and [experimental Ubuntu 24.04 CI workflow](../.github/workflows/packaging-plugin-experimental.yml).

## Release block

**NOT RELEASE-READY.** A clean root build and the exact Ubuntu CI result still require verification. Full daemon dependency metadata, distribution/ABI compatibility, accurate Debian copyright/third-party notices, package file ownership, conffiles, install/upgrade/remove/purge and database backup/rollback tests remain open. No package upload to Launchpad and no production yaVDR installation are authorized by this document.

In the experimental packages, automatic systemd enable/start is explicitly disabled. No maintainer script performs database migration.
