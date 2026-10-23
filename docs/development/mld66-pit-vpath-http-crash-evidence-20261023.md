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
