# Packaging P1 — Package boundaries and platform test matrix

Status: PROPOSED (unverified compatibility). Scope intentionally constrained to daemon + VDR SuiteBridge and required support files.

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
| Ubuntu 24.04 LTS | first | pending | pending | pending | pending | pending |
| Ubuntu 26.04 LTS | first | pending | pending | pending | pending | pending |
| Debian 13 | first | pending | pending | pending | pending | pending |
| Debian 12 | compatibility | pending | pending | pending | pending | pending |
| Actual yaVDR Ubuntu host | acceptance | pending | **requires explicit deployment approval** | pending | pending | pending |

Use target release codename/architecture and exact `vdr-dev` / `vdr` ABI metadata at build time. Do not infer a working plugin package simply from successful daemon build.

## Delivery channels

1. Locally built `.deb` plus source artifacts for early verification.
2. Later: private/experimental **Launchpad PPA** for Ubuntu release-specific builds and `apt` update testing. Needs Launchpad account, GPG signing and actual source package readiness; no PPA has been created.
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
- [ ] Verify distribution-specific VDR packages and ABI dependencies.
- [ ] Confirm support floor and matrix through clean builds.

P1 remains **proposed** until these validation items pass.
