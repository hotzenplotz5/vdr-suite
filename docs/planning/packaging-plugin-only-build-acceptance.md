# P5 experimental SuiteBridge-only Debian package build

Status: **build gate implemented, real host build not yet executed**. This replaces ad-hoc package testing on the productive yaVDR with a checked-in strict helper.

## Verified clean baseline

The local `/home/yavdr/vdr-suite-packaging-test` checkout previously passed **13 packaging unit tests** under each of `umask 0002` and `0022`, version contract `0.14.1`, and read-only tracked-artifact audit, all at commit `6e0776bb1114b365c328085843db4b439b7d9bc9`. This is not a binary package build pass.

## Build safety

The helper `tools/build_suitebridge_deb_isolated.sh` checks unprivileged user `yavdr`, absolute checkout root, branch, clean status, exact expected SHA, remote branch, all required commands, target VDR `pkg-config` variables and plugin-only build dependencies. A missing tool/dependency is a **stop**, not permission to `sudo apt install` dependencies on the live server.

After a 2 GiB conservative scratch budget and a 4 GiB/15% filesystem reserve check, it creates a fresh private directory under `/home/yavdr/vdr-suite-pkgbuild-XXXXXXXX`, clones the current already-verified source locally and runs `dpkg-buildpackage -B -Pplugin-only -us -uc` at bounded `parallel=2`. The original packaging checkout and Codec checkout are not built in or modified.

On success it requires exactly one `vdr-plugin-suitebridge` binary package, no daemon/agent binary packages, no runtime server dependency and a VDR ABI dependency. It lists the package payload and checks post-build free space. On failure it leaves an isolated log/working directory for diagnosis; no destructive cleanup is automated.

## Execution (only on the separately tested user checkout)

First fast-forward the packaging checkout to an **individually verified current remote SHA**, confirm an empty `git status --porcelain`, and pass that exact SHA:

```sh
cd /home/yavdr/vdr-suite-packaging-test
bash tools/build_suitebridge_deb_isolated.sh <VERIFIED_40_CHARACTER_HEAD_SHA>
```

This is a **binary package test only**. It does not create a signed Ubuntu source upload, contact Launchpad, install the built package, enable the plugin, modify VDR configuration or restart services.

## Blockers and caveats

- The host must already provide `dpkg-buildpackage`, `dh`, `dh_vdrplugin_depends`, `vdr-dev`/matching `vdr.pc`, compilation tools and all plugin-only build dependencies. Verify target VDR ABI separately from product version.
- The distro `vdr.pc` `libdir` must match the currently hardcoded Debian plugin install manifest (`/usr/lib/vdr/plugins`). If not, stop and fix packaging layout rather than bypassing the guard.
- A failed `dpkg-buildpackage` must be diagnosed from the isolated build log before changes.
- A successful package build alone is not installation/upgrade/recovery acceptance or approval to upload to the Ubuntu PPA.
- Scratch directories are deliberately preserved for inspection; explicit cleanup is a separate approved operation after disk headroom is rechecked.
