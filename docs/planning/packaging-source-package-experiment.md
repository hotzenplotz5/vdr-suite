# Debian source-package readiness: isolated smoke-test

Status: **offline 3.0 (quilt) source-package roundtrip PASSED on the maintainer's yaVDR host** at commit `39995c626c25909a972b6d9f886ebdc0c82b5fd5`. A series-specific PPA preview has been implemented but **not yet executed**.

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

## Confirmed offline source build (2026-10-14)

Maintainer terminal output for candidate `39995c626c25909a972b6d9f886ebdc0c82b5fd5`:

- `21` packaging unit tests: **OK**. The argparse usage diagnostic shown during the tests was from an intentionally invalid argument test; it was not an actual packaging failure. The test now captures that expected diagnostic instead of printing it.
- `PACKAGING_VERSION_OK=0.14.1`; `DEBIAN_PACKAGE_VERSION_OK=0.14.1-1~exp1`; `TRACKED_ARTIFACT_AUDIT=PASS`.
- Before and after the build, resource reserve checks: PASS.
- `PRISTINE_UPSTREAM_TARBALL=PASS`, `SOURCE_ROUNDTRIP=PASS`, `SOURCE_ONLY_PACKAGE=PASS`.
- The built files include `vdr-suite_0.14.1.orig.tar.xz`, `vdr-suite_0.14.1-1~exp1.debian.tar.xz`, `vdr-suite_0.14.1-1~exp1.dsc`, and `vdr-suite_0.14.1-1~exp1_source.changes` (the file naming and tarball extraction were observed directly).
- Isolated build location: `/home/yavdr/vdr-suite-sourcepkg-9zgfdhwi`. This remains a local test directory; no upload or deployment occurred.
- The unsigned-source warning is expected: `-us -uc` intentionally prevented signing.

**This is source-format acceptance only**, not proof of clean builds for Ubuntu noble or resolute.

## Ubuntu source-preview mode (new, untested on host)

The same helper now accepts `--ubuntu-preview noble` or `--ubuntu-preview resolute`. It appends a PPA-preview-only changelog stanza **inside the disposable source snapshot**, leaving both the checked-in changelog and `/home/yavdr/vdr-suite` Codec checkout unchanged. Illustrative Debian revisions from the current source base:

- noble: `0.14.1-1~exp1~ubuntu24.04.1`
- resolute: `0.14.1-1~exp1~ubuntu26.04.1`

The source archive is still unsigned, and `PPA_UPLOAD_READY=NO` is always reported. Preview tests verify both codename/version variants and ensure the original changelog is retained below the new stanza.

From the independently checked out Packaging candidate only, after all normal identity and resource preflights:

```bash
python3 tools/build_vdr_suite_sourcepkg_isolated.py <VERIFIED_SHA> --ubuntu-preview noble
```

A **PPA upload must not** use these unsigned preview outputs. Signing the source with the Launchpad-registered OpenPGP key and verifying complete distro-specific binary-build readiness are separate release gates.

## Important non-acceptance

This output would still be **unsigned** and targeted at `unstable`, not at Ubuntu noble/resolute. It is a smoke test of source packaging only. Packaging the entire source does not confirm license clearance or that the complete daemon/agent binary packages build on the target Ubuntu release. A separate signed, series-specific changelog and source-only build is needed for the Launchpad test PPA, after all required legal/build/lifecycle gates.

Official references: Debian `dpkg-source(1)` for the `3.0 (quilt)` format and Ubuntu Launchpad's `Upload a package to a PPA` manual.
