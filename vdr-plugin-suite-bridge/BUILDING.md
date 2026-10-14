# SuiteBridge independently of the VDR-Suite daemon

The SuiteBridge VDR plugin can be compiled and staged by itself. No VDR-Suite daemon executable, database, or Backend-Agent process is required to **build** this plugin. Its source tree uses a shared C++ header from `../core/agent/include`, so the **full repository source checkout** is still required.

Prerequisites: the target VDR's `vdr-dev`/headers, a working `vdr.pc` pkg-config definition, GNU Make, g++, and the plugin's actual transitive build requirements. Always build against the VDR API of the target machine/distribution. Do not copy one distro's plugin binary to another VDR ABI.

## Direct build (no root)

From the repository root:

```sh
make -C vdr-plugin-suite-bridge
make -C vdr-plugin-suite-bridge check-build
```

Stage without touching the running VDR:

```sh
stage="$(mktemp -d)"
make -C vdr-plugin-suite-bridge install DESTDIR="$stage"
find "$stage" -type f -name 'libvdr-suitebridge.so.*' -print
```

The destination is derived from the installed VDR's `pkg-config --variable=libdir vdr` and `pkg-config --variable=apiversion vdr`. The stage is disposable; don't run `make install` without `DESTDIR` when merely testing.

## Only the plugin Debian binary package (no daemon build)

On a Debian/Ubuntu build environment with the appropriate build dependencies installed, at the repository root:

```sh
dpkg-buildpackage -B -Pplugin-only -us -uc
```

`plugin-only` is a Debian build profile. It excludes the daemon and Backend-Agent binary packages and skips all of their build and install Make targets. The plugin binary package does not depend on the VDR-Suite daemon; it does depend on the compatible VDR ABI.

For a normal combined packaging build, omit `-Pplugin-only`. A Launchpad PPA source upload normally builds the default profile, producing the packages independently installable through APT; a plugin-only PPA source recipe is a separate future option.

## Tests and safeguards

```sh
python3 -m unittest discover -s tools -p 'test_packaging_*.py'
python3 tools/check_packaging_tracked_artifacts.py
```

`debian/rules` must be executable (mode 100755). Build artifacts are ignored by Git, but previously tracked artifacts remain tracked: the audit checks this and never removes any files. Do not run a package build, upload, or installation on production yaVDR until clean-root build and upgrade tests have passed.
