# Packaging P1 — Package boundaries and platform test matrix

Status: P1 package boundaries are provisional. Local SuiteBridge-only `.deb` and unsigned Ubuntu noble source-preview tests PASS (2026-10-14), but **no official Ubuntu VDR ABI compatibility or complete binary build is certified**. Scope remains daemon + VDR SuiteBridge and required support files.

## Binary packages (working names)

| Package | Planned file ownership | Relationship |
|---|---|---|
| `vdr-suite-daemon` | daemon, bundled web UI/static assets, essential bootstrap/admin programs, daemon unit/manpages, required runtime configuration | Primary server runtime |
| `vdr-plugin-suitebridge` | ABI-specific `libvdr-suitebridge.so.<VDR-APIVERSION>` and plugin-only documentation | Must depend on target VDR plugin ABI; cannot use portable architecture-independent package |
| `vdr-suite-backend-agent` | agent binary/admin/enrollment helpers, unit, agent config and secure state directories | Optional only after startup/runtime audit proves standalone local mode |
| `vdr-suite` | empty convenience metapackage only | Optional after install topology proven |

Versioned software payload and generated web assets live in `/usr`; admin-owned configuration under `/etc/vdr-suite`; persistent state `/var/lib/vdr-suite`; cache `/var/cache/vdr-suite`. Never put secrets or generated databases into binary package contents.

Do not publish `vdr-suite-dev` or pretend internal C++ code is a stable external ABI. Avoid hard dependency on TVScraper, hardware decode tools or FFmpeg where runtime pathways are truly optional; verify from code before freezing metadata.

## Planned target matrix — NOT yet certified

| Target | Priority | Build | Install | Plugin ABI | Real VDR | Upgrade |
|---|---|---|---|---|---|---|
| Ubuntu 24.04 LTS (noble) | first | **unsigned source preview PASS; full default binary pending** | pending | stock `vdr-dev` 2.6.0-1.1; compile not proven | pending | pending |
| Ubuntu 26.04 LTS (resolute) | first | preview implementation only; binary pending | pending | stock `vdr-dev` 2.6.9-3build1; compile not proven | pending | pending |
| Debian 13 | first | pending | pending | pending | pending | pending |
| Debian 12 | compatibility | pending | pending | pending | pending | pending |
| Actual yaVDR Ubuntu host | acceptance | **Earlier independent SuiteBridge plugin-only `.deb` PASS for an earlier verified candidate** | **requires explicit deployment approval** | Current observed `vdr`/`vdr-dev`: `2.8.3-0yavdr0~resolute`; exact `pkg-config apiversion` and installed plugin ABI still unverified | daemon active from unowned `/usr/sbin/vdr-suite-daemon` | manual upgrade/rollback BLOCKED; 3.36 GB DB with WAL present |

Use target release codename/architecture and exact `vdr-dev` / `vdr` ABI metadata at build time. Do not infer a working plugin package simply from successful daemon build.
The 2026-10-16 yaVDR host observation is a package-version string,
**not** confirmation of the operating-system release. The Ubuntu 24.04 Seahawk
VDR 2.8.3 CI package build and lifecycle evidence in
[CI #111](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38929697400)
does not certify this installed `~resolute` VDR build, even if the upstream
VDR version is identical. Installed SuiteBridge filenames `.so.11`,
`.so.14` and older backup-suffixed variants are conflicts needing
separate review; their suffixes alone cannot establish the active VDR API.


Source of the Ubuntu VDR version rows: [Ubuntu 24.04 vdr-dev](https://packages.ubuntu.com/noble/vdr-dev) and [Ubuntu VDR package-version search including 26.04](https://packages.ubuntu.com/vdr), checked 2026-10-14. The local yaVDR ABI is from a maintainer-supplied successful `dpkg-buildpackage -Pplugin-only` output, not the official Ubuntu package repository.

## Delivery channels

1. Locally built `.deb` plus source artifacts for early verification.
2. Experimental **[Launchpad PPA](https://launchpad.net/~hotzenplotz5/+archive/ubuntu/vdr-suite)** identified by the maintainer (`ppa:hotzenplotz5/vdr-suite`). Upload not yet performed; source signing, clean Ubuntu builds, ownership/license and upgrade gates remain open.
3. Stable APT publication, if desired, is separately gated after build/install/upgrade acceptance. Debian packages tested independently; Ubuntu PPA is not a substitute for Debian support.

## Dependency and lifecycle requirements

- Declare exact `Build-Depends` and generated shared library runtime dependencies.
- Discover optional tools dynamically or declare Recommends/Suggests only where appropriate.
- Model supported `vdr` ABI as a strict package dependency.
- Make `debian/rules` reuse `DESTDIR` install logic and plugin make install.
- Protect existing config/secrets and DBs, distinguish `remove` from `purge`.
- Do not implicitly install/restart VDR/daemon on live yaVDR for testing.
- Detect files already installed from sources before dpkg assumes ownership.
- Test upgrades including service restart policy and plugin ABI transitions.

## P1 exit gate

- [x] Draft component/package boundaries.
- [x] Initial candidate distro and Launchpad test matrix.
- [ ] Verify backend-agent splitting with actual runtime behavior.
- [ ] Finalize package ownership with generated install manifest.
- [x] Identify official Ubuntu stock `vdr-dev` versions and locally built yaVDR ABI; **compatibility build still pending**.
- [ ] Confirm support floor and matrix through clean builds.

P1 remains **proposed** until these validation items pass.
