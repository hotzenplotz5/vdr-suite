# MLD.1A — Pit's initial MLD 6.6 runtime evidence (2026-10-19)

**Evidence class: MAINTAINER-REPORTED, not independently executed by VDR-Suite.**
Source: Pit's response in the [existing VDR-Portal MLD thread](https://www.vdr-portal.de/forum/thread/137533-vdr-suite-in-mld-verf%C3%BCgbar-machen/), with the outputs copied into the project conversation on 2026-10-20. Observations were taken on 2026-10-19 at about 20:25 UTC according to the HTTP Date header.

## MLD integration locations (reported)

- [SuiteBridge recipe directory](https://gitlab.com/MLD-6/meta-mld/-/tree/wrynose-ng/meta/recipes-vdr/vdr-plugin-suitebridge?ref_type=heads)
- [VDR-Suite recipe directory](https://gitlab.com/MLD-6/meta-mld/-/tree/wrynose-ng/meta/recipes-vdr/vdr-suite?ref_type=heads)
- GitLab branch: `wrynose-ng`.
- **Access limitation:** GitLab directory/raw/API content could not be retrieved with the available read-only web access in this round. Directory links were supplied by the maintainer, but their recipe contents, SRCREV, LICENSE/LIC_FILES_CHKSUM, package splits, build flags, service policy and dependency metadata remain **UNINSPECTED**; do not mark recipes accepted.

**Important source branch discrepancy:** Pit reports switching a recipe to GitHub branch `work/mld66-integration`. At verification on 2026-10-20 the authorized existing GitHub branch is **`work/mld66-platform-integration`**; `work/mld66-integration` returned ref 404. Maintainer should confirm the exact `SRC_URI`/`branch=`/`SRCREV` in the existing recipe and use the authorized branch, with an immutable reproducible source revision. Do not create another branch simply to match the typo.

The maintainer calls the in-progress package version `VDR-Suite-MLD.69` to signal the Phase-69 baseline. The actual in-tree `VERSION` is `0.14.1` at this branch. **Phase number is not package/source release identity.** Agree on Yocto `PV`/`PR` conventions and source SHA provenance before publication; do not describe an unverified package as a released Phase-69 runtime. No MLD fork is needed as a permanent product source.

## Actual reported target runtime

| Input | Observed via maintainer | Confidence |
| --- | --- | --- |
| VDR | `2.8.3` from `GET /info.json` | MAINTAINER-REPORTED runtime |
| VDR API | `apiversion=14` asserted by maintainer | MAINTAINER-REPORTED; `vdr.pc` + ELF filename not yet inspected |
| RESTfulAPI | `0.2.6.6` loaded | MAINTAINER-REPORTED runtime |
| SuiteBridge | `0.14.1` listed as loaded, prior `CAPS 1` output reported | MAINTAINER-REPORTED runtime; contract/provenance still OPEN |
| TVScraper | `1.2.16` loaded | MAINTAINER-REPORTED; optional functionality not verified |
| `GET http://127.0.0.1:8002/info.json` | HTTP 200; 3106-byte JSON; headers include `Access-Control-Allow-Origin: *` and methods POST/GET/DELETE/PUT | MAINTAINER-REPORTED; no proof of network-only loopback binding or native auth |
| `GET http://127.0.0.1:8002/change-state.json` | HTTP 404; empty payload | **OBSERVED INCOMPATIBILITY** with Suite adapter `getChangeState()` expectation |
| `GET http://127.0.0.1:8002/recordings.json` | curl timeout at 5.002 seconds without response bytes | **OBSERVED SLOW/UNAVAILABLE READ** at 5-second bound; precise cause unproven |

## Source comparison and technical consequences

1. The current public `yavdr/vdr-plugin-restfulapi` `master` (examined on 2026-10-20, commit `79f9bd26370a08d6c5186c19cefa300c569045ea`) declares version `0.2.6.6` in `restfulapi.cpp` and registers `changeState` in `serverthread.cpp`. Its `changestate.cpp` serializes `bootID`, `channelsUpdate`, `recordingsUpdate`, `timersUpdate`, `eventsUpdate`. The reported MLD `/info.json` `services` collection omits `/change-state` while the actual endpoint returns 404. This **strongly motivates an exact MLD source/recipe/patch/binary review**, not a guessed Suite fallback or an assumption that version 0.2.6.6 guarantees the feature.
2. The Suite adapter `core/vdr/src/RestfulApiVdrAdapter.cpp` requests `/change-state.json` and returns a default/empty change-state object on non-200. Thus this read does not establish valid change counters; freshness detection or polling paths should be assessed, and it must **not** be reported as full compatibility. The `/recordings.json` read is also required for normalized recordings. Neither REST failure should disable Suite authentication, allow blind native operations, or mark inventory complete.
3. Upstream RESTfulAPI `recordings.cpp` `showRecordings()` supports `start` and `limit` query filters, but **iterates the entire recording list under a VDR read lock** even for a limited response; `JsonRecordingList::addRecording()` may include media and metadata work. A 5-second timeout could result from large catalog size, blocking/locks, I/O or slow serialization. **No root cause is demonstrated.** A bounded, read-only count/timing comparison on a test host can distinguish the main hypotheses without fetching or publishing recording names.
4. Upstream private RESTfulAPI implements its own POST/PUT/DELETE routes; wildcard CORS response headers are **not authorization**. Binding `curl` to 127.0.0.1 is **not proof** that the *listener* is restricted to loopback. A read-only listener inventory and effective binding policy are required before connecting it as a trusted private provider to an internet-facing Suite API. Root-running VDR is an additional risk, not a new Suite principal.
5. Suite's `core/vdr/src/RestfulApiNativeTimerInventoryReader.cpp` reads `GET /timers.json`, validates complete parsed native timer IDs, and fails closed on non-200/invalid/oversized responses. `/info.json` advertising `/timers` is not a complete-inventory or Timer CREATE acceptance.
6. `SuiteBridge 0.14.1` and `VDR 2.8.3` are **runtime version evidence**, not installed VDR plugin ABI proof. A compatible `libvdr-suitebridge.so.14` naming expectation follows `vdr.pc` + the VDR plugin Makefile, but installed library path and real toolchain ABI still require MLD artifact evidence.

## Narrow maintainer follow-up (all read-only)

1. Please check/correct recipe `SRC_URI`/branch/`SRCREV` to reference **`work/mld66-platform-integration`**. Share the recipe text (or a publicly readable raw link) and immutable MLD `meta-mld` revision so we can audit actual BitBake metadata. Which part currently uses the separate `VDR-Suite-MLD.69` identifier: recipe `PV`, `PR` or package label?
2. Why is `/change-state` omitted from the runtime `/info.json` service list and why does `/change-state.json` return 404 although current upstream `serverthread.cpp` registers it? Please share the exact RESTfulAPI `SRCREV`, patches and installed binary provenance. No restart or patching is necessary for initial analysis.
3. Can `GET /recordings.json?start=0&limit=1` finish within the same bounded timeout? Please share only **HTTP status, elapsed time, total bytes or timeout, and related sanitized VDR log entries**, not recording filenames or response bodies. If it still hangs, please capture whether VDR remains responsive, without modifying the service.
4. Could you also provide similarly summarized HTTP statuses/timings for `/channels.json?start=0&limit=1` and `/timers.json`? The latter should be a *complete response* to support future native timer inventory verification (no mutation).
5. What listener interface/port/ACL exposes RESTfulAPI and SVDRP to local non-root processes? Which MLD service users and persistent state/config directories should the native Suite daemon and Agent use? Confirm `vdr.pc` apiversion, VDR ABI package/so name and the selected machine/toolchain.

These observations do **not** authorize live MLD or yaVDR changes. Do not ask for credentials, recordings, timer contents, database files or unrestricted logs.

## Milestone accounting

MLD.1A now has target runtime evidence from the maintainer, but two known RESTfulAPI read-path failures and missing recipe reproducibility data. MLD.1B package recipes have been **reported as present** in MLD GitLab; they have **not** been audited, built or accepted by VDR-Suite. Maintain a shared single-source codebase and do not merge MLD integration into `main` without separate authorization.
