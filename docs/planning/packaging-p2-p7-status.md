# Packaging P2–P7 implementation ledger

Status: **in development**, not released. Source-level implementation is distinct from tested packaging.

| Slice | Added now | Remaining release gate |
| --- | --- | --- |
| P2 | Root `VERSION`, compiler input and packaging version checks; 0.14.1 consistency verified by CI | Full installed version-report audit and monotonic target-series changelog still pending |
| P3 | `DESTDIR` install and distinct daemon, agent and plugin payload manifests; isolated builds PASS | Final clean *source archive* default build and legacy install conflict audit |
| P4 | Ubuntu noble Seahawk VDR 2.8.3 ABI verified; Ubuntu 26.04 Resolute API-14 SuiteBridge-only `.deb` and generated ABI dependency PASS [CI #118](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38936881893), code `c957f0c0a6c6b21e152315da31152f572c1e59b8`; live yaVDR `/proc/<MainPID>/maps` confirms mapped `.so.14` and matches the on-disk file's device/inode (`MAPPED_FILE_IDENTITY=PASS`, read-only operator audit, 2026-10-16) | Functional SuiteBridge handshake/runtime tests, full Resolute daemon/agent build, Debian 13, lintian and live-package takeover remain open |
| P5 | Working root `debian/rules` (mode 100755), `3.0 (quilt)` source preview, three separate noble `.deb` outputs and ABI-correct plugin-only profile PASS | `debian/copyright` and third-party licensing, final source build reproducibility and upload metadata |
| P6 | Disposable Ubuntu noble real package install/reinstall/remove/purge and conffile/state sentinel preservation PASS | Systemd startup/user/runtime directory tests, real manual-install transition and actual live yaVDR approval |
| P7 | Tagged historical SQLite replay, additive SQL / synthetic old-column fixtures, 33/33 **named Repository C++ `ensureSchema()`** sources plus GenreIndex .inc exercised offline; synthetic pairing/browser/playback/artwork/manual-metadata and native-recording one-time DML migration rehearsals, and *unreleased* `exp1 → exp2` package upgrade/restore in disposable Noble CI | For first publication: actual **manual-install daemon-created database** compatibility evidence, SQL writers outside the named repository pattern, writer quiescence/WAL backup and restore, enforced source/target version policy and approved service transitions |

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

## Experimental exp1 → exp2 P7 package transition

The previous successful CI #41 commit `bc4623374a4df4c08118c424ea19aaf552a4964c` is now pinned for a separate build of unsigned `0.14.1-1~exp1` packages. The branch changelog advances to `0.14.1-1~exp2` and a new disposable Ubuntu Noble container CI job exercises an **actual dpkg package version transition**, interrupted unpack/configure, preservation of user-edited conffiles and source-attested synthetic SQLite rows, verified offline backup restore and binary downgrade. The [CI run #46](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38640654487) on `098a14046d346cda5f03bbd3abaf4b386ae03428` and the [CI run #47](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38645349742) on `08b0fbfdfb3c9ea959d93d7010801b53d46dbbd1` both completed with all five jobs **PASS**, including `p7-versioned-upgrade-noble`. The latter SHA also passed all **68** local packaging tests, version checks (`0.14.1`, `0.14.1-1~exp2`) and tracked-artifact audit. This establishes an **experimental disposable-container package transition only**, not an upgrade of released legacy binaries, a live daemon-created database or production yaVDR. No PPA signing/upload or live installation is authorized.

## CI #48 — additional SQLite repository owners (2026-10-15)

[GitHub Actions #48](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38676138135)
finished **SUCCESS** on immutable commit `db10fba7164b43ecea8832f5353da9ad1cd0a10c`:
all five Ubuntu Noble packaging jobs passed, including the independent
SuiteBridge build, three-binary build, offline C++ historical-schema rehearsal,
package lifecycle, and the experimental `exp1 → exp2` upgrade/restore/downgrade.
The C++ runner explicitly emitted
`P7_PAIRING_PRECOLUMN_AND_CHANNEL_PRESERVATION=PASS` and
`P7_HISTORICAL_CPP_SCHEMA_REHEARSAL=PASS`.
Pairing migration uses a **synthetic, not historical-source-attested** older
column shape. No released-binary compatibility, complete schema-owner coverage,
production restore or live yaVDR deployment is established.

## First-release migration acceptance — no fictional older package

VDR-Suite currently has no published supported earlier Debian/Ubuntu package
release in this experimental packaging sequence. Therefore **a previously
published binary-package upgrade is not a prerequisite for the first package
release when no such predecessor exists**. Instead the initial release must
name which pre-package/manual VDR-Suite versions are eligible for migration,
exercise representative real-daemon-created databases from those versions in
isolated installations, preserve operator-owned state, and explicitly reject
unknown or newer schemas. The disposable `exp1 → exp2` test proves the dpkg
mechanics only; it does **not** establish support for any manual-install
transition or any real service migration. For subsequent published packages,
version-to-version upgrade tests become mandatory for supported predecessors.

## P7 named schema-initializer coverage (2026-10-15)

The source-audited test ledger is
[packaging-p7-schema-owner-inventory.md](packaging-p7-schema-owner-inventory.md).
The current real C++ offline harness invokes **33 of 33** named
`Repository::ensureSchema()` owners that create tables in the narrowly scoped
`core/*/src/*Repository[Schema].cpp` pattern. GenreIndex is additionally
exercised through its `.inc` source. One-time native-recording-cache TTL
invalidation is exercised against a synthetic pre-marker database, with
restoration of the prior value/marker; manual recording cast completeness and
other additive columns also have synthetic old-column tests. No real VDR
recording, timer, device credential or account is modified by these tests.

**The 33/33 count does not close P7.** Table creation, schema writes and
database operations in other source files still need a separate inventory.
The provided historical tagged SQL is real source provenance, but the
additional old-column and pre-marker fixtures are reconstructed, not actual
databases from released package binaries. The first-release migration gate
remains unfulfilled until we can validate specifically supported prior
**manual** installs using offline copies of databases actually created by
their daemons, backed up consistently with WAL writers quiesced. The dpkg
`exp1 → exp2` rehearsal proves experimental package mechanics only.

## CI #60 — named schema-owner rehearsal complete (2026-10-15)

[GitHub Actions #60](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38715053824)
finished **SUCCESS** for immutable code commit
`412d83706adb0c8becbf8c2b81ce81b88fbdc232`: all five Ubuntu
Noble Seahawk VDR 2.8.3 packaging jobs PASS (independent SuiteBridge,
prior exp1 build, full three-binary build, disposable package lifecycle,
and isolated exp1-to-exp2 upgrade/state/rollback). The real C++ offline
schema test explicitly emitted:
`P7_MANUAL_METADATA_PRE_CAST_MIGRATION=PASS`,
`P7_NATIVE_TIMER_BINDINGS_PRESERVATION=PASS`,
`P7_BACKEND_AGENT_STATE_PRESERVATION=PASS`,
`P7_EMBEDDED_RECORDING_EDITS_PRESERVATION=PASS`,
`P7_NATIVE_METADATA_ONETIME_DML_PRESERVATION=PASS`, and
`P7_HISTORICAL_CPP_SCHEMA_REHEARSAL=PASS`.

The 33/33 counted named C++ initializers do not cover SQL writers outside
that naming pattern, and the synthetic fixtures are not daemon-created
databases from a previously published binary. No installed yaVDR service,
real runtime database or Launchpad archive has been changed.


## P7 post-33 runtime SQL-writer survey (incremental)

A separate C++ runtime DDL file-owner inventory classifies named, included,
indirectly exercised and pending writers. The first additional real C++ test
covers TimerAssignmentSetRevisionRepository.cpp with a synthetic epoch-7
backfill, three triggers and an epoch-8 insertion. Eleven writer source files
remain pending; dynamic SQL, in-file statement changes, migration DML and real
manual-install database transitions are not yet covered. This does not close P7.

## P7 non-repository writer increment — embedded lifecycle

The real C++ `EmbeddedBackendLifecycleService::ensureSchema()` is now
exercised on a private SQLite fixture, including an artificial persisted
backend lifecycle row, idempotent reopens and backup/restore preservation.
The narrow C++ DDL file survey now has 11 explicitly pending writers.
This does not prove compatibility of real manual-install databases.

## P7 static runtime DDL survey milestone (CI #84, 2026-10-15)

The bounded `tools/test_packaging_p7_sql_writer_inventory.py` source-file
survey now classifies **every discovered runtime DDL writer file** in scope:
the final four source owners have passed actual C++ initializer/trigger
regressions in [CI #84](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38801942051)
on `e116824ef0cf30b0effb80285bc3a145f9e8d857`, all five jobs green.
`PENDING=0` is a **source-file DDL inventory milestone**, not a release
acceptance label. `BackendRuntimeGeneration.cpp` remains indirectly
exercised rather than independently tested; SQL DML and constructed SQL
still need separate migration-risk review. See
[the current schema-owner ledger](packaging-p7-schema-owner-inventory.md).

**Do not close P7** without a real (quiesced, safely obtained) supported
manual-install daemon-created database sample, a named old-source SHA or
version and source/target compatibility policy, tested WAL-consistent
snapshot/restore with writer quiescence, fail-closed unknown/future schema
behavior, approved service startup/rollback gates and additional release
checks. No production yaVDR modifications, package upload/signing or
migration are authorized by this CI evidence.

## P7 offline source-schema evidence (2026-10-16)

The read-only `tools/packaging_upgrade_preflight.py` now emits an output
schema revision **3** and a stable `schema_sha256` of ordered
`sqlite_master` DDL object tuples; its algorithm matches the independent
offline recovery rehearsal. It also reports the DDL object count and rejects
SQLite foreign-key violations in addition to quick-check failures. Python
regressions assert that changing only stored row contents preserves the DDL
fingerprint, DDL changes invalidate it, the result contains no test row secrets,
and invalid foreign keys fail without changing the source file.

This is a **source-evidence preparation tool only**, not an upgrade eligibility
decision: `compatibility=REVIEW_REQUIRED` is always emitted. Exact-source
[GitHub Actions #88](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38850863155)
**PASS, all five jobs**, on
`6ab051f8e8dd2bafaf03a26da38117024e202cec`, including **84 Packaging
Python regressions**, independent SuiteBridge, daemon/agent build, disposable
package lifecycle and experimental exp1-to-exp2 recovery. The subsequent
status-ledger-only change does not alter the tested tool or tests. The tool
never reads a production database automatically, performs no DDL/DML, and
neither snapshots writers nor modifies installed services.

P7 remains blocked on an identified and explicitly supported old manual-install
source SHA/version and its authentic daemon-created SQLite snapshot, approved
consistent backup (including WAL and writer quiescence), fail-closed
source/target compatibility registry, isolated real-version migrations and
rollback, and authorized live lifecycle acceptance. Fingerprints alone are
insufficient to approve any of these operations.
