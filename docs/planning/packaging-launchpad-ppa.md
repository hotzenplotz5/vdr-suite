# VDR-Suite Ubuntu test PPA

Status: TARGET IDENTIFIED — not verified as live by automated checks; no source packages uploaded by this work.

## PPA

- Owner: `hotzenplotz5`
- Name: `vdr-suite`
- Public URL (provided by project maintainer): https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite
- Upload target: `ppa:hotzenplotz5/vdr-suite`
- Role: Ubuntu **experimental/testing** only. Debian support must be validated separately.
- Candidate Ubuntu test releases: 24.04 LTS (`noble`), then 26.04 LTS (`resolute`). These are TARGETS, not confirmed supported builds.

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

P2–P7 remain **in progress**; no release-grade package or successful clean distro build has been demonstrated yet. No upload to Launchpad, service restart, host permission change, package installation or main-branch merge is performed by this document.
