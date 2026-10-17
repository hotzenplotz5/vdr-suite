# Packaging P0 — Source and install baseline

Status: REPOSITORY AUDIT RECORDED; full dependency/link/runtime verification still open.
Scope: daemon, necessary web/administration assets, SuiteBridge plugin, Backend Agent linkage. No live host touched.

## Verified repository observations

| Area | Repository evidence | Packaging consequence |
|---|---|---|
| Daemon build | `mk/common.mk` and runtime make source groups; `VDR_SUITE_SERVER_VERSION` defaults to a Git-derived identifier | Canonical product version and reproducible build override needed (P2) |
| Daemon install | `mk/install.mk` stages `vdr-suite-daemon` under `SBINDIR`, normally `/usr/sbin` for `PREFIX=/usr` | Preserve staged install contract |
| Web runtime | `mk/install.mk` installs frontend modules/assets alongside runtime | Daemon package requires those assets, not just one binary |
| Supporting tools | `mk/install.mk` installs backend agent and administrative/bootstrap helpers | Audit which are required for startup vs optionally split |
| Daemon unit | `packaging/systemd/vdr-suite-daemon.service` has `ExecStart=/usr/sbin/vdr-suite-daemon`, DB and frontend environment | Audit initial activation policy and service dependencies |
| Agent unit | `packaging/systemd/vdr-suite-backend-agent.service` runs as `vdr`, with state ownership and config path | Preserve permission and secret boundaries |
| VDR plugin build | `vdr-plugin-suite-bridge/Makefile` uses VDR `pkg-config`, `APIVERSION`, installs `libvdr-suitebridge.so.$(APIVERSION)` with `DESTDIR` | Build plugin per matching VDR ABI and distribution; no universal binary |
| Plugin version | Plugin version extracted from `suitebridge_plugin_identity.h`; check-version target has an explicit comparison | Remove any divergence/hardcoded version in P2 without breaking plugin runtime identity |
| Debian directory | `packaging/debian/README.md` explicitly says no working `debian/control`/`rules` | Native Debian source packaging still to implement |
| Install policy | ADR-0037, roadmap, runtime staging contracts | Preserve config, database, credentials, source/package ownership |
| Plugin documentation | `vdr-plugin-suite-bridge/README.md` | Historical version/feature descriptions must be checked against current source |

## Gaps and blockers before building release packages

1. Complete source/file ownership manifest: paths installed by `mk/install.mk`, plugin `Makefile`, tmpfiles, units, manpages; mark shared paths and package split.
2. Build dependency inventory: identify headers, pkg-config modules and toolchains with actual clean-root builds.
3. Runtime dependency inventory: `readelf -d`, `dpkg-shlibdeps`, scripts/interpreters, ffmpeg invocations, optional acceleration.
4. Distinguish mandatory Backend Agent architecture from optional remote-node deployment.
5. Verify VDR API/ABI dependency naming of each target distribution, exact plugindir and plugin config naming.
6. Service enablement/start/upgrade and runtime ownership; no root-level live changes during audit.
7. Migration from existing manual/source deploy to package ownership, no silent overwrite.
8. Verify built-in version reporting, release tarballs without `.git`, plugin check-version contract.
9. Run clean `DESTDIR` staging and check full payload; review any install-target deletions/side effects within staging.
10. Resolve discrepancies between aged documentation and current code; source contracts take precedence.

## P0 exit gate

- [x] Existing build, staging, systemd and plugin mechanics inspected.
- [x] Outstanding release blockers inventoried.
- [ ] Complete generated install manifest compared with file owners.
- [ ] Clean isolated dependency/link audit executed.
- [ ] Binary tests, container build and real VDR ABI check executed.

P0 is a **documented reconnaissance slice**, not an accepted finished release test.
