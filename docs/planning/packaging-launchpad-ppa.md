# VDR-Suite Ubuntu test PPA

Status: PPA target recorded; public PPA package list is not verified by automated checks. Local unsigned Debian source roundtrip PASS, Ubuntu 24.04 noble unsigned source preview PASS (2026-10-14); **no signing or upload**.

## PPA

- Owner: `hotzenplotz5`
- Name: `vdr-suite`
- Public URL (provided by project maintainer): https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite
- Upload target: `ppa:hotzenplotz5/vdr-suite`
- Role: Ubuntu **experimental/testing** only. Debian support must be validated separately.
- Candidate Ubuntu test releases: 24.04 LTS (`noble`), then 26.04 LTS (`resolute`). These are TARGETS, not confirmed supported builds.

## Verified progress and current PPA build blockers (2026-10-14)

- The maintainer built a standalone `vdr-plugin-suitebridge_0.14.1-1~exp1_amd64.deb` with dependencies including `vdr-abi-2.8.3-0yavdr`, without daemon or Backend-Agent package dependencies.
- A separate local `3.0 (quilt)` Debian source archive and unsigned `_source.changes` were successfully generated and extracted again (`SOURCE_ONLY_PACKAGE=PASS`, `SOURCE_ROUNDTRIP=PASS`) at SHA `39995c626c25909a972b6d9f886ebdc0c82b5fd5`. This source has the `unstable` changelog suite and is *not* suitable for immediate upload.
- The official Ubuntu 24.04 (`noble`) `vdr-dev` package is version **2.6.0-1.1** (source: https://packages.ubuntu.com/noble/vdr-dev); thus `vdr-abi-2.8.3-0yavdr` from local yaVDR must **not** be presumed available in the PPA's noble build environment.
- **Critical:** by default Launchpad builds every binary package declared by `debian/control` without the experimental `-Pplugin-only` build profile. Our current source declares **daemon, Backend Agent and SuiteBridge**, so a PPA upload must first prove the *complete default build*. The local standalone plugin-only success does not clear that gate.
- A new offline `--ubuntu-preview noble|resolute` mode can generate unsigned, target-specific source metadata in a disposable copy. Preview generation is **not proof that the default three-package build passes** in Ubuntu.
- The complete license/provenance assessment and `debian/copyright` review remain open. Never fabricate blanket copyright claims or silently ship third-party media.
- Launchpad requires an upload signed by an OpenPGP key associated with the Launchpad account. The PPA signs the binaries *after* building them; this does not replace the user's upload signature.

## Requested Launchpad build dependencies: Seahawk VDR 2.8.3

**Maintainer request (2026-10-14):** build the VDR-Suite PPA against seahawk1986's VDR 2.8.3 and matching plugin packages, rather than relying exclusively on stock Ubuntu's older VDR. The archive owner is `seahawk1986-hotmail`, not `seahawk1986`.

The requested external **Launchpad PPA build dependency** is:

- `seahawk1986-hotmail/vdr-2.8.3` (PPA: `ppa:seahawk1986-hotmail/vdr-2.8.3`), see https://launchpad.net/~seahawk1986-hotmail/+archive/ubuntu/vdr-2.8.3.
- Target PPA: `ppa:hotzenplotz5/vdr-suite` at https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite.

**Owner action (not yet performed/verified):** Open the target PPA → **Edit PPA dependencies** → search for `seahawk1986-hotmail/vdr-2.8.3` → select → Save. Launchpad then uses the additional archive to resolve Build-Depends, including `vdr-dev`, where the requested series and architecture are actually published. Build priority is configurable in Launchpad since September 2026; prefer a deliberate policy and inspect the result rather than silently pinning all source packages.

For `noble` and `resolute`, **verify actual publication** of the VDR 2.8.3 `vdr-dev`, its plugins and dependency chain before launching builds. Seahawk's release-specific supplemental PPAs such as `seahawk1986-hotmail/noble-main` and `seahawk1986-hotmail/resolute-main` may be needed if `vdr-2.8.3` alone cannot satisfy dependencies; add only after checking actual dependency resolution, do not blindly mix distribution series.

Important distinctions:

- Launchpad PPA dependencies are **not** `deb-src` entries in the maintainer's local `/etc/apt/sources.list*`, and **not** a `Build-Depends: seahawk1986...` line in `debian/control`. The source still correctly declares `Build-Depends: vdr-dev` and the binary plugin depends on generated `${vdr:Depends}`.
- This configuration affects **Launchpad build chroots**, not users' APT sources. A user installing the suite might separately need Seahawks VDR PPA to satisfy the matching `vdr-abi-*` runtime dependency.
- The successful local `vdr-abi-2.8.3-0yavdr` package build supports the strategy, but does not prove Seahawk's archive contains that exact ABI for each Ubuntu series.
- As of this documentation commit, there has been **no authenticated Launchpad dependency edit** and no official PPA build acceptance. Verify the archive appears on the target PPA's dependency listing after saving and verify build logs select the intended VDR version.

Official procedure: https://ubuntu.com/hardware/docs/image-cookbook/howto/launchpad/create_ppa/ (Edit PPA dependencies); dependency priority background: https://discourse.ubuntu.com/t/introducing-ppa-dependency-pinning/87067.

## Gates before first upload

1. Fix and validate source package completeness (`debian/rules` executable bit, `debian/copyright`, source tarball, changelog suite and version, `debian/*.install` paths).
2. Test the actual Debian build on an isolated root: `dpkg-buildpackage` and `lintian` with clean build dependencies. No production host installation.
3. Confirm `dh_vdrplugin_depends`, `${vdr:Depends}`, plugin output directory and VDR ABI requirements **for each target series**. Avoid universal binary promises.
4. Test staged package ownership, conffiles, installation, upgrade, removal, purge, daemon/agent startup and database backup/recovery in an isolated VM or container.
5. Confirm source package licensing, provenance and no embedded secrets. Version must be monotonic within the target PPA series.
6. Associate an OpenPGP signing key with the Launchpad account; use a matching package author identity. Never commit or upload private keys.
7. Run `debuild -S -sa` from a clean source package tree with a Ubuntu-series-specific `debian/changelog` entry; inspect and sign `*_source.changes`.
8. Only after all gates pass, manually upload the signed source change artifact with `dput ppa:hotzenplotz5/vdr-suite ../<package>_<version>_source.changes`.
9. Verify acceptance, architecture build logs and resulting APT metadata on Launchpad before recommending installation.

Launchpad PPA uploads consist of **source packages**, not prebuilt binary `.deb` uploads. Launchpad then builds the binaries. Its target series is determined by the `Distribution` field in `debian/changelog`.

## Future installation test (not yet executable)

Once an Ubuntu-series build has succeeded and is published, test **only in an isolated Ubuntu VM**:

```sh
sudo add-apt-repository ppa:hotzenplotz5/vdr-suite
sudo apt update
apt-cache policy vdr-suite-daemon vdr-plugin-suitebridge
```

Do **not** run this on the productive yaVDR installation now. PPA availability and package versions have not been confirmed.

## Official documentation

- https://ubuntu.com/docs/launchpad/user/how-to/packaging/ppa-package-upload/
- https://ubuntu.com/docs/launchpad/user/how-to/packaging/build-deb-packages-in-launchpad/
- https://ubuntu.com/docs/launchpad/user/reference/packaging/ppas/ppa/

## Current status and non-goals

P2–P7 remain **in progress**; the standalone yaVDR plugin-only binary build and the local unsigned Debian source roundtrip have both passed, but no official Ubuntu clean build has been demonstrated yet. No upload to Launchpad, service restart, host permission change, package installation or main-branch merge is performed by this document.
