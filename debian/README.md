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

## Source-package smoke test (unsigned, offline)

A separately gated helper prepares a Debian `3.0 (quilt)` source archive from the exact Git commit, with the upstream tarball containing no `debian/` directory:

```sh
python3 tools/build_vdr_suite_sourcepkg_isolated.py <VERIFIED_HEAD_SHA> --preflight-only
python3 tools/build_vdr_suite_sourcepkg_isolated.py <VERIFIED_HEAD_SHA>
```

This is a local, non-root source-package roundtrip. It **does not** sign packages, change the changelog distribution from `unstable`, upload to Launchpad, or validate the complete daemon/agent binary build. See [source package acceptance](../docs/planning/packaging-source-package-experiment.md).

The standalone SuiteBridge binary package **has now been built successfully** on the maintainer's yaVDR VDR ABI `vdr-abi-2.8.3-0yavdr`, with no daemon dependency, at candidate `ce63332dc72b0a262f1f30b410c18493668c2df1`. This is *not* official Ubuntu compatibility certification.

## Release block

**NOT RELEASE-READY.** Ubuntu noble against Seahawk VDR 2.8.3 now has a verified full three-binary build plus disposable package install/reinstall/remove/purge (GitHub Actions #29). Still open: source-release copyright/third-party review, other distribution ABI coverage, manual source-install migration, real service startup, historic-version DB upgrade/rollback, lintian and release signing. No package upload to Launchpad and no production yaVDR installation are authorized by this document.

In the experimental packages, automatic systemd enable/start is explicitly disabled. No maintainer script performs database migration.

## Verified CI full-build staging ownership (2026-10-14)

The first full Ubuntu 24.04 + Seahawk VDR 2.8.3 compilation reached `dh_missing --fail-missing` but failed because three staged assets had no package owner. The daemon now declares ownership for `/usr/bin/vdr-suite-media-calibrate` (a Python 3 command) and the inactive Nginx snippet `/etc/nginx/snippets/vdr-suite.conf`. The independent SuiteBridge binary package owns `/usr/lib/tmpfiles.d/vdr-suite-live.conf` because VDR runs as user `vdr` and requires private `/run/vdr` transport roots. The plugin-only profile explicitly stages that file without invoking the daemon build. No Nginx site is activated or VDR service started by this change.

The old `dh_missing` failure was recorded in [GitHub Actions run #26](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38584786074). A successful replacement full build occurred in [GitHub Actions #27](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38586364547) and was repeated with 50 passing Packaging tests in [#29](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38588255607). This validates the experimental noble binary-build gate but does not complete P5 legal/source-release acceptance. Whether manual existing Nginx snippets collide with dpkg conffiles, and install-time tmpfiles execution under target VDR, remain P6 gates. Neither a package install nor an upgrade is authorized on production yaVDR.

## Experimental P7 versioned upgrade job

The branch now builds `0.14.1-1~exp2` separately from pinned earlier commit `bc4623374a4df4c08118c424ea19aaf552a4964c` (`0.14.1-1~exp1`). GitHub Actions constructs both **unsigned** package sets in disposable runners, tests a real dpkg upgrade and later an explicit binary downgrade after restoring synthetic offline test data. This is not an on-host install procedure or evidence of supported runtime migration. See [`P7 upgrade/recovery`](../docs/planning/packaging-p7-upgrade-recovery.md).
