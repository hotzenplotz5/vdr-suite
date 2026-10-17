# VDR-Suite Ubuntu test PPA

Status (2026-10-17): **19/19 CI jobs PASS** on exact packaging commit `cd0bc82ca1ff183c3d7cc69a553a51cd479b9eca`, [GitHub Actions run 38990016336](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38990016336). Noble and Resolute full three-binary builds, plugin-only builds, isolated package lifecycle, P8 refusal, historical exp1-to-exp2 upgrade/rollback, deterministic source package and zero-error Lintian gate all pass. Three Lintian `copyright-without-copyright-notice` warnings remain: the source-level inventory intentionally avoids asserting a specific personal copyright holder without sufficient evidence. **No signed source upload, Launchpad chroot build, full rights clearance or production migration approval**.

## PPA

- Owner: `hotzenplotz5`
- Name: `vdr-suite`
- Public URL (provided by project maintainer): https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite
- Upload target: `ppa:hotzenplotz5/vdr-suite`
- Role: Ubuntu **experimental/testing** only. Debian support must be validated separately.
- Candidate Ubuntu test releases: 24.04 LTS (`noble`), then 26.04 LTS (`resolute`). Both pass isolated GitHub CI with Seahawk VDR 2.8.3; Launchpad and production support remain unconfirmed.

## Verified progress and current PPA build blockers (2026-10-14)

- The maintainer built a standalone `vdr-plugin-suitebridge_0.14.1-1~exp1_amd64.deb` with dependencies including `vdr-abi-2.8.3-0yavdr`, without daemon or Backend-Agent package dependencies.
- A separate local `3.0 (quilt)` Debian source archive and unsigned `_source.changes` were successfully generated and extracted again (`SOURCE_ONLY_PACKAGE=PASS`, `SOURCE_ROUNDTRIP=PASS`) at SHA `39995c626c25909a972b6d9f886ebdc0c82b5fd5`. This source has the `unstable` changelog suite and is *not* suitable for immediate upload.
- The official Ubuntu 24.04 (`noble`) `vdr-dev` package is version **2.6.0-1.1** (source: https://packages.ubuntu.com/noble/vdr-dev); thus `vdr-abi-2.8.3-0yavdr` from local yaVDR must **not** be presumed available in the PPA's noble build environment.
- **Critical:** Launchpad's default profile builds all three packages. A complete Ubuntu noble + Seahawk VDR 2.8.3 default build has now PASSED in GitHub Actions #29, separate from a plugin-only build. This is **not** yet a proof of identical Launchpad chroot resolution from a clean, signed source upload.
- Offline `--ubuntu-preview noble|resolute` generates unsigned series-specific source metadata only. Both noble and resolute full three-binary builds now pass in GitHub Actions on the exact CI commit identified above; neither is a Launchpad chroot acceptance.
- The detailed license/provenance inventories and `debian/copyright` exist and explicitly distinguish original code, adapted TVScraper interface, maintainer-introduced native ABI mirrors and AI-assisted project art. Remaining uncertainty concerns affirmative rights/redistribution authorization for particular materials and definitive personal copyright attributions; do not fabricate them. The three remaining Lintian warnings are not missing-file errors.
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

The exact packaging HEAD `8cf632f10dc42e704f5941f94645102c43686b1b` is **19/19 green** in [GitHub Actions 38995685538](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38995685538) on 2026-10-17. The run covers independent plugin-only builds, full noble/resolute three-binary builds, source roundtrip, disposable install/reinstall/remove/purge, nine P8 overwrite refusals, and a pinned exp1-to-exp2 state-preservation/rollback check. Debian copyright metadata is present in each binary package; Lintian has a zero-error enforced gate. This verifies isolated CI packaging and historical test-fixture upgrade behavior, **not** a Launchpad chroot build, a real already-installed yaVDR migration, or production service acceptance.

The maintainer has now licensed project-specific ChatGPT-generated art under `GPL-3.0-or-later`; third-party TVScraper interface terms and external broadcaster artwork remain separately scoped. Source code/contributor rights and any future bundled contents still require precise review. **No signed upload, archive publication, service restart, host permission modification, package installation on productive yaVDR or main-branch merge has been performed.**

## 2026-10-17 copyright metadata checkpoint (not an upload clearance)

At branch commit `45521e27d4fc7158075d4c255d83939876f3c2d7`, the packaging work added provisional [DEP-5 `debian/copyright`](../../debian/copyright), a [scope explanation](../../debian/README.md#provisional-debian-copyright-inventory-2026-10-17), and focused source regression coverage. The record preserves the TVScraper GPL-2.0-or-later exception, distinguishes independently introduced service ABI declarations, and **explicitly withholds** copyright/licensing clearance for bundled art with incomplete rights evidence. It is not a release authorization.

The completed [baseline run 38970428064](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38970428064) at `94562a77c61a017f3b3a13d36ea0d0f69947c7bf` passed all 19 CI jobs. Its Resolute Lintian output had three `no-copyright-file` errors, one per .deb, and `LINTIAN_EXIT_STATUS=2`; Lintian was captured but not enforced as a release pass. The new metadata has not been verified in a completed corresponding package build at the time of this checkpoint. For P5 closure, inspect each **rebuilt** binary for `/usr/share/doc/<package>/copyright`, rerun Lintian, and separately resolve the actual remaining art/code rights and publication approval. A lintian-clean package can still be legally blocked.

The Seahawk Launchpad archive dependency is recorded as present in the maintainer's authenticated PPA UI but still requires validation in actual Launchpad series chroots. Uploader OpenPGP identity, source `debian/changelog` target suite, source signing, monotonic PPA version and manual-install migration acceptance remain separate gates. Do not upload or run apt/dpkg against the productive yaVDR.

## Next controlled Launchpad source-upload preparation (after exact-head CI green, 2026-10-17)

The following tasks are **not** authorizations to upload or to modify the production host:

1. Keep `debian/changelog` at the experimental `unstable` baseline in the checked-in branch. Generate series-specific `noble` and `resolute` entries only in isolated disposable source trees using the existing source package preview tool. Reconfirm version monotonicity against any versions already published in the target PPA; no archive comparison has yet been certified.
2. Confirm that the existing registered Launchpad dependency `seahawk1986-hotmail/vdr-2.8.3` is usable by the actual target-series Launchpad build chroots. The maintainer's authenticated UI showed the dependency with pin priority 500, but no Launchpad source build has been performed.
3. Verify the uploader's Launchpad account has an associated OpenPGP public signing key and a matching selected signing identity; never commit private key material. Check the source-only package, its `*_source.changes`, `.dsc`, `.orig.tar.*` and `.debian.tar.*` in a disposable checkout before asking for a signed release candidate.
4. Resolve any remaining third-party source or contributor-rights questions at the level of files actually distributed. The maintainer's `GPL-3.0-or-later` approval for ChatGPT-generated project artwork is recorded and must **not** be reverted to `License-grant-pending`.
5. A first experimental upload is a separately authorized, manual action only after these checks. Passing isolated CI does not automatically authorize distribution or productive installation.

## Noble / Resolute source-change acceptance in disposable CI (17 October 2026)

At exact code commit `beda36dbb6579144b5f4fa491a444e7a22131fac`, [GitHub Actions run 38996844523](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38996844523) verified that both Ubuntu-specific **unsigned** source packages can be built, that the `_source.changes` metadata has the target distribution and version, that matching `.dsc` and Debian delta files exist, and that `dpkg-source -x` roundtrips the source without changing `VERSION` or the changelog:

| Ubuntu | Verified source version | CI proof |
| --- | --- | --- |
| 24.04 noble | `0.14.1-1~exp2~ubuntu24.04.1` | `UBUNTU_SOURCE_noble=PASS` |
| 26.04 resolute | `0.14.1-1~exp2~ubuntu26.04.1` | `UBUNTU_SOURCE_resolute=PASS` |

The same `.orig.tar.xz` bytes are produced for both distributions (`UBUNTU_SOURCE_SHARED_ORIG_TARBALL=PASS`); the source-preview artifact stores **checksums and evidence only**, not a signed changeset or upload payload. This CI step uses a pristine `git archive` source and `dpkg-buildpackage -S -sa -us -uc -d -nc`; the `-nc` bypasses `debian/rules clean` when `dh` is absent in the source-only runner. Separate full binary build jobs still run with their actual build dependencies and cleaning.

This proves source-package *technical structure*, not Launchpad uploader eligibility, an OpenPGP signature, PPA version monotonicity against archive records, authenticated Seahawk PPA build-dependency resolution, a genuine Launchpad chroot build or external code ownership. No PPA upload and no productive yaVDR changes were performed.

## Locally signed **offline** source previews after Launchpad OpenPGP validation (2026-10-17)

The maintainer has confirmed that Launchpad **successfully validated** the personal RSA-4096 OpenPGP key with fingerprint `4587F102BE996FA83BCA67D3384ECF9181E54ED8` and UID `Holger Schvestka <hotzenplotz5@gmx.de>`. This is the **personal source-upload signature key**, not Launchpad's separate repository/APT signing key. The signing secret currently resides in the maintainer's local `yavdr` user's GnuPG home. **Back up the secret key securely offline; never commit, upload, paste, or transmit it through CI.**

The isolated source-package helper now supports an *explicit* `--sign-with FINGERPRINT` option only together with `--ubuntu-preview noble|resolute`. It verifies the exact secret-key fingerprint and UID, keeps the checked-in `debian/changelog` untouched, and writes `Holger Schvestka <hotzenplotz5@gmx.de>` as the `Changed-By` identity **only in the disposable Ubuntu preview**. It signs the local source `.dsc` and `_source.changes` with `debsign`, then independently checks both signatures against the selected fingerprint and verifies the source roundtrip. The helper reports `PPA_UPLOAD_READY=NO` and **never invokes `dput` or installs software/services**.

Run this only as the non-root `yavdr` user in the isolated checkout `/home/yavdr/vdr-suite-packaging-test`, never in the production checkout; the helper verifies checkout and exact remote HEAD, repository cleanliness, available tools, and the 15%-or-4-GiB reserved storage before writing up to its 2-GiB budget. First use `--preflight-only` with the explicit signer for a read-only gate, then request a specific series and sign within the disposable staging area. A successful signed preview is **not an authorized release**: its changelog deliberately says `OFFLINE preview`, and external rights, archive-version monotonicity, PPA dependency/chroot availability and user authorization remain distinct upload gates.

The earlier green [run 38996844523](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38996844523) established **unsigned** Ubuntu source previews for both series. The local secret-key/signature functionality cannot be executed in GitHub Actions (which has no private key); unit tests cover guard behavior, while the real signature must be checked on the holder's machine. Do not install any new packages, run `dput`, or alter the productive yaVDR runtime for this preview.

## First real Launchpad Noble Recipe and binary-build finding (2026-10-17)

The maintainer requested and launched the **manual**, public Noble recipe [#4102217](https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite/+recipebuild/4102217) from imported GitHub packaging SHA `266ce744cb3b54118f9dd4fb06379de5f084ad28`. Its *source recipe* completed successfully and queued [amd64 binary build #33851175](https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite/+build/33851175). **The binary build FAILED** (no installable `.deb` was published). The [actual binary log](https://launchpadlibrarian.net/881432950/buildlog_ubuntu-noble-amd64.vdr-suite_0.14.1-1~exp2~ppa1+202610170409~ubuntu24.04.1_BUILDING.txt.gz) shows that the Seahawk VDR 2.8.3 dependency **did resolve**: `vdr-dev 2.8.3-0yavdr0~noble` was installed in Launchpad's genuine Noble sbuild environment. This is stronger evidence than the earlier CI-only dependency proof.

The binary build's `override_dh_auto_test` failed with **11 errors and 1 failure**, not a C++ compiler error:

1. `tools/test_packaging_recovery_rehearsal.py` wrote temporary SQLite fixtures under `Path.home()`; Launchpad sbuild deliberately uses `HOME=/sbuild-nonexistent`, so `tempfile.TemporaryDirectory(..., dir=Path.home())` raised `FileNotFoundError`. The replacement uses only newly generated, private mode-0700 `/tmp/vdr-suite-p7-recovery-*` fixture directories, independently of `HOME` and inherited `TMPDIR` (which might point inside a protected checkout); the scoped temporary-directory context removes only the fixture it created. The actual recovery helper's prohibitions on root, protected paths, symlink/hardlink sources, non-private workspaces, insufficient disk reserve and destructive overwrites are preserved. The regression now explicitly mocks the missing sbuild home and asserts mode, path and cleanup.
2. `tools/test_packaging_p7_versioned_upgrade_contract.py` required the *first two* changelog revisions to be `0.14.1-1~exp2` and `0.14.1-1~exp1`. The recipe legitimately prepends `0.14.1-1~exp2~ppa1+202610170409~ubuntu24.04.1`. The new check verifies the exact exp2/exp1 entries retain their sequence and validates their real `dpkg --compare-versions` upgrade ordering without assuming that no additional top entry exists.

**Separate packaging limitation, not the cause of this test failure:** the source-recipe log shows `git-build-recipe --allow-fallback-to-native` and consequently `3.0 (native)`, although the tracked `debian/source/format` says `3.0 (quilt)`. It also warns the auto-generated PPA revision sorts before the checked-in exp2 baseline. Do not misreport this experimental recipe as a clean quilt-source release; repair or replace the recipe-based delivery path before approving the final public package distribution.

The scoped GitHub changes above do **not** change productive yaVDR state. **Do not use `Retry this build` on old Launchpad build #33851175 to test the fix**: that retries the already-uploaded source at the old revision. A *new* recipe build, only after reviewing the exact new GitHub SHA and any remaining PPA/publication gates, is required to exercise fixed source. The PPA remains an experimental public archive; this document is not an unconditional approval to publish or install.
