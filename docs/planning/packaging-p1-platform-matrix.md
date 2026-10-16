# Packaging P1 — Package boundaries and platform test matrix

Status: P1 package boundaries remain provisional. Full default Ubuntu 24.04
(Seahawk VDR 2.8.3) builds, independent plugin, disposable install and
experimental upgrade have passed [CI #112](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38932136305).
The independent Resolute VDR 2.8.3/API 14 plugin-only package was
subsequently verified on [CI #118](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38936881893):
**15/15 jobs SUCCESS**, including prior Noble builds, disposable P8 collision
cases and P7 package lifecycle; code SHA
`c957f0c0a6c6b21e152315da31152f572c1e59b8`.
This still does **not** certify a full Resolute daemon/agent build,
live VDR operation or productive manual-install upgrade.

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
| Ubuntu 24.04 LTS (noble), Seahawk VDR 2.8.3 | first | three binaries + plugin-only PASS CI #112 | disposable install/remove/purge PASS | `vdr-abi-2.8.3-0yavdr` generated and verified | real running VDR acceptance pending | synthetic `exp1→exp2` PASS, manual upgrade pending |
| Ubuntu 26.04 LTS (resolute), Seahawk VDR 2.8.3 | first | **plugin-only build PASS, exact VDR package/API 14, CI #118**; full daemon/agent packaging pending | **not authorized** | generated `vdr-abi-2.8.3-0yavdr`, packaged `.so.14` verified in CI | running VDR acceptance pending | real manual upgrade blocked |
| Debian 13 | first | pending | pending | pending | pending | pending |
| Debian 12 | compatibility | pending | pending | pending | pending | pending |
| Actual yaVDR Ubuntu 26.04 amd64 host | acceptance | exact-version plugin-only CI #118 PASS; host install/build not performed | **no host installation authorized** | `vdr`/`vdr-dev` `2.8.3-0yavdr0~resolute`, API `14`, `/usr/lib/vdr/plugins`, `vdr-abi-2.8.3-0yavdr` confirmed | `vdr.service` active; **`/proc/<VDR MainPID>/maps` confirms mapped `libvdr-suitebridge.so.14`**; functional handshake unverified | manual upgrade/rollback BLOCKED; 3.36 GB DB with WAL present |

Use target release codename/architecture and exact `vdr-dev` / `vdr` ABI metadata at build time. Do not infer a working plugin package simply from successful daemon build.
The 2026-10-16 yaVDR host observation confirms Ubuntu 26.04
(`ID=ubuntu`, `VERSION_CODENAME=resolute`), amd64 and package-provided
VDR API 14. The Ubuntu 24.04 Seahawk
VDR 2.8.3 CI package build and lifecycle evidence in
[CI #111](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38929697400)
does not certify this installed `~resolute` VDR build, even if the upstream
VDR version is identical. Installed SuiteBridge filenames `.so.11`, `.so.14` and older
backup-suffixed variants are conflicts needing separate review.
A 2026-10-16 read-only `/proc/<VDR MainPID>/maps` inspection confirmed
that the active VDR mapped `/usr/lib/vdr/plugins/libvdr-suitebridge.so.14`
at the observed instant (PID 1757). This closes **loaded plugin identity** for that inspection.
A subsequent non-mutating `/proc/1757/maps` device/inode cross-check
against `stat` for the disk file returned
`MAPPED_FILE_IDENTITY=PASS` (disk inode `1704446`, disk size
`13523792` bytes, SHA-256
`8158311f874f0e1b10620192eb3e35f6f62d10e7723571d33635e7ecc1858580`).
The result establishes the mapped file's identity with the current disk
file at inspection time; the SHA-256 was measured on disk, not from
the mapped memory, and does **not** attest source commit or CI binary
provenance. A subsequent read-only `ss -xlnpH` inspection confirmed that
`/run/vdr/vdr-suite-control/control.sock` is a listening
`SOCK_SEQPACKET` endpoint owned by the same VDR process (PID 1757,
fd 28); parent directory `0700 vdr:vdr` and socket `0660 vdr:vdr`.
That confirms the native control-plane listener and permission topology
at inspection time, not an authenticated CAPS/SNAP exchange.
Protocol operation, upgrade/rollback and service recovery remain
unverified. The [separate Resolute plugin-only CI #118](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38936881893)
successfully resolved exact `2.8.3-0yavdr0~resolute`, compiled API 14,
verified the Debian `.so.14` and automatically generated dependency
`vdr-abi-2.8.3-0yavdr`, and passed 130 packaging regressions.
Tested code SHA: `c957f0c0a6c6b21e152315da31152f572c1e59b8`.
The package was only built in a disposable Ubuntu 26.04 container:
this is **not** a full-daemon build, runtime acceptance, host deployment or
real-manual-upgrade certification. If this exact VDR package is unavailable
in a future build, Ubuntu's stock `vdr-dev` 2.6.9 must not be substituted.


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
- [x] Identify official Ubuntu stock `vdr-dev` versions, host yaVDR ABI, and exact Resolute API-14 SuiteBridge-only compatibility build (CI #118). Full Resolute daemon/agent and end-to-end plugin functionality are pending.
- [ ] Confirm support floor and matrix through clean builds.

P1 remains **proposed** until these validation items pass.
