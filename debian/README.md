# Experimental Debian source packaging

**PPA status (17 October 2026):** The Debian/Ubuntu packaging is in
[merged PR #494](https://github.com/hotzenplotz5/vdr-suite/pull/494) on `main`
(SHA `2a81dea07455ad2801ac959d9a8fcf759dbc4994`). The existing
Launchpad recipe `vdr-suite-packaging-experimental` now tracks `main`
and targets Noble and Resolute.

For **Noble 24.04**, the three VDR-Suite binaries were published as
`0.14.1-1~exp2~ppa2+202610170549~ubuntu24.04.1`, with real isolated
APT/daemon/HTTP/plugin-loader acceptance in
[run #39038795095](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39038795095).
For **Resolute 26.04**, all three binaries were published as
`0.14.1-1~exp2~ppa2+202610171542~ubuntu26.04.1`, and the actual
APT installation, VDR ABI, daemon/HTTP and restart passed in
[run #39070097283](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39070097283)
on `main`. Both series also have the separately packaged patched
`vdr-plugin-web`.

Old statements below such as “no Launchpad upload”, “Resolute pending” or
“main integration in progress” describe **historical checkpoints only**.
The PPA remains **experimental**; neither CI nor successful isolated
runtime checks authorize existing yaVDR migration, hardware playback or
automatic Daily publication. Launchpad's *Built daily* option has no
built-in dependency on GitHub CI success; retain manual builds pending an
explicit publishing policy. [Current PPA acceptance](../packaging/README.md).

This root `debian/` directory is the canonical source-package location for `dpkg-buildpackage`. The historical `packaging/debian/` directory is documentation, not an alternative `debian/` source tree.

## Independent VDR plugin build

SuiteBridge can be built **without building the daemon or Backend Agent**:

```sh
make -C vdr-plugin-suite-bridge
```

For an experimental Debian/Ubuntu plugin-only binary package, from a clean test checkout with `vdr-dev` and other build prerequisites installed:

```sh
dpkg-buildpackage -B -Pplugin-only -us -uc
```

The `plugin-only` profile removes both server binary packages from `debian/control` and bypasses the server and agent build/install rules. It does **not** require or install the VDR-Suite daemon binary package. The SuiteBridge source still references shared internal headers in the same repository, so the entire source checkout is needed.

Without the profile, the source package attempts to build daemon, Backend Agent and SuiteBridge as separate **installable binary packages**. The plugin must carry the ABI-specific `${vdr:Depends}`, populated by the VDR debhelper sequence. The exact VDR `vdr.pc`, `libdir` and ABI must match each target Debian/Ubuntu series.

## Backend Agent package: roles and deployment

The default build produces `vdr-suite-daemon`, `vdr-suite-backend-agent` and `vdr-plugin-suitebridge` as **separate** `.deb` files. The Agent is a backend-side machine component, **not** another Web UI, not a replacement for the main daemon, and not a separate database server. The current `debian/control` does not declare the Backend Agent as an explicit hard dependency of `vdr-suite-daemon`. Building all three packages is not a requirement to *enable* all three services.

| Deployment mode | Agent choice |
| --- | --- |
| One VDR with the central daemon on the same host | Do not assume that a separately running agent service is mandatory. An embedded/local backend path and an explicitly enrolled Agent are different configurations; validate the selected topology before release or installation. |
| Additional VDR backend (same network) | Install/configure an agent *on the backend host* when that backend is managed through the authenticated Agent Protocol; the central daemon stays at the Control Plane. |
| Remote site | Agent initiates outbound HTTPS to the central server. Do not expose local VDR plugin ports or a database to the Internet. |

The daemon package also includes the `/usr/bin/vdr-suite-logo-sync` and
`/usr/bin/vdr-suite-media-calibrate` utilities, with dedicated section-1
manual pages. Logo synchronization may download third-party artwork, which
remains subject to its own provenance and usage rights. Media calibration
benchmarks FFmpeg workloads and writes a persistent performance profile, so
it is an explicit operator action, not part of package installation.

**Packaged artwork versus runtime cache:** The Debian build stages VDR-Suite's
bundled branding, genre artwork and live-remote illustration at
`/usr/share/vdr-suite/channel-logos/vdr-suite-brand/`, with the corresponding
`/channel-logos/vdr-suite-brand/...` URLs resolved by an HTTP-server read-only
fallback after explicitly configured logo roots and
`/var/cache/vdr-suite/channel-logos`. No package-owned branding file belongs in
`/var/cache`: the cache remains for operator/runtime-synchronized broadcaster
logos and any pre-existing local overrides. This is a **Debian-only CACHEDIR
staging override**; the canonical manual `mk/install.mk` default remains
unchanged. Do not copy or delete operator cache data during package installation.

**Exact payload from `debian/vdr-suite-backend-agent.install`:**

| Installed path | Responsibility |
| --- | --- |
| `/usr/sbin/vdr-suite-backend-agent` | Agent process and outbound Control Plane connection |
| `/usr/sbin/vdr-suite-backend-agent-enroll` | Administrator-controlled, one-time enrollment-package creation for an existing backend |
| `/usr/sbin/vdr-suite-backend-agent-admin` | Local Agent status and explicit revocation |
| `/usr/sbin/vdr-suite-backend-agent-command-admin` | Administrative command tool; not a client-facing API |
| `/etc/vdr-suite/backend-agent.conf` | Non-secret service configuration |
| `/usr/lib/systemd/system/vdr-suite-backend-agent.service` | Restricted systemd service (usrmerged package path) |
| `/usr/share/man/man8/vdr-suite-backend-agent.8`, `vdr-suite-backend-agent-enroll.8`, `vdr-suite-backend-agent-admin.8`, `vdr-suite-backend-agent-command-admin.8` | Manual pages, including protected command administration |

The Debian package stages systemd units under `/usr/lib/systemd/system` rather than the aliased `/lib/systemd/system`. On a previously **package-managed** `exp1` installation, dpkg may register the old `/lib` path even when both spellings resolve to the same file on an usrmerged system. The P8 preinst permits that **one exact historical unit alias only** after confirming `/lib` is a symlink, both paths resolve to the same inode, the old dpkg owner is the same binary package and its installed executable is package-owned. Foreign/unowned paths, query errors, manually managed units and first installs remain blocked. This is not a general takeover authorization. This is a Debian-only staging override, not a change to the already deployed runtime tree.

The packaged systemd unit runs under `vdr:vdr` with `NoNewPrivileges`, `UMask=0077`, system protection and a private `StateDirectory=vdr-suite/backend-agent` (mode `0700`), located under `/var/lib/vdr-suite/backend-agent`. The experimental `debian/rules` deliberately calls `dh_installsystemd --no-enable --no-start`: **package installation must neither auto-enable nor start the Agent**. Runtime state and credentials are **not** included in the `.deb`.

### Configuration and enrollment boundary

The shipped `/etc/vdr-suite/backend-agent.conf` is a **template**, not an immediately functional connection:

- `CONTROL_PLANE_URL=https://control-plane.example.invalid` is intentionally invalid. Set the real trusted HTTPS Control Plane origin (including any necessary proxy base path) during an approved configuration process; TLS hostname and peer verification are mandatory.
- `BACKEND_ID=default` names the target backend; it must match a backend that has actually been registered/authorized in the central Control Plane.
- `IDENTITY_PATH` and `ENROLLMENT_PATH` refer to protected Agent state under `/var/lib/vdr-suite/backend-agent/`. One-time enrollment secrets and runtime credentials must not be copied into the non-secret config file, logs or README.
- `OBSERVATION_DOMAINS=backend-health` with empty `ADAPTERS` is the shipping default. Channel observations require **both** the explicit `channels-conf` adapter and `channels` observation domain, with `CHANNELS_CONF_PATH` pointing to a permitted local snapshot source. There is no implicit fallback to VDR provider endpoints.
- The config declares the known native timer `COMMAND_TYPES`; this is an allowlist input, **not blanket permission to change VDR**. Commands require the actual enrolled Agent, current generation, Control Plane authorization and runtime capability/provider gates. Other native actions remain gated separately.

The privileged `vdr-suite-backend-agent-enroll` tool creates a mode-`0600` one-time enrollment file for an existing central backend (defaults include a 900-second TTL); this is **a central database operation** and must not be run casually against a live installation. The local `vdr-suite-backend-agent-admin --status` is a read-oriented administrative interface; `--revoke` changes persisted Agent trust and is not a routine diagnostic. Refer to the supplied manual pages and the [Phase-63 runtime acceptance contract](../docs/development/phase-63-backend-agent-runtime-acceptance-runbook.md) before any operator procedure.

**Separation:** Client apps authenticate against the central daemon, which owns user authorization, backend registry and the database. Backend Agents authenticate with their own technical identities, report bounded state and perform explicitly gated backend-local operations. The Agent does not require exposing VDR SVDRP, RESTfulAPI or SuiteBridge to remote clients. See [ADR-0039](../docs/adr/ADR-0039-backend-agent-control-plane-boundary.md).

### Packaging acceptance versus release

On exact code SHA `d144a9cb69e82f5cc60d93962e512a21595a1616`, [CI #135](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38968900186) **passed overall**: full binary builds and disposable install/reinstall/remove/purge on Noble and Resolute, nine real P8 overwrite-refusal fixtures, and a genuine pinned historical `exp1`→`exp2` `dpkg` upgrade including interrupted unpack, configuration, unchanged SQLite/configuration, and offline rollback rehearsal. This is strong **package lifecycle evidence**, but it does **not** verify an enrolled/connected Agent, a live VDR backend, runtime command permissions, a production manual-install takeover or an unassisted live SQLite migration. The subsequent artwork-path relocation remains separately subject to its exact CI checks.

The P8 native `preinst` collision guards protect already-manually-installed paths. In particular, do not run package installation, overwrite existing `/etc/vdr-suite/backend-agent.conf`, change `/var/lib/vdr-suite/backend-agent`, re-enroll or restart the Agent on the productive yaVDR without an explicitly approved migration and rollback plan.

## Git and safe staging

`debian/rules` is executable (Git mode `100755` required by dpkg). Source metadata (`debian/control`, `rules`, `changelog`, `*.install`) remains tracked. Generated `.deb`, `.dsc`, `.changes`, buildinfo, `debian/tmp`, debhelper output and `libvdr-suitebridge.so.*` are ignored. Already tracked outputs are **not** automatically removed; check read-only with:

```sh
python3 tools/check_packaging_tracked_artifacts.py
python3 -m unittest discover -s tools -p 'test_packaging_*.py'
```

[Standalone plugin instructions](../vdr-plugin-suite-bridge/BUILDING.md) and [experimental Ubuntu 24.04 CI workflow](../.github/workflows/packaging-plugin-experimental.yml).

## Source-package smoke test (unsigned, offline)

A separately gated helper prepares a Debian `3.0 (quilt)` source archive from the exact Git commit, with the upstream tarball containing no `debian/` directory:

```sh
python3 tools/build_vdr_suite_sourcepkg_isolated.py <VERIFIED_HEAD_SHA> --preflight-only
python3 tools/build_vdr_suite_sourcepkg_isolated.py <VERIFIED_HEAD_SHA>
```

This is a local, non-root source-package roundtrip. It **does not** sign packages, change the changelog distribution from `unstable`, upload to Launchpad, or validate the complete daemon/agent binary build. See [source package acceptance](../docs/planning/packaging-source-package-experiment.md).

**Hosted deterministic-source evidence:** [CI #128](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38957762174), tested workflow/code SHA `de5cab976e883bc7dcce6cca92bbab45c88c89a6`, built two separate Git-tracked `3.0 (quilt)` source packages without the Seahawk PPA. Both `.dsc` roundtrips passed; the original upstream tarball, Debian tarball and `.dsc` have identical SHA-256 checksums across the two builds (`SOURCEPKG_SOURCE_TAR_DSC_REPRODUCIBILITY=PASS`). This is **unsigned source reproducibility**, not licensing clearance, a signed PPA source upload, or an installed binary-package acceptance. The separate host-specific source helper above retains its safety gates.

The standalone SuiteBridge binary package **has now been built successfully** on the maintainer's yaVDR VDR ABI `vdr-abi-2.8.3-0yavdr`, with no daemon dependency, at candidate `ce63332dc72b0a262f1f30b410c18493668c2df1`. This is *not* official Ubuntu compatibility certification.

## Release block

**NOT RELEASE-READY.** Ubuntu noble against Seahawk VDR 2.8.3 has a verified full three-binary build plus disposable package install/reinstall/remove/purge (GitHub Actions #29). Ubuntu 26.04 Resolute now also has an exact-ABI unprivileged full three-binary build (CI [#123](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38949619320), exact code SHA `a2bd136074cfcc84b981af5abf010c9e4ccd1426`), with package manifests, the generated `vdr-abi-2.8.3-0yavdr` dependency and native P8 preinst guards checked. Resolute install/lifecycle acceptance remains pending. Still open: source-release copyright/third-party review, other distribution ABI coverage, manual source-install migration, real service startup, historic-version DB upgrade/rollback, lintian and release signing. No package upload to Launchpad and no production yaVDR installation are authorized by this document.

In the experimental packages, automatic systemd enable/start is explicitly disabled. No maintainer script performs database migration.

## Full default-profile build on Ubuntu 26.04 Resolute (2026-10-16)

On exact code SHA `a2bd136074cfcc84b981af5abf010c9e4ccd1426`, the [CI #123](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38949619320) `full-default-packages-resolute` job built all three `0.14.1-1~exp2` amd64 binary packages as a non-root user with exact Seahawk `vdr`/`vdr-dev` `2.8.3-0yavdr0~resolute`, VDR API 14 and ABI `vdr-abi-2.8.3-0yavdr`. CI verified package names, versions, Depends, manifests, SuiteBridge `.so.14` and the generated native P8 `preinst` refusal guards. `nodejs <!plugin-only>` is a build-time dependency for generating a browser diagnostics page in the full daemon installation stage; the independent SuiteBridge-only profile does not need it. The CI artifacts are **unsigned experimental binaries, not a PPA publication**. No live or isolated Resolute package installation is implied by this binary-build milestone.

## Verified CI full-build staging ownership (2026-10-14)

The first full Ubuntu 24.04 + Seahawk VDR 2.8.3 compilation reached `dh_missing --fail-missing` but failed because three staged assets had no package owner. The daemon now declares ownership for `/usr/bin/vdr-suite-media-calibrate` (a Python 3 command) and the inactive Nginx snippet `/etc/nginx/snippets/vdr-suite.conf`. The independent SuiteBridge binary package owns `/usr/lib/tmpfiles.d/vdr-suite-live.conf` because VDR runs as user `vdr` and requires private `/run/vdr` transport roots. The plugin-only profile explicitly stages that file without invoking the daemon build. No Nginx site is activated or VDR service started by this change.

The old `dh_missing` failure was recorded in [GitHub Actions run #26](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38584786074). A successful replacement full build occurred in [GitHub Actions #27](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38586364547) and was repeated with 50 passing Packaging tests in [#29](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38588255607). This validates the experimental noble binary-build gate but does not complete P5 legal/source-release acceptance. Whether manual existing Nginx snippets collide with dpkg conffiles, and install-time tmpfiles execution under target VDR, remain P6 gates. Neither a package install nor an upgrade is authorized on production yaVDR.

## Experimental P7 versioned upgrade job

The branch now builds `0.14.1-1~exp2` separately from pinned earlier commit `bc4623374a4df4c08118c424ea19aaf552a4964c` (`0.14.1-1~exp1`). GitHub Actions constructs both **unsigned** package sets in disposable runners, tests a real dpkg upgrade and later an explicit binary downgrade after restoring synthetic offline test data. This is not an on-host install procedure or evidence of supported runtime migration. See [`P7 upgrade/recovery`](../docs/planning/packaging-p7-upgrade-recovery.md).

## Debian copyright and project artwork license (2026-10-17)

The DEP-5 [`debian/copyright`](copyright) records the intended `GPL-3.0-or-later` licensing for authorized original code and for the VDR-Suite graphics generated by ChatGPT from the maintainer's ideas. The maintainer explicitly selected the same open-source license for project branding, icon and genre artwork on 2026-10-17. The former pending-license markers for these VDR-Suite graphics have been removed. This does not create exclusive copyright in unprotectable AI output, certify trademark clearance, or extend to downloaded broadcaster logos, TV/movie posters, or the separately maintained VIDAA repository.

The adapted TVScraper `services.h` continues to retain `GPL-2.0-or-later` attribution and the HbbTV/Teletext ABI mirrors retain scoped provenance. A generic contributor credit cannot verify every copyright holder; relevant non-artwork source rights and actual distribution scope remain subject to the [license inventory](../docs/third-party-license-inventory.md) and [compatibility audit](../docs/gplv3-compatibility-audit.md).

At exact code SHA `cd0bc82ca1ff183c3d7cc69a553a51cd479b9eca`, [GitHub Actions run 38990016336](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38990016336) passed all 19 CI jobs, confirmed a copyright file in each .deb and reported `LINTIAN_ERROR_GATE=PASS` (three `copyright-without-copyright-notice` warnings remained). The updated artwork licensing has not yet been accepted by a completed exact-head CI run. A zero-error Lintian result is not legal or PPA-release clearance. No Launchpad upload or productive yaVDR takeover is authorized.
