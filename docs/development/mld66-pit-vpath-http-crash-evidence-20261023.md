# MLD 6.6 — Pit's VPATH result, recording timeout and VDR restarts (2026-10-23)

**Status: maintainer-reported evidence, not independent package/runtime acceptance.** The reports may reflect different sessions or package states; the relation of the 40-minute quiet period and four-minute restart pattern is not established.

## Verified on Pit's MLD host (as reported)

```text
svdrpsend.sh plug suitebridge VPATH 1
220 VDRSuedHD SVDRP VideoDiskRecorder 2.8.3; Fri Oct 23 19:25:21 2026; UTF-8
900 /data/tv
221 VDRSuedHD closing connection
```

The VDR-native **read-only VPATH 1** command succeeds on MLD VDR 2.8.3. `/data/tv` is Pit's user-chosen video mount/share, **not** an MLD system default. Pit also reports quieter logs and roughly 40 minutes without erroneous entries. This establishes the SVDRP VPATH command, **not** automatic daemon root discovery, a complete Suite package installation, native write permission, or long-term VDR process stability.

## 21:10 CEST update — exact RESTfulAPI revision and 30-second failure

Pit has now independently reported:

- `GET /recordings.json`: curl exit 28, `HTTP 000`, **30.002754 seconds with 0 response bytes** (`--max-time 30`).
- `GET /info.json`: `HTTP 200`, **0.001798 seconds**.
- Exact MLD RESTfulAPI recipe: `SRC_URI = "git://github.com/yavdr/vdr-plugin-restfulapi.git;protocol=https;branch=master"`, `SRCREV = "79f9bd26370a08d6c5186c19cefa300c569045ea"`. The source identity is no longer unknown; the installed ELF SHA/patches remain unverified.

**Source inspection at this immutable revision:** `recordings.cpp` `showRecordings()` holds `LOCK_RECORDINGS_READ` through list traversal. `JsonRecordingList::addRecording()` performs `Scraper2VdrService::getMedia(recording, ...)`, `stat()`, `recording->NumFrames()` (through `VdrExtension::RecordingLengthInSeconds()`), and `recording->FileSizeMB()`. `JsonRecordingList::finish()` serializes the assembled vector, so 0 client bytes at 30 seconds does **not** prove that processing never started. The VDR lock/list traversal, JSON metadata enrichment, filesystem and scraper service may be involved, but no individual call is yet proved to block.

**Focused next comparison, only on a stable test host:** `--recordings-html-sample` sends exactly one `GET /recordings.html?start=0&limit=1` (10s); unlike JSON, the HTML list only prints a recording name and skips metadata enrichment. Then, separately and only if safe, the available `--recordings-sample` sends one **10s** limited JSON request. Both formats still touch the shared VDR recording lock/list; the limit does **not** avoid traversal.

- HTML fast, JSON slow: focus on JSON per-recording scraper, index/file-size and serializing paths.
- Both slow: focus on recording lock acquisition, shared listing and underlying VDR/provider response flow.
- Both fast, full JSON slow: focus on per-recording scaling (catalog size, metadata work or IO).
- Results inconclusive if the VDR restarts, the cache/recording state changes, or the request times out for another reason.

No change to Suite timeout, no plugin reload, no selective disabling of TVScraper and no native write commands are warranted from these measurements alone.

[Exact pinned source](https://github.com/yavdr/vdr-plugin-restfulapi/tree/79f9bd26370a08d6c5186c19cefa300c569045ea).

## 22:14 CEST update — HTML success and installation-scoped VDR restarts

Pit now reports two successful `GET /recordings.html?start=0&limit=1` calls: HTTP 200 in **0.012861s** and **0.001116s**. These are requests for *one* recording; the timed-out JSON request was an **unlimited full-list** request. The difference is therefore **not yet controlled for pagination**: this result proves the HTML single-item read can succeed quickly, but does not prove that TVScraper, JSON serialization, filesystem metadata or the full catalog is the bottleneck.

**Next discriminating test, only while the VDR is stable:** make *one* read-only `GET /recordings.json?start=0&limit=1` with a **10-second** deadline (or run `sh tools/mld66-readonly-http-probe.sh --recordings-sample`). Response body discarded; no repeat/full-list fallback.

- Limited JSON fast: suspect full-catalog scaling/work, compare catalog size/metadata on the source side before any more host tests.
- Limited JSON slow: focus on **per-recording JSON enrichment** (TVScraper's service, `stat()`, index/length/size, serialization); no individual cause proven.
- The very fast HTML result makes a permanently blocked VDR recording-list lock less likely *at the moment of those tests*, but does not rule out intermittent contention.

Pit additionally clarifies that the reported VDR restarts occur **only when he installs VDR-Suite**. Currently **only SuiteBridge is enabled**, and he reports this baseline without those restarts. Do not equate this with proof that the Suite daemon itself crashes VDR: possible origins include package install/postinst hooks, systemd service actions/dependencies, process interaction, or a VDR crash after the extra components start. We still lack **the exact timing relative to package installation versus service startup**, an exit reason, and the first relevant log/core trace.

Ask Pit for the first redacted VDR failure around Suite installation and whether restarts happen **during installation only or periodically afterward**; do not request another install on the production system, do not advise disabling the known-good SuiteBridge plugin, and do not infer a patchable culprit.

## Later evidence — single-item JSON failure and reproducible empty-path defect

Pit measured `GET /recordings.json?start=0&limit=1`: **curl 28, HTTP 000, 0 bytes after 10.002424s**. This is now directly comparable to one-item HTML (HTTP 200 in 0.012861s and 0.001116s). Thus the blocker is specific to additional JSON record processing or serialization, **not** just unbounded catalog size.

### Source-confirmed unbounded loop candidate

The exact pinned RESTfulAPI source `79f9bd26370a08d6c5186c19cefa300c569045ea` has a concrete indefinite-loop defect:

1. `tools.h` defaults `EPG_IMAGE_DIR` to `/var/cache/vdr/epgimages` unless overridden at build time.
2. `Settings::initDefault()` invokes `SetEpgImageDirectory(EPG_IMAGE_DIR)`. `SetEpgImageDirectory()` sets `epgimage_dir` only if `stat()` of the path succeeds. If it fails and no valid `--epgimages` override is provided, this std::string remains empty.
3. `JsonRecordingList::addRecording()` invokes TVScraper using `sc.getMedia(recording, am)` when it is installed and service data is available. The media service calls `Scraper2VdrService::cleanImagePath()` on returned image paths.
4. `cleanImagePath()` invokes `StringExtension::replace(path, epgImagesDir, "")` unconditionally. With `epgImagesDir == ""`, its utility repeatedly finds the empty substring at position 0, replaces it with `""`, then increments the position by zero: **infinite loop**.

This is a **proved source defect conditional on an empty configured EPG image root and the media call reaching `cleanImagePath()`**, NOT proof that Pit's effective EPG image directory is empty. It also does not prove the installed 4-minute crash is caused by this request, though the scenario is worth investigating because the Suite daemon consumes `/recordings.json` and may exercise this RESTfulAPI code path.

Proposed minimal vendor fix (not applied to MLD recipe or upstream):
[`packaging/mld66/patches/0001-restfulapi-guard-empty-epg-image-root.patch`](../../packaging/mld66/patches/0001-restfulapi-guard-empty-epg-image-root.patch). Add both an empty-prefix guard in `StringExtension::replace()` and a fail-closed empty-root guard in `cleanImagePath()`. The exact seven-line patch was checked against isolated excerpts of the immutable source, and equivalent C++17 guards passed four self-contained assertions. **Full RESTfulAPI compile, native MLD package test and target VDR runtime acceptance remain OPEN.**

### Only requested evidence from Pit (read-only)

```sh
journalctl -b --no-pager | grep -E 'RESTful-API Settings:|restfulapi: Used settings:' | tail -n 5
ls -ld /var/cache/vdr/epgimages
```

The first line reveals RESTfulAPI's actually configured `eimgs:` value; the second checks the known upstream default directory, which might be different in an MLD package. No further timed-out JSON curl, directory creation, symlink manipulation, plugin disabling, VDR restart or package reinstall is needed for this check. If `eimgs:` is nonempty, this particular empty-string loop does **not** explain Pit's failure and a thread stack trace or narrowly instrumented test build would be needed. If `eimgs:` is empty, fix should be validated in isolated MLD build and compared with stable runtime.

### Four-minute VDR restarts

Pit clarified that the VDR starts restarting about four minutes **after VDR-Suite installation**, while **SuiteBridge-only** runs are stable. The daemon can cause private `/recordings.json` calls; if such a request drives a loop inside a VDR plugin, it could indirectly contribute to instability, but **we have no correlation of the crash stack/timing to those calls yet**. A userspace core dump logged by kernel is not necessarily a kernel panic. Capture sanitized `journalctl -b -u vdr -u vdr-suite-daemon --no-pager` around the first fault and any available user-space core backtrace before assigning root cause. Do not re-install the full suite on Pit's stable host merely to reproduce.

## 2026-10-23 late evening — empty EPG image root independently confirmed by Pit

Pit reports *at runtime* four RESTfulAPI startup lines, from distinct VDR process instances:

```text
restfulapi: Used settings: port: 8002, ip: 0.0.0.0, eimgs: , cimgs: , webapp: , headers: activated
```

Likewise `RESTful-API Settings` lines show **`eimgs:` empty**, not merely an absent guessed pathname. `ls -ld /var/cache/vdr/epgimages` returns **No such file or directory**. The reported `/var/cache/vdr` contains `cam.data`, `epg.data` and `plugins/`; plugin cache contains `restfulapi/`, `skindesigner/`, `tvscraper/`. The MLD TVScraper-specific cache is not the RESTfulAPI EPG artwork root and must not be conflated with it.

This **confirms the missing setting, the prerequisite for the source-confirmed indefinite loop**, on the affected host. The condition that a recording's TVScraper media reaches `cleanImagePath()` is still to be established by a call trace, and an on-device patched RESTfulAPI JSON PASS has **not** been obtained. No missing generic JSON runtime or mandatory EPG-image plugin has been established. There is no reason to install arbitrary plugins or create fake cache directories as the permanent solution.

**Fix approach:** use the staged [source patch candidate](../../packaging/mld66/patches/0001-restfulapi-guard-empty-epg-image-root.patch) to make empty EPG artwork configuration safe. Validate its application against the exact upstream commit and then native MLD package build / runtime separately. The source patch is not yet deployed. Keep the official MLD recipe `SRCREV` unchanged until a verified patch-delivery strategy is agreed.

**Crash correlation remains OPEN.** Pit says repeated VDR restarts arise ~4 minutes after full Suite installation, while SuiteBridge-only is stable. A daemon-triggered RESTfulAPI recording request may exercise the proven loop, but no watchdog/core/kernel signature has been supplied that proves it causes the restarts. A log line in kernel facilities is not itself proof of a kernel panic. Require the first original fault/watcher reason before assigning root cause or claiming stability.

## 2026-10-24 — TVScraper maintainer confirms `GetEnvironment` ownership contract

MarkusE points out the public scraper plugin service `GetEnvironment`: its `cEnvironment` structure has `basePath`, `seriesPath` and `moviesPath`; both TVScraper and scraper2vdr are intended providers. Independently inspected actual [TVScraper implementation](https://github.com/MarkusEh/vdr-plugin-tvscraper/blob/master/tvscraper.c): on `GetEnvironment`, the plugin sets `basePath = config.GetBaseDir()`, `seriesPath = config.GetBaseDirSeries()` and `moviesPath = config.GetBaseDirMovies()`. TVScraper defaults its base path to `cPlugin::CacheDirectory(PLUGIN_NAME_I18N)` if not configured via `--dir`, **not** an assumed fixed path. `config.c` creates subdirectories under the configured base directory.

**Distinguish two ownership domains:**

- `Settings::EpgImageDirectory()` / the RESTfulAPI default `/var/cache/vdr/epgimages`: legacy external EPG image source, not a mandatory TVScraper output path.
- The scraper's dynamically reported `GetEnvironment.basePath` and children: the correct authoritative image root for media returned by that plugin, including nondefault `--dir`.

The pinned RESTfulAPI revision `79f9bd2` does **not** contain a `cEnvironment` definition in its bundled `scraper2vdr/services.h`, does **not** call `GetEnvironment`, and incorrectly uses the legacy EPG image root both to normalize scraper-returned paths in `Scraper2VdrService::cleanImagePath()` and to serve `/scraper/image/` in `ScraperImageResponder::reply()`. Therefore **the staged empty-root patch fixes the infinite-loop precondition but cannot on its own guarantee correct poster/fanart URLs or image delivery**. In particular, silently reinterpreting TVScraper's base as the external EPG image directory is not safe/compatible.

**Separate follow-up (not yet implemented or tested):** update RESTfulAPI's scraper-provider adapter to use `GetEnvironment` where available; normalize image paths relative to that provider root and serve only paths confined within that root (never arbitrary absolute paths or `..` traversal); preserve separately configured legacy EPG images and support absence of either source without hanging. Account for absence of `GetEnvironment` on older plugins without assuming one universal location. Validate actual TVScraper URL and image behavior, not merely JSON HTTP 200, in a native MLD build. Keep the source-level empty-path guard. Do **not** make EPG image directory creation or symlinks the mandatory fix, and do not force a TVScraper-specific default on Ubuntu.

## 2026-10-24 — distribution-neutral RESTfulAPI fix prepared in plugin fork

A dedicated [Draft PR #7 in `hotzenplotz5/vdr-plugin-restfulapi`](https://github.com/hotzenplotz5/vdr-plugin-restfulapi/pull/7) now implements the full **provider-aware** fix on its own branch `fix/scraper-environment-image-paths`, based on `vdr-suite-integration`. It addresses both the empty-EPG-root infinite-loop and correct TVScraper `GetEnvironment.basePath` integration for recording JSON artwork metadata **and** GET `/scraper/image/` resolution. File reads and private resize writes are confined to their corresponding roots; standalone C++17 path-helper checks passed locally, including a real escaping symlink test. Full plugin build and MLD/yaVDR runtime image acceptance remain unverified because VDR/cxxtools headers are not present in the testing environment.

**Critical integration boundary:** This plugin fork branch contains many RESTfulAPI Suite-specific APIs and **must not be used as an automatic replacement** for Pit's existing `SRCREV=79f9bd26370a08d6c5186c19cefa300c569045ea`. Keep the separate minimal vendor patch on the MLD branch available for Pit's isolated test. A full provider-aware change for MLD requires a separate minimal cherry-pick/port, native BitBake recipe inclusion, and acceptance. Do not claim or attempt unattended VDR restarts/deployments. The four-minute restart remains a separate unproven issue.

## Independent recording-read blocker

```text
curl -i --max-time 5 http://127.0.0.1:8002/recordings.json
curl: (28) Operation timed out after 5002 milliseconds with 0 bytes received
```

Port 8002 serves RESTfulAPI, not SuiteBridge. A five-second timeout does not prove a deadlock. One prior yaVDR case took 10.774108 seconds to serve 1017 recordings (see [recording reconciliation evidence](recording-import-auto-reconciliation.md)). That is not a measurement of Pit's system.

A new **optional, read-only, bounded** probe is provided at [tools/mld66-readonly-http-probe.sh](../../tools/mld66-readonly-http-probe.sh):

```sh
# Only after the VDR restart problem has been resolved:
sh tools/mld66-readonly-http-probe.sh --metadata

# One lightweight HTML recording list sample, max 10 seconds:
sh tools/mld66-readonly-http-probe.sh --recordings-html-sample

# If still needed and safe: one limited JSON recording request, max 10 seconds:
sh tools/mld66-readonly-http-probe.sh --recordings-sample
```

The script makes only fixed GET requests to `127.0.0.1:8002`, ignores curl configuration/proxies, follows no redirects, discards all response bodies, and prints only HTTP status, elapsed seconds, transferred bytes and curl exit status. No retry and no full recording-inventory request are built in. Even `?start=0&limit=1` may scan all VDR recordings internally; **do not run repeatedly or during VDR instability**. The post-update `/change-state.json` result remains unknown.

Possible causes include slow serialization, TVScraper/metadata work, storage I/O or VDR locking; none is proven by the five-second timeout.

## VDR instability — independent release blocker

Pit additionally reports VDR restarting about every four minutes, with a Linux kernel dump, after changes to the MLD package assembly. The exact crash class is unknown: a kernel oops, userspace core dump, systemd restart and watchdog action need distinct diagnoses. No causal responsibility can yet be assigned to SuiteBridge, RESTfulAPI, VDR or a recipe.

Next evidence should be limited to the **sanitized** first fault, timestamp, process/service exit reason, relevant stack trace if available, and immutable package/recipe revisions. Avoid unrestricted logs, credentials, service restarts or write operations on the live host. Pause optional recording HTTP benchmarking until VDR runs reliably.

## Release gates

- **Reported PASS:** read-only `VPATH 1` returned `900 /data/tv` on MLD VDR 2.8.3.
- **Reported PASS:** single-item HTML recordings read twice in 0.012861s / 0.001116s.
- **OPEN:** one-item JSON versus full-list JSON, post-update change-state result, effective RESTfulAPI build provenance.
- **BLOCKED:** native MLD Suite package/functional acceptance: restarts are reported only with VDR-Suite installed, not with SuiteBridge alone; exact trigger unknown.
- **UNCHANGED:** Ubuntu/yaVDR defaults, explicit mutation gates, MLD package `SRCREV`, Draft PR #525, and `main`.
