# MLD.1A compatibility inventory — MLD 6.6

**Evidence date:** 2026-10-19. **Scope:** source/read-only investigation; **not** MLD runtime acceptance.
**VDR-Suite source baseline:** `work/mld66-platform-integration` based on `main` `01b74e30ec1a09ee68986059f508284564ef67e1`; first build portability change `4aacd34a2e0f00fce28a1aeb570b2cfa125111c6`.
Keep this branch; no main merge, Phase-70 work, production service changes or package publication are authorized.

## Evidence categories

- **SOURCE-CONFIRMED:** observed in the named VDR-Suite source or published MLD build documentation.
- **MAINTAINER-REPORTED:** information from the MLD developer, not independently reproduced on an MLD host.
- **ADAPT:** an evidenced mismatch or required distribution decision; a proposed approach is **not** a runtime result.
- **OPEN:** missing exact MLD source, configuration, target architecture or real runtime output. No claim of compatibility.

## Matrix

| Area | Evidence | Classification | MLD.1B / acceptance gate |
| --- | --- | --- | --- |
| MLD build orchestration | [MLD-6 mld-dev](https://gitlab.com/MLD-6/mld-dev) README documents Yocto/BitBake via `kas`, including `kas build mld6-x86.yml`, `kas shell` and `bitbake package-index` | SOURCE-CONFIRMED (build family), OPEN (exact selected revision/recipes) | Record immutable mld-dev + layer commits, machine, distro, PACKAGE_CLASSES, compiler/sysroot and feeds |
| VDR runtime/version | MLD developer reports VDR running as root; successful `svdrpsend.sh plug suitebridge CAPS 1` | MAINTAINER-REPORTED | Capture VDR version and `vdr.pc` `apiversion` from the *same* target build; actual process UID/GID |
| SuiteBridge | Source `vdr-plugin-suite-bridge/Makefile`: `PLUGIN=suitebridge`, `-std=c++17`, VDR `vdr.pc`/headers, `install-lib` to `$(LIBDIR)/libvdr-suitebridge.so.$(APIVERSION)`; source identity `0.14.1` | SOURCE-CONFIRMED (build contract); MAINTAINER-REPORTED (MLD installation); OPEN (exact MLD recipe/artifact/build SHA) | Rebuild in MLD against native VDR headers/plugin ABI; inspect recipe, ELF link deps, package ownership and `CAPS 1` output |
| Private RESTfulAPI | MLD developer reports `vdr-plugin-restfulapi` 0.2.6.6; Suite adapter uses `/info.json`, `/channels.json`, `/events...`, `/timers.json`, `/recordings.json`, `/change-state.json`; optional `/searchtimers/conflicts.json` | SOURCE-CONFIRMED (Suite expectations); MAINTAINER-REPORTED (MLD version); OPEN (version-specific response semantics) | Verify status, payload/ID shapes, complete native timer inventory, plugin health, optional endpoints and error handling against **MLD build's patched source**, not version label alone |
| Public API v1 | Exposed by Suite daemon; RESTfulAPI is a **private provider**, not the client API | SOURCE-CONFIRMED (design); OPEN (MLD runtime) | Preserve identical API v1 auth, schema, permission, CSRF, device and timer gates; no plugin-to-browser bypass |
| Daemon build | `Makefile` includes `mk/common.mk`/daemon sources. C++17, SQLite3 through pkg-config; `-lcrypt -lcrypto` in `mk/daemon-sources.mk`. Before MLD change `CXX := g++` and hard-reset CXXFLAGS/LDFLAGS blocked preserving cross-toolchain defaults | ADAPT: MLD branch now retains caller's compiler, CPPFLAGS, CXXFLAGS, LDFLAGS and PKG_CONFIG in `mk/common.mk` | Run actual cross build for MLD architecture; verify headers, libsqlite3, libcrypt, libcrypto, libstdc++/libc and ABI in Yocto sysroot, not guessed package names |
| Backend Agent | `mk/backend-agent-runtime.mk` builds `vdr-suite-backend-agent` plus enrollment/admin tools; separate long-lived process. Service template is `User=vdr`, `Group=vdr` | SOURCE-CONFIRMED; ADAPT (MLD identity/service) | Define dedicated non-root Agent identity/state access; VDR root is **not** a reason to run Agent as root; validate loopback SVDRP and native command transport |
| Daemon service | `packaging/systemd/vdr-suite-daemon.service` has **no** `User=`/ `Group=`, and references `vdr.service`; defaults `/etc/default/vdr-suite-daemon` | SOURCE-CONFIRMED; ADAPT | Native MLD service must specify a dedicated unprivileged user; confirm init manager, ordering, privilege/config/DB access and filesystem policy |
| Staged runtime | `mk/install.mk`: `build-runtime-binaries`, `install-runtime`, `install-systemd`, `DESTDIR`/`PREFIX`, state under `/var/lib/vdr-suite`, cache under `/var/cache/vdr-suite`, frontend under `/usr/share/vdr-suite/web/frontend` for `PREFIX=/usr` | SOURCE-CONFIRMED; ADAPT | Use staged install contract for file ownership; avoid copying source web tree or shipping Debian binary payload into MLD |
| Plugin transport | SuiteBridge uses SVDRP; local daemon/Agent default endpoint 127.0.0.1:6419 in packaged configs. MLD developer says `svdrpsend` fails, `svdrpsend.sh` works | SOURCE-CONFIRMED (transport defaults); MAINTAINER-REPORTED (CLI difference); OPEN (MLD SVDRP bind/socket/ACL/port) | Determine actual SVDRP host/port and allowed callers. The Suite C++ transport opens an SVDRP socket; it does **not** require the CLI wrapper |
| Native mutation | Suite Bridge includes native Timer create/modify/delete paths, but capability advertisement, backend generation, actor/grants and readback must authorize each operation | SOURCE-CONFIRMED (source contract), OPEN (MLD runtime) | No direct unauthenticated SVDRP mutation; no assumption that a successful CAPS response permits writes |
| Media playback | Suite uses FFmpeg/ffprobe process-backed media paths, Gateway, web clients, and optional hardware adaptation | SOURCE-CONFIRMED (code structure); OPEN (MLD supplied binaries/codecs/hardware) | Inventory FFmpeg/ffprobe and formats, writable session scratch, hardware drivers only after package/bootstrap |
| State, upgrades and pairing | Database/identity/account/Device/First Admin live in Suite, not MLD VDR root identity | SOURCE-CONFIRMED (Suite design); OPEN (MLD deploy) | Explicit dedicated owner for DB/state; no chown of recordings; preserve persistent secrets on update; test First Admin, pairing, browser CSRF and grant enforcement |

**Not yet validated:** any MLD package build, MLD linker runtime, RESTfulAPI 0.2.6.6 endpoint payload, MLD SuiteBridge ABI, service user, installation, upgrade, device, timer, EPG, recordings, streaming or security journey. The 0.14.1 source plugin version and the reported MLD installed plugin version must not be conflated with a verified shared-object provenance.

## Dependency boundaries (source-derived; no unverified MLD package names)

| Artifact | Build-time requirement | Runtime/packaging requirement |
| --- | --- | --- |
| `vdr-suite-daemon` | C++17 cross-compiler, Make, pkg-config, SQLite3 headers/libs, libcrypt/libcrypto link; app sources from `mk/*.mk` | ELF dependency inspection on MLD, database/state ownership, runtime HTTP endpoint, optional RESTfulAPI service, FFmpeg/ffprobe for media |
| Web frontend | Installed by `mk/install.mk` from composed `web/frontend` sources | Data files from same source SHA/install-stage; appropriate HTTP serving and static-file path |
| `vdr-suite-backend-agent` | C++17 and `mk/backend-agent-runtime.mk` inputs | Service identity/state + `/etc/vdr-suite/backend-agent.conf`; explicit enrollment, host/port and transport authority |
| `libvdr-suitebridge.so.<apiversion>` | `vdr.pc`, matching VDR headers, C++17, Make, in-tree `core/agent/include` header; any transitive link dependencies must be measured | Native VDR plugin dir, ABI-matched package dependency, discoverable read-only CAPS, network-local SVDRP restrictions |
| Optional runtime | TVScraper, epgsearch, RESTfulAPI/provider integrations depending on installed capabilities | Optional/discoverable; absence must degrade safely and never grant permissions |

The public [yavdr/vdr-plugin-restfulapi source](https://github.com/yavdr/vdr-plugin-restfulapi/blob/master/restfulapi.cpp) **currently declares version `0.2.6.6`** (`restfulapi.cpp`, source blob `e6c46343e0e0973fb57558ef7bf3d1aa8e115219`). Its `serverthread.cpp` registers info/channels/events/recordings/timers/change-state handlers; `changestate.cpp` emits `bootID`, `channelsUpdate`, `recordingsUpdate`, `timersUpdate`, `eventsUpdate`. Its `timers.cpp` implements **GET, POST, PUT and DELETE**, and `recordings.cpp` also has write-capable routes. This confirms upstream **route availability at the named version**, not identity of the MLD binary, MLD patch level, Suite parser compatibility or operational authorization.

**Mandatory private-provider network gate:** upstream version 0.2.6.6 command-line help advertises `--ip=0.0.0.0` and `--port=8002`; its HTTP server binds to configured `Settings::get()->Ip()/Port()`. We **have not established MLD's effective bind or firewall policy**. RESTfulAPI has native write-capable endpoints, so exposing it on an untrusted network could bypass the VDR-Suite public-v1 authentication and permission authority, especially with root-running VDR. Confirm actual bind/network ACL and require least-exposed private-provider access (normally loopback-only where Suite and VDR are co-located), **without** relying on Suite authentication to protect the native plugin. The separate SuiteBridge SVDRP listener also needs loopback/ACL verification. Do not weaken VDR-Suite actor, grant or mutation gates in response.

The exact MLD `restfulapi` recipe `SRCREV`, patches and installed runtime response data have **not** been obtained. All MLD-specific RESTfulAPI compatibility remains OPEN.

## Exact questions for the MLD maintainer

1. Which immutable `mld-dev` and layer commits, `kas` manifest, machine/CPU architectures, Yocto/OE release, libc, init system and `PACKAGE_CLASSES` define the MLD **6.6** image to target? Which package-feed channel and update policy?
2. Where are the actual VDR, `restfulapi` 0.2.6.6, and experimental `suitebridge` recipes hosted (full GitLab links and commit refs)? What is each recipe's `SRCREV` and patch set? Which layer accepts the Suite daemon/Agent recipes?
3. What do `pkg-config --modversion vdr`, `pkg-config --variable=apiversion vdr`, `pkg-config --variable=libdir vdr` yield **in the MLD build sysroot**? What is the loaded `libvdr-suitebridge.so.*` file path, recipe revision and compiler/libc variant?
4. What is the MLD SVDRP listener binding/port/access control, and what is the distinction between `svdrpsend.sh` and `svdrpsend` on that image? Please share the *redacted, read-only* `PLUG suitebridge CAPS 1` output and `SNAP` schema/version (no secrets).
5. Can you share sanitized HTTP **read-only** status and representative JSON structures for `GET /info.json`, `/channels.json`, `/events/...json`, `/timers.json`, `/recordings.json`, `/change-state.json` from MLD RESTfulAPI 0.2.6.6? Specify response codes/content-type and whether timer inventory is **complete**; redact paths/tokens/user data.
6. Which **effective** RESTfulAPI bind address/port and firewall/ACL protect the HTTP listener and native write endpoints on MLD? Upstream 0.2.6.6 advertises `--ip=0.0.0.0` in help and implements POST/PUT/DELETE Timer paths. Is the service restricted to the trusted local Suite backend, independently of Suite API authentication?
7. Which init manager/service unit naming, VDR runtime UID/GID, allowed non-root SVDRP clients, state/data/cache roots, service-account provisioning, writable `/run` paths, and tmpfiles policies apply? Are `/var/lib/vdr/channels.conf` and `/run/vdr` valid on this target, or different?
8. Are C++17 compiler, SQLite3 development/provider libraries, libcrypt, libcrypto, FFmpeg/ffprobe and TLS/CA certificates available in the chosen target feed? Which are built-in versus optional?
9. What is the recommended recipe split and regular update/feed/ABI rebuild policy? Can we stage `make install-runtime DESTDIR=${D} PREFIX=/usr` and install the plugin separately from `vdr-plugin-suite-bridge`?

Request **no** credential files, session tokens, database backups, unrestricted dumps or live mutations.

## MLD.1B starting contract

1. Work in an **isolated MLD dev/build environment**, never on running yaVDR.
2. Agree the upstream-owned Yocto layer and pinned `SRCREV` before creating final `.bb` files. Preserve one Suite codebase.
3. Cross-compile daemon and Agent using the sysroot-aware `mk/common.mk` toolchain variables; build SuiteBridge separately using the exact target `vdr.pc`.
4. Stage runtime/web via `mk/install.mk` with `DESTDIR`, stage plugin via its own `install-lib`. Compare real staged manifest and ELF dependencies against MLD package ownership. Never use `stage-install-runtime`'s default host `/tmp` path inside an OE recipe without explicitly controlled build directories.
5. Define native service units/init hooks with explicit least-privilege users, persistent data and secret policy; do not reuse the Ubuntu Agent `User=vdr` or implicit-root daemon service blindly.
6. Only then build one target image, check VDR ABI/linkage and run read-only capability/RESTfulAPI tests. Controlled mutations, clean upgrade and public API tests remain MLD.1C/D gates.

## Validation record

- Read-only GitHub inspection of the named source files and branch/ref confirms the source contracts above.
- MLD official published build README confirms the build family but its actual layer recipes could **not** be retrieved through available public readers in this round.
- No MLD 6.6 build, CI cross-compile, target-host command, permission change, timer operation or live service installation has been executed in this inventory.
