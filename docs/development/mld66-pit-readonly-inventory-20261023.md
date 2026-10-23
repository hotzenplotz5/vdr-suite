# MLD.1A — Pit's read-only target inventory, 2026-10-23

**Evidence:** verbatim values reported by Pit in the existing [MLD VDR-Portal collaboration thread](https://www.vdr-portal.de/forum/thread/137533-vdr-suite-in-mld-verf%C3%BCgbar-machen/). **MAINTAINER-REPORTED**, not an independent VDR-Suite runtime acceptance. Source tool: [`tools/mld66-readonly-inventory.sh`](../../tools/mld66-readonly-inventory.sh) with `--caps`, plus `ss -lnt` and a local curl request.

## Confirmed reported platform characteristics

| Observation | Target result | Interpretation / release gate |
| --- | --- | --- |
| Architecture and platform | `x86_64`; `ID=mld`; `VERSION_ID=6.6.0`; kernel `6.18.24-yocto-standard` | Real MLD 6.6 target evidence. Exact BitBake MACHINE/sysroot remains unknown |
| VDR | Binary present, running as `root`; SVDRP greeting VDR `2.8.3` | Root-running VDR confirmed; separate Suite daemon/Agent must **not** inherit root identity |
| SuiteBridge | `CAPS 1` returns SVDRP `900` JSON: `plugin_version=0.14.1`, `discovery_schema=1`, `capability_schema=1`, `snapshot_schema=4`, `local_contract_schema=4` | Discovery transport responds; not proof that every advertised read operation works |
| Advertised SuiteBridge capabilities | `lifecycle`, `status-events`, `snapshots`, `local-contract`, `recording-metadata`, `recording-marks`, `recording-cut-state`, `epg-type-snapshot`, `vdr.live.stream`, `osd.view`, `osd.control`: `available` | Read and media capabilities are **advertised**, not functionally accepted |
| Native mutation | `mutations: disabled` | Safe conservative runtime baseline; do **not** treat as an error or enable native writes before independent authorization/readback acceptance |
| Commands installed | `vdr`, `svdrpsend`, `svdrpsend.sh`, `systemctl`, `apt` present; `pkg-config`, `opkg`, `rpm`, `ffmpeg`, `ffprobe` absent | Runtime host does not need a development `pkg-config`; FFmpeg and ffprobe packaging is open for media functions |
| VDR plugin ABI | `VDR_PKGCONFIG=UNAVAILABLE_ON_HOST`, `VDR_ABI=REQUIRES_BUILD_SYSROOT_EVIDENCE` | Obtain actual `vdr.pc`/`apiversion` and installed plugin SONAME from the Yocto build sysroot, not guessed from version |
| Suite HTTP port `18080` | No listener reported | Expected if the Suite daemon has **not yet been installed**; not a defect in the Suite daemon |
| RESTfulAPI HTTP `8002` | `LISTEN 0.0.0.0:8002` | **Network policy/security gate OPEN:** bound to every IPv4 interface; network reachability may still depend on firewall. Private RESTfulAPI includes write routes and must not be treated as an authenticated Suite API |
| SVDRP `6419` | `LISTEN 0.0.0.0:6419` | **Network ACL review OPEN:** VDR SVDRP access must be checked against `svdrphosts.conf`/network filters; listener alone does not imply remote command authorization |
| RESTfulAPI recordings | Initial 5s timeout, later **30.002754s with 0 bytes / HTTP 000** | **Open functional blocker** for Suite's recording provider; `/info.json` independently returned 200 in 0.001798s |
| RESTfulAPI change-state | Earlier `GET /change-state.json` returned 404; no post-update probe reported | Remains **UNVERIFIED after plugin update**, not automatically fixed by same-version newer git |
| RESTfulAPI exact revision | Declared recipe `SRCREV=79f9bd26370a08d6c5186c19cefa300c569045ea`, source `yavdr/vdr-plugin-restfulapi` `master` | Source revision identified; effective binary/patch provenance still open |

## Source-grounded constraints

- `yavdr/vdr-plugin-restfulapi` upstream `restfulapi.cpp` documents `-i/--ip` to choose its listen address. For VDR-Suite's **private provider**, default target should be loopback (`127.0.0.1:8002`) when it does not need direct LAN clients; otherwise an explicit, audited network isolation policy is necessary. Wildcard CORS headers do not enforce authentication.
- VDR's upstream INSTALL manual warns that SVDRP network exposure can be hazardous and refers to `svdrphosts.conf` allowlisting. Its listening socket can be `0.0.0.0` while access is controlled separately. Do not claim `ss` alone proves either secure or vulnerable remote access.
- `yavdr/vdr-plugin-restfulapi` upstream `recordings.cpp` `showRecordings()` uses a VDR recordings read lock and loops across the recording collection even when `?start=0&limit=1` is used. `JsonRecordingList::addRecording()` can call `Scraper2VdrService::getMedia(recording,...)` and access the filesystem, read recording length and serialize JSON. `scraper2vdr.cpp` can delegate to installed `tvscraper`. **These are concrete code paths to investigate, not a proven root cause** of Pit's timeout; do not disable plugins or change production just on suspicion.
- `CAPS 1` successful with `mutations disabled` establishes discovery only. The Suite's device/actor/grant and native mutation gates must stay fail-closed. No MLD public API or full runtime installation has been tested.

## Safe MLD maintainer next checks (no service/config modifications)

1. Capture a single bounded, read-only request for `/change-state.json` after the plugin update: whether 200 or 404, but do not share private content.
2. Compare one bounded `GET /recordings.json?start=0&limit=1` with the previously reported full-list timeout; report HTTP code, curl total time, number of response bytes or timeout, and whether the VDR remains responsive. A limited response may still scan all recordings; avoid repeated slow concurrent tests.
3. Check `/timers.json` and `/channels.json` once, measuring status and time but **not publishing channel/timer/recording data**.
4. Report effective RESTfulAPI plugin listen options (`--ip`), whether access to `8002` from LAN is required, and permitted SVDRP clients from sanitized ACL configuration; do not paste secrets or unrestricted config. Agree on non-root daemon/Agent ownership.
5. Supply current MLD RESTfulAPI git `SRCREV` and recipe patches, native Suite recipe metadata, and VDR plugin ABI from the build sysroot. No ad hoc chmod/chown, service restart, mutation, or test host installation requested.

## Later maintainer report on 2026-10-23

Pit subsequently confirmed read-only `PLUG suitebridge VPATH 1` on VDR 2.8.3, receiving `900 /data/tv`. He also reported another five-second `/recordings.json` timeout and recurring VDR restarts with an unspecified dump during MLD package-rebuild work. These are **separate findings**; the new success does not resolve RESTfulAPI or process stability. See [VPATH/HTTP/crash evidence](mld66-pit-vpath-http-crash-evidence-20261023.md) and the [bounded, opt-in probe](../../tools/mld66-readonly-http-probe.sh). Do not benchmark recordings while VDR is unstable.

## RESTfulAPI recipe identity and 30-second result (Pit, 2026-10-23)

Pit now confirms RESTfulAPI `SRC_URI` points to `yavdr/vdr-plugin-restfulapi` `master` and `SRCREV=79f9bd26370a08d6c5186c19cefa300c569045ea`. A full `/recordings.json` timed out after **30.002754 seconds** with HTTP 000 and 0 response bytes, while `/info.json` returned HTTP 200 in **0.001798 seconds**. The exact *declared* source revision is now known; final installed ELF/patch provenance is still unverified. A source audit at this revision identifies a recording-read lock covering list traversal and per-recording JSON metadata / filesystem / scraper processing, not yet an established hang cause. When VDR is stable, the opt-in `--recordings-html-sample` mode can distinguish shared VDR list/lock latency from JSON enrichment. See [detailed analysis](mld66-pit-vpath-http-crash-evidence-20261023.md).

## Later HTML timing and installed-package crash boundary (2026-10-23)

Pit ran `recordings.html?start=0&limit=1` twice: HTTP 200 in **0.012861s** and **0.001116s**. This is one HTML item and is **not directly comparable** to the timed-out *full* `recordings.json`. The next controlled measurement is a *single* 10-second bounded `recordings.json?start=0&limit=1` request, if the host remains stable. Pit reports VDR restarts only with **VDR-Suite installed**; the SuiteBridge-only baseline has not displayed those restarts. Determine *install vs service-start vs running workload* and examine the first failure safely before assigning a cause. [Detailed analysis](mld66-pit-vpath-http-crash-evidence-20261023.md).

## Single-item JSON failure and source-confirmed conditional defect

New maintainer measurement: `/recordings.json?start=0&limit=1` **HTTP 000 / curl 28 / 10.002424s / 0 bytes**, directly comparable to the fast one-item HTML result. The pinned RESTfulAPI source contains a conditional endless loop in `cleanImagePath()` → `StringExtension::replace()` if its EPG image path is empty and TVScraper media calls reach that code. An absent default `/var/cache/vdr/epgimages` can leave `Settings::epgimage_dir` empty; the **effective** `eimgs:` configuration on Pit's MLD host has not been checked. Do **not** claim the root cause is confirmed or that this alone causes his ~4-minute VDR restarts after installing full Suite. Read-only proof and proposed vendor patch: [detail](mld66-pit-vpath-http-crash-evidence-20261023.md), [unapplied patch](../../packaging/mld66/patches/0001-restfulapi-guard-empty-epg-image-root.patch).

## Acceptance status

**MLD.1A** runtime inventory and SuiteBridge read-only `CAPS` and `VPATH 1` responses: **reported PASS**. **MLD.1B** standalone native Suite build/install: **NOT TESTED**. RESTfulAPI recording reads: **FAIL at 30s**. `/change-state` after update: **OPEN**. Native write acceptance, FFmpeg-backed features, network isolation and independent non-root Suite services: **OPEN**.

Keep the user's productive yaVDR server untouched and maintain the authorized MLD branch in isolation.
