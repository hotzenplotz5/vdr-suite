# Packaging P2–P7 implementation ledger

Status: **in development**, not released. Source-level implementation is distinct from tested packaging.

| Slice | Added now | Remaining release gate |
| --- | --- | --- |
| P2 | Root `VERSION`, compiler input and packaging version checks; 0.14.1 consistency verified by CI | Full installed version-report audit and monotonic target-series changelog still pending |
| P3 | `DESTDIR` install and distinct daemon, agent and plugin payload manifests; isolated builds PASS | Final clean *source archive* default build and legacy install conflict audit |
| P4 | Ubuntu noble Seahawk `vdr-dev 2.8.3-0yavdr0~noble` and plugin ABI `vdr-abi-2.8.3-0yavdr` verified in actual CI logs | Debian 13/Ubuntu resolute ABI and runtime dependency matrix; lintian |
| P5 | Working root `debian/rules` (mode 100755), `3.0 (quilt)` source preview, three separate noble `.deb` outputs and ABI-correct plugin-only profile PASS | `debian/copyright` and third-party licensing, final source build reproducibility and upload metadata |
| P6 | Disposable Ubuntu noble real package install/reinstall/remove/purge and conffile/state sentinel preservation PASS | Systemd startup/user/runtime directory tests, real manual-install transition and actual live yaVDR approval |
| P7 | Offline preflight, verified tagged historical SQLite fixture, additive SQL rollback and real metadata/search/security/timer/operation C++ schema-initializer replay/backup rehearsal in disposable CI | Full daemon-owner migrations, historical binary-package upgrade and controlled real restore acceptance |

## Confirmed local build milestone (2026-10-14)

Maintainer-supplied terminal evidence confirms a successful isolated SuiteBridge-only binary `.deb` build, with an ABI dependency on `vdr-abi-2.8.3-0yavdr`, no daemon or agent binary dependency, and post-build resource reserve PASS. This validates P5's **local plugin-only binary path** on the yaVDR build host only. It does not clear clean Ubuntu builds, daemon packaging, signed source package, installer or P7 upgrade acceptance.

An unsigned, no-install source-package roundtrip and an Ubuntu noble source preview are confirmed PASS on their recorded earlier SHAs; exact current-head source rebuild remains a separate gate. Signed Ubuntu series uploads remain gated by license, build-dependency and lifecycle review.

## October 14: Ubuntu PPA preflight additions

- The original local binary-only SuiteBridge and unsigned source-only `3.0 (quilt)` roundtrip are confirmed **PASS**; neither output has been installed, signed or uploaded.
- Ubuntu `noble` (24.04) officially supplies `vdr-dev` 2.6.0-1.1, which is **not** the yaVDR `vdr-abi-2.8.3-0yavdr` tested on the user's build host. Ubuntu must build its own matching plugin ABI.
- The source helper supports **offline only** `--ubuntu-preview noble` or `--ubuntu-preview resolute`: the target suite and packaging revision are changed only in an isolated source snapshot, not in Git, the normal checkout or the host installation. The maintainer has now confirmed **noble source preview PASS** at `0ff3f56f0b91e390fed7e4d6ff872715571bf740`; resolute and clean Ubuntu binaries are not yet tested.
- Source dependencies now declare required daemon/agent development headers for libcurl, OpenSSL, libcrypt and SQLite under `<!plugin-only>`, so the independent SuiteBridge profile stays lean.
- Ubuntu 24.04 CI now has three successful jobs: independent SuiteBridge, full default three-binary build and disposable package install/reinstall/remove/purge, all against Seahawk VDR 2.8.3. Actual Launchpad source builds remain unverified.
- Release blockers remain: a fresh release-source build, legal source/license inventory and `debian/copyright`, non-tested service startup/user/config transitions, P7 older-version upgrade/restore acceptance, and user-authorized signing/upload.

## Seahawk Launchpad dependency configuration (2026-10-14)

The maintainer confirms having added `ppa:seahawk1986-hotmail/vdr-2.8.3` to `ppa:hotzenplotz5/vdr-suite` using Launchpad's **Edit PPA dependencies**. This is a user-reported completion, not yet an independently verified public API result. A new read-only checker, `python3 tools/check_launchpad_ppa_dependency.py`, validates the external archive link and reports pin priorities when Launchpad's public API is accessible. GitHub's Ubuntu noble build and package-install logs now prove Seahawk `vdr-dev` and `vdr` 2.8.3 resolution with the ABI `vdr-abi-2.8.3-0yavdr`; other distribution series and Launchpad chroot resolution remain unproven. The local host APT sources have not been changed by this work.

## Essential release policy

- No package maintainer script may silently delete `/var/lib/vdr-suite`, secrets, recordings or a user-managed configuration.
- `remove` keeps state and configuration. A `purge` must not delete valuable user data by default without explicit opt-in/data export guidance.
- DB rollback means restoring a validated pre-upgrade backup, not running an undocumented reverse migration.
- No database mutation in `postinst` until a tested reversible migration sequence and explicit transactional safety are available.
- Do not turn on or restart VDR automatically on the production yaVDR target during experimentation.
- Existing manually installed files under `/usr/local` must be detected before package deployment; dpkg ownership does not protect these paths.
- Ubuntu Launchpad PPA only after source package and isolated installation/upgrade tests pass.

## Candidate release acceptance

1. Build and test in fresh Debian 13 and Ubuntu 24.04/26.04 containers/VMs as available.
2. `dpkg-buildpackage -us -uc` succeeds from a pristine source package.
3. `lintian`, shared library, and VDR ABI dependencies verified.
4. Install, reinstall, remove, purge, and upgrade preserve config and database state as specified.
5. Real yaVDR smoke test only under separate deployment approval.

## CI milestone: Ubuntu noble + Seahawk VDR 2.8.3 (2026-10-14)

At [GitHub Actions run #27](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38586364547), both jobs passed on commit `e158ef2ca170280e1cf9f6278bf36c460b3c0d01`: 48 Packaging tests, independently built SuiteBridge with the `vdr-abi-2.8.3-0yavdr` dependency, and the **complete default** `vdr-suite-daemon`, `vdr-suite-backend-agent`, and `vdr-plugin-suitebridge` amd64 packages. A previous `dh_missing --fail-missing` identified an unowned Nginx snippet, calibrator, and transport tmpfiles configuration; the fix assigns each without suppressing `dh_missing`. These are **runner-local experimental binary builds**, not signed PPA uploads or installed yaVDR packages.

At [GitHub Actions run #29](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38588255607), on candidate `42c0ae1d0aa17b376be1a0ef3b0932ddaa59c627`, **all three jobs PASS**: 50 Packaging unit tests, independent plugin, full three-binary build and disposable Ubuntu 24.04 install/reinstall/remove/purge against Seahawk VDR 2.8.3. A modified daemon conffile and an unowned persistent sentinel survived reinstall and remove; the sentinel also survived purge. System services were blocked from starting in the container; real service startup was **not** tested. This validates P6's isolated package lifecycle *subset* only; it is neither a historical-version upgrade/migration test nor live yaVDR acceptance. P7's existing scratch SQLite backup/recovery rehearsal has unit tests, including a no-deletion race test, but no production quiesce/restore integration. Missing legal notices, `debian/copyright`, lintian review, older-version compatibility, and a separately approved runtime smoke test remain explicit release blockers.
