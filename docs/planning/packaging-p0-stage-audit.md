# Packaging P0.1 — Staged payload and ELF dependency inventory

Status: IMPLEMENTED IN SOURCE, **NOT YET EXECUTED against real compiled binaries**.

The checked-in tool `tools/packaging_inventory.py` inventories two **existing isolated** `DESTDIR` trees. It never installs, enables or restarts a service, and does not download dependencies. It fails on unexpected payload ownership and overlapping staged paths. Its output is an **audit report, not a package manifest automatically safe to publish**.

## Build and staging prerequisites

Prepare source checkout and build requirements of the chosen distro. After the ordinary server runtime build and native SuiteBridge plugin build have passed separately, stage into two **new throwaway directories**. Example:

```sh
# From repository root, with supported build dependencies already present:
make build-installable
make install DESTDIR=/tmp/vdr-suite-packaging-runtime PREFIX=/usr

# VDR headers and vdr.pc from the target VDR distribution must exist.
make -C vdr-plugin-suite-bridge
make -C vdr-plugin-suite-bridge install DESTDIR=/tmp/vdr-suite-packaging-plugin

# Read-only inventory; only output JSON is written:
python3 tools/packaging_inventory.py \
  --runtime-stage /tmp/vdr-suite-packaging-runtime \
  --plugin-stage /tmp/vdr-suite-packaging-plugin \
  --output /tmp/vdr-suite-packaging-inventory.json

# No VDR needed for these unit tests:
python3 -m unittest discover -s tools -p test_packaging_inventory.py
```

**Caution:** The `make install` stage is only safe with an explicitly set `DESTDIR`; do not run a bare live `make install`. For package builds use clean disposable roots instead of reusing old `/tmp` trees. Compile and execute builds only inside test environments, not on a running production yaVDR machine.

## Report

JSON schema 1 contains sorted absolute package paths, proposed binary package, source stage, file type, SHA-256/size for regular files, symlink targets, and linked ELF `DT_NEEDED` sonames as seen by `readelf -d`. This is **not** a complete dependency resolution (no script interpreters, `dlopen`, executable subprocess dependencies, distribution package names or automatic ABI verification). `dpkg-shlibdeps` and clean-root verification are still needed in P4.

## Known follow-up

- Verify package ownership of the entire real stage. Classification intentionally fails on unknown payloads.
- Add manifest diff and conffile/config owner checks.
- Catch dangling/escaping symlinks, non-reproducible inputs and permissions.
- Link real ABI-specific VDR plugin dependencies to the target distro.
- Reconcile mandatory Backend Agent functionality and potential package split.
- Update the P0/P1 acceptance checklists only after tests have actually been executed.
- For future Ubuntu PPA testing: create signed source packages and let Launchpad build per Ubuntu series; do not upload local binary `.deb` as substitutes.

No live deployment and no published packages are part of this slice.
