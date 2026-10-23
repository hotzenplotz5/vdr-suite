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

# Explicitly opt in to exactly one limited recording request, max 30 seconds:
sh tools/mld66-readonly-http-probe.sh --recordings-sample
```

The script makes only fixed GET requests to `127.0.0.1:8002`, ignores curl configuration/proxies, follows no redirects, discards all response bodies, and prints only HTTP status, elapsed seconds, transferred bytes and curl exit status. No retry and no full recording-inventory request are built in. Even `?start=0&limit=1` may scan all VDR recordings internally; **do not run repeatedly or during VDR instability**. The post-update `/change-state.json` result remains unknown.

Possible causes include slow serialization, TVScraper/metadata work, storage I/O or VDR locking; none is proven by the five-second timeout.

## VDR instability — independent release blocker

Pit additionally reports VDR restarting about every four minutes, with a Linux kernel dump, after changes to the MLD package assembly. The exact crash class is unknown: a kernel oops, userspace core dump, systemd restart and watchdog action need distinct diagnoses. No causal responsibility can yet be assigned to SuiteBridge, RESTfulAPI, VDR or a recipe.

Next evidence should be limited to the **sanitized** first fault, timestamp, process/service exit reason, relevant stack trace if available, and immutable package/recipe revisions. Avoid unrestricted logs, credentials, service restarts or write operations on the live host. Pause optional recording HTTP benchmarking until VDR runs reliably.

## Release gates

- **Reported PASS:** read-only `VPATH 1` returned `900 /data/tv` on MLD VDR 2.8.3.
- **OPEN:** recordings read beyond five seconds; post-update change-state result; effective RESTfulAPI build provenance.
- **BLOCKED:** native MLD Suite package/functional acceptance while periodic VDR restarts persist.
- **UNCHANGED:** Ubuntu/yaVDR defaults, explicit mutation gates, MLD package `SRCREV`, Draft PR #525, and `main`.
