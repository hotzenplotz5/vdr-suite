# VDR-Suite Ubuntu test PPA

Status: Ubuntu 24.04 noble + Seahawk VDR 2.8.3 three-binary package build and disposable install/reinstall/remove/purge **PASS** in GitHub Actions #29 (2026-10-14). Earlier unsigned Debian and noble source previews PASS; **no signed source upload, Launchpad build or public PPA publication**.

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
- **Critical:** Launchpad's default profile builds all three packages. A complete Ubuntu noble + Seahawk VDR 2.8.3 default build has now PASSED in GitHub Actions #29, separate from a plugin-only build. This is **not** yet a proof of identical Launchpad chroot resolution from a clean, signed source upload.
- Offline `--ubuntu-preview noble|resolute` still generates unsigned series-specific source metadata only; the complete noble binary build has separately passed in GitHub Actions, while resolute remains untested.
- The complete license/provenance assessment and `debian/copyright` review remain open. Never fabricate blanket copyright claims or silently ship third-party media.
- Launchpad requires an upload signed by an OpenPGP key associated with the Launchpad account. The PPA signs the binaries *after* building them; this does not replace the user's upload signature.

## Requested Launchpad build dependencies: Seahawk VDR 2.8.3

**Maintainer request (2026-10-14):** build the VDR-Suite PPA against seahawk1986's VDR 2.8.3 and matching plugin packages, rather than relying exclusively on stock Ubuntu's older VDR. The archive owner is `seahawk1986-hotmail`, not `seahawk1986`.

The requested external **Launchpad PPA build dependency** is:

- `seahawk1986-hotmail/vdr-2.8.3` (PPA: `ppa:seahawk1986-hotmail/vdr-2.8.3`), see https://launchpad.net/~seahawk1986-hotmail/+archive/ubuntu/vdr-2.8.3.
- Target PPA: `ppa:hotzenplotz5/vdr-suite` at https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite.

**Confirmed through maintainer screenshot of authenticated Launchpad UI (2026-10-14):** The **Edit PPA dependencies** page visibly lists `vdr-2.8.3` in the existing dependency table, with **pin priority 500**, and says **"This dependency is already registered"** when attempting to add it again. This establishes that the UI already registered the PPA dependency. The read-only anonymous API checker returned an empty dependencies collection even while this UI showed the record; the checker result must **not** be interpreted as proof of a missing dependency. To review/alter it: open the target PPA → **Edit PPA dependencies** → search for `seahawk1986-hotmail/vdr-2.8.3` → select → Save. Launchpad then uses the additional archive to resolve Build-Depends, including `vdr-dev`, where the requested series and architecture are actually published. Build priority is configurable in Launchpad since September 2026; prefer a deliberate policy and inspect the result rather than silently pinning all source packages.

For `noble` and `resolute`, **verify actual publication** of the VDR 2.8.3 `vdr-dev`, its plugins and dependency chain before launching builds. Seahawk's release-specific supplemental PPAs such as `seahawk1986-hotmail/noble-main` and `seahawk1986-hotmail/resolute-main` may be needed if `vdr-2.8.3` alone cannot satisfy dependencies; add only after checking actual dependency resolution, do not blindly mix distribution series.

Important distinctions:

- Launchpad PPA dependencies are **not** `deb-src` entries in the maintainer's local `/etc/apt/sources.list*`, and **not** a `Build-Depends: seahawk1986...` line in `debian/control`. The source still correctly declares `Build-Depends: vdr-dev` and the binary plugin depends on generated `${vdr:Depends}`.
- This configuration affects **Launchpad build chroots**, not users' APT sources. A user installing the suite might separately need Seahawks VDR PPA to satisfy the matching `vdr-abi-*` runtime dependency.
- The successful local `vdr-abi-2.8.3-0yavdr` package build supports the strategy, but does not prove Seahawk's archive contains that exact ABI for each Ubuntu series.
- The maintainer reports that the dependency was saved; **automated verification and official PPA build acceptance are still pending**. Verify the archive appears on the target PPA's dependency listing, then check the build logs select the intended VDR version.

Read-only public API verification from the separate Packaging checkout (no login or system modifications): `python3 tools/check_launchpad_ppa_dependency.py`. `LAUNCHPAD_DEPENDENCY_CHECK=PASS` confirms the archive link only; `UBUNTU_VDR_ABI_VALIDATION=PENDING` remains until successful Ubuntu-series builds. A network/API failure is reported as `UNKNOWN`, not as a false claim that the dependency was removed. Unit tests: `python3 -m unittest discover -s tools -p 'test_packaging_*.py'`.

**Live Launchpad observation (2026-10-14):** The maintainer's read-only check at `d138b8550b9d832ec716edf0e67042702ef06463` passed all 36 tests but reported `OBSERVED_DEPENDENCY_COUNT=0` and `FAIL`. The authenticated screenshot **contradicts that API-derived absence claim**. The checker now performs an optional documented read-only `getArchiveDependency` lookup if the collection is empty. If both anonymous methods provide no record, its status is **UNKNOWN (public API inconclusive)**, not **FAIL**. These results do **not** imply that a `-dev` PPA is needed. `vdr-dev` is a binary development package, normally supplied by the selected VDR archive, not the name of another PPA. Seahawk's distro-specific `noble-main` or `resolute-main` may supply other prerequisites, depending on actual dependency resolution.

Use `python3 tools/check_launchpad_ppa_dependency.py --diagnose` to show **only** the public archive identities already returned by Launchpad, and their pin priorities. If `OBSERVED_DEPENDENCY_COUNT=0`, revisit the target PPA's **Edit PPA dependencies** page; if another PPA is listed, confirm which one was selected and saved. If Seahawk's archive is listed but matching still fails, diagnose the API representation before editing the user's PPA. **No local APT sources, installed VDR, or Launchpad configuration are modified by the script.**

**API compatibility fix (2026-10-14):** The maintainer's initial live run passed all 29 packaging tests but yielded `LAUNCHPAD_DEPENDENCY_CHECK=UNKNOWN (unexpected Launchpad API response)`. The original checker assumed Launchpad's `getByReference` returned a JSON archive object, without accepting a JSON-null/other response. The checker now requests the PPA's canonical public API entry directly, validates the archive's identity, and uses `getByReference` as a fallback. If neither route succeeds, the error describes each route's JSON response type or HTTP status without logging credentials, and the check still reports `UNKNOWN`, **not** `FAIL` or a claim that the dependency was removed. New mock tests cover null responses, alternate API versions and wrong archive identities. The corrected live query has not yet been re-run.


Official procedure: https://ubuntu.com/hardware/docs/image-cookbook/howto/launchpad/create_ppa/ (Edit PPA dependencies); dependency priority background: https://discourse.ubuntu.com/t/introducing-ppa-dependency-pinning/87067.

## Two different OpenPGP signing keys

- **Your uploader signing key:** before the **first source upload**, the maintainer must have a personal OpenPGP key's **public** half registered with the Launchpad user account. The corresponding **private** key stays with the maintainer, is used by `debsign` or `debuild`, and must **never** be uploaded to GitHub, pasted into chat, or copied to the test archive. Check available local private keys as ordinary user: `gpg --list-secret-keys --keyid-format LONG`. Registration: https://launchpad.net/~/+editpgpkeys. No need to create a new key if an existing suitable private key is available.
- **Launchpad's PPA repository signing key:** managed/generated by Launchpad to sign built/published repository metadata/packages. For a brand-new PPA it may not appear before the first source upload; it is **not** the key used to sign the user's upload. https://help.launchpad.net/Packaging/PPA.
- **Seahawk PPA build dependency:** can be set in the logged-in web interface **without either key**. An empty anonymous API collection does not refute the screenshot's registered entry. Build-time package selection and installed ABI for **noble** have now been verified in GitHub Actions #29; official Launchpad chroot behavior and other series remain unverified.
- **No upload yet:** source licensing and clean release-source build, real runtime/migration/upgrade gates and user authorization still apply; the three-binary Ubuntu CI build itself has now passed. Do not sign or upload a preview merely to force key generation.

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

P2–P7 remain **in progress**. GitHub Actions #29 demonstrates a reproducible-in-CI noble/Seahawk three-binary build and disposable package lifecycle, not a signed clean Launchpad build, supported historical upgrade, or real service acceptance. No upload to Launchpad, service restart, host permission change, package installation or main-branch merge is performed by this document.

## 2026-10-17 copyright metadata checkpoint (not an upload clearance)

At branch commit `45521e27d4fc7158075d4c255d83939876f3c2d7`, the packaging work added provisional [DEP-5 `debian/copyright`](../../debian/copyright), a [scope explanation](../../debian/README.md#provisional-debian-copyright-inventory-2026-10-17), and focused source regression coverage. The record preserves the TVScraper GPL-2.0-or-later exception, distinguishes independently introduced service ABI declarations, and **explicitly withholds** copyright/licensing clearance for bundled art with incomplete rights evidence. It is not a release authorization.

The completed [baseline run 38970428064](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38970428064) at `94562a77c61a017f3b3a13d36ea0d0f69947c7bf` passed all 19 CI jobs. Its Resolute Lintian output had three `no-copyright-file` errors, one per .deb, and `LINTIAN_EXIT_STATUS=2`; Lintian was captured but not enforced as a release pass. The new metadata has not been verified in a completed corresponding package build at the time of this checkpoint. For P5 closure, inspect each **rebuilt** binary for `/usr/share/doc/<package>/copyright`, rerun Lintian, and separately resolve the actual remaining art/code rights and publication approval. A lintian-clean package can still be legally blocked.

The Seahawk Launchpad archive dependency is recorded as present in the maintainer's authenticated PPA UI but still requires validation in actual Launchpad series chroots. Uploader OpenPGP identity, source `debian/changelog` target suite, source signing, monotonic PPA version and manual-install migration acceptance remain separate gates. Do not upload or run apt/dpkg against the productive yaVDR.
