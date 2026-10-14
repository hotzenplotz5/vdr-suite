# Debian source-package readiness: isolated smoke-test

Status: source-only build helper and unit tests committed; the experiment has **not yet been run**.

## Why this is the next gate

At yaVDR candidate `ce63332dc72b0a262f1f30b410c18493668c2df1` the standalone `vdr-plugin-suitebridge_0.14.1-1~exp1_amd64.deb` build was confirmed, but its `_amd64.changes` is binary-only. Launchpad does **not** accept a prebuilt `.deb` as an upload. A Debian `3.0 (quilt)` source package needs a corresponding upstream `.orig.tar.xz` without the `debian/` directory plus the Debian source metadata.

The offline tool `tools/build_vdr_suite_sourcepkg_isolated.py` stages **only Git-tracked files at a verified SHA**; it generates a versioned upstream tarball excluding `debian/`, then runs `dpkg-buildpackage -S -sa -us -uc -d` and round-trips the `.dsc` through `dpkg-source -x` in a separate scratch directory.

## Safety and execution

The tool is restricted to non-root user `yavdr`, the dedicated checkout `/home/yavdr/vdr-suite-packaging-test`, exact local and remote branch HEAD, clean working tree, required packaging programs, existing `unstable` changelog and 2 GiB scratch budget with at least 4 GiB/15% space reserve retained. It has a read-only `--preflight-only` mode.

Run from the approved checkout with a freshly verified exact commit:

```bash
python3 tools/build_vdr_suite_sourcepkg_isolated.py <EXACT_VERIFIED_COMMIT> --preflight-only
python3 tools/build_vdr_suite_sourcepkg_isolated.py <EXACT_VERIFIED_COMMIT>
```

A failure reports the retained source build area/log for diagnosis. Nothing is removed automatically. **No sudo, apt installation, service change, package installation, upload, signing or production update** occurs.

## Important non-acceptance

This output would still be **unsigned** and targeted at `unstable`, not at Ubuntu noble/resolute. It is a smoke test of source packaging only. Packaging the entire source does not confirm license clearance or that the complete daemon/agent binary packages build on the target Ubuntu release. A separate signed, series-specific changelog and source-only build is needed for the Launchpad test PPA, after all required legal/build/lifecycle gates.

Official references: Debian `dpkg-source(1)` for the `3.0 (quilt)` format and Ubuntu Launchpad's `Upload a package to a PPA` manual.
