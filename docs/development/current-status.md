# VDR-Suite Current Project Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Roadmap](../planning/roadmap.md)
- [Lazy Recording Loading](../planning/lazy-recording-loading.md)
- [Startup Snapshot Runtime Rule](startup-snapshot-runtime.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)

---

## Purpose

This document tracks the current verified technical state of VDR-Suite.

Implementation history belongs in [Completed Phases](completed-phases.md).

Future planning belongs in [Roadmap](../planning/roadmap.md).

---

## Project

VDR-Suite is a service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST APIs, Web UI, OSD integration and future integration of VDR-Rectools.

VDR remains the primary backend domain and source of truth.

---

## Current Branch

```text
main
```

---

<!-- PROJECT_PROGRESS_START -->
## Project Progress

Overall foundation progress, not product completion:

    ███████░░░ 73%

Milestone progress:

    Core Runtime Foundation              ██████████ 100%  completed
    Multi-Backend Foundation             ██████████ 100%  completed
    Query Foundation                     ██████████ 100%  completed
    Action Foundation                    ██████████ 100%  completed
    Metadata Foundation                  ██████████ 100%  completed
    Documentation Foundation             ██████████ 100%  completed
    SearchTimer Backend Foundation       ██████████ 100%  completed
    SearchTimer User Workflow            ██████████ 100%  completed
    SearchTimer Runtime Mutation Policy  ██████████ 100%  completed
    SearchTimer Preview EPG Performance  ████████░░  75%  in progress
    Real VDR Acceptance Foundation       ██████████ 100%  completed
    Runtime Lifecycle Hardening          ██████████ 100%  completed
    Lazy Recording Loading               █░░░░░░░░░  10%  in progress
    Live Plugin Parity Foundation        ██████████ 100%  completed
    Automation Foundation                ██████████ 100%  completed
    Recording Operations Safety          ░░░░░░░░░░   0%  planned
    Federation Foundation                ░░░░░░░░░░   0%  planned
    Frontend Foundation                  ░░░░░░░░░░   0%  planned

Current milestone:

    Phase 55.6 - Recording operations audit and safety policy

Progress source: [Project Progress](../planning/project-progress.md)
<!-- PROJECT_PROGRESS_END -->

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 55.5m - Documentation consolidation and roadmap alignment
```

Current documentation consolidation state:

```text
Phase 55.5m - Documentation consolidation and roadmap alignment
```

Next major implementation milestone:

```text
Phase 55.6 - Recording operations audit and safety policy
```

Required planned follow-up:

```text
Recording operations audit and safety policy
```

Completed foundations:

```text
Core Platform Foundation
VDR Backend Foundation
Multi-Backend Foundation
Snapshot Runtime Foundation
Change Feed Foundation
Live Transport Foundation
Capability Foundation
Recording Query Foundation
Recording Action Foundation
EPG Search Foundation
Content Classification Foundation
Person Metadata Foundation
Recording Person Search Foundation
Recording Character Search Foundation
SearchTimer User Workflow Foundation
SearchTimer Runtime Mutation Policy
SearchTimer Warm EPG Cache Architecture
SearchTimer Preview EPG Input Status Contract
SearchTimer Discovery RESTfulAPI Runtime Wiring
SearchTimer Discovery Shared Decoder Cleanup
Real VDR Acceptance Foundation
Daemon Runtime Lifecycle Hardening
Documentation Handoff Verification
```

Current foundation in progress:

```text
Recording Operations Audit and Safety Policy
```

Direct GitHub documentation synchronization should still be followed locally by functional or runtime-specific checks only when the change requires real local behaviour validation.

---

## Verification Summary

- Snapshot-based read architecture is completed for the current domain set.
- Backend registry, backend-aware snapshots and multi-backend snapshot summaries are implemented.
- Change feed and live transport foundations are implemented.
- Selective EPG REST APIs and the EPG search API are implemented.
- Recording query and recording action foundations are implemented.
- ADR-0028 documents the source-aware content classification architecture.
- Person metadata, recording-person search and recording-character search foundations are implemented.
- SearchTimer route, daemon backend provider wiring and backend contract documentation are implemented.
- Native fuzzy SearchTimer backend capability validation is complete through operator refresh, capability report and persisted restore.
- SearchTimer user workflow, validation, planning, guarded execution and readback verification foundations are implemented.
- Phase 53.x preserves title-only SearchTimer compare fields across REST, workflow planning and command dispatch.
- Phase 54.1 fixes SearchTimer preview comparison-option handling and verifies title-only preview behavior against live VDR EPG input.
- Phase 54.2 accepts ADR-0034 for warm EPG cache input, change-state invalidation and future SSE-triggered refresh.
- Phase 54.3e verifies and documents the SearchTimer preview `epgInput` contract so non-ready EPG input is not hidden as a normal zero-match result.
- Phase 54.3 now has a runtime cache refresh foundation: backend-scoped refresh service, refresh service registry, refresh controller, explicit REST refresh route, daemon wiring and router regression coverage are implemented.
- Phase 55.4c wires the SearchTimer discovery runtime to the RESTfulAPI provider and verifies daemon build through GitHub Actions.
- Phase 55.4d removes duplicate SearchTimer discovery JSON string decoding and routes discovery string escape decoding through the shared `JsonStringDecoder`.
- ADR-0035 records that recordings are a heavy on-demand domain and must not be loaded synchronously for all backends during daemon startup.
- Startup snapshot runtime is implemented for the initial poll: status, timers, SearchTimer metadata and channels may load, while recordings and full EPG events remain excluded from startup.
- Runtime lifecycle hardening 55.5l is verified against a real local daemon: duplicate daemon start on an occupied HTTP port exits cleanly with status 1 instead of aborting, SIGTERM stops the listener and releases port 18080 without `kill -9`, and the real VDR acceptance manifest passes 20/20 probes afterward.

---

## New Chat Handoff and Required Verification Checklist

New chats must start from this checklist before declaring a VDR-Suite change complete.

### Repository state

Collect the current repository state first:

```bash
cd /home/yavdr/vdr-suite
git status --short
git log --oneline -5
```

When work was done directly through GitHub, run `git pull` locally before compiling or runtime testing.

### Standard CI expectations

Every implementation or guardrail change must be verified by GitHub Actions unless it is explicitly local-only experimental work.

Required GitHub jobs:

```text
docs-check: success
fast-regression-test: success
Build daemon: success
```

Do not mark a phase as complete from local tests alone when the change was pushed to GitHub.

### Documentation and guardrail checks

Documentation-sensitive changes must keep the documentation checks green and must not accidentally move global phase markers unless all tracked files are updated together.

Minimum checks for documentation and guardrail work:

```bash
make test-real-vdr-acceptance-manifest
make test-daemon-runtime-shutdown-resets
make test-http-listener-bind-failure-handling
```

If a new documentation file is added, it must be reachable from the relevant documentation index or an existing navigation path before `docs-check` is considered trustworthy.

### Daemon lifecycle checks

Runtime lifecycle changes must be tested against the built daemon, not only with unit checks.

Start the daemon:

```bash
rm -f /tmp/vdr-suite-daemon.log /tmp/vdr-suite-daemon.pid

/tmp/vdr-suite-daemon > /tmp/vdr-suite-daemon.log 2>&1 &
echo $! > /tmp/vdr-suite-daemon.pid

sleep 3

ps -p "$(cat /tmp/vdr-suite-daemon.pid)" -o pid,cmd
ss -ltnp | grep ':18080'
tail -40 /tmp/vdr-suite-daemon.log
```

Expected startup evidence:

```text
simple HTTP listener running on 127.0.0.1:18080
```

Duplicate-start bind failure must exit cleanly and must not create a core dump:

```bash
/tmp/vdr-suite-daemon > /tmp/vdr-suite-daemon-bind-test.log 2>&1
echo "exit=$?"
tail -30 /tmp/vdr-suite-daemon-bind-test.log
```

Expected duplicate-start evidence:

```text
failed to bind HTTP listener to 127.0.0.1:18080
exit=1
```

Forbidden duplicate-start evidence:

```text
Abgebrochen
Speicherabzug
terminate called after throwing
```

SIGTERM must stop the daemon without `kill -9`:

```bash
pid="$(cat /tmp/vdr-suite-daemon.pid)"
kill -TERM "$pid"
sleep 3

ps -ef | grep '[v]dr-suite-daemon' || true
ss -ltnp | grep ':18080' || true
tail -50 /tmp/vdr-suite-daemon.log
```

Expected shutdown evidence:

```text
HTTP server runtime stopped
API router runtime stopped
REST controller runtime stopped
dashboard runtime stopped
database closed
vdr-suite-daemon runtime shutting down
```

### Real VDR acceptance checks

Before running the real VDR acceptance runner, ensure the process owning port 18080 is the daemon instance you intend to test.

```bash
cat /tmp/vdr-suite-daemon.pid 2>/dev/null || true
ps -ef | grep '[v]dr-suite-daemon' || true
ss -ltnp | grep ':18080' || true
```

A stale daemon can make acceptance appear green against the wrong process. If the PID file and port owner differ, stop the stale daemon first.

Run the real VDR acceptance manifest:

```bash
python3 tools/real-vdr-acceptance/runner.py \
  --base-url http://127.0.0.1:18080 \
  --max-risk dry-run \
  --report-json /tmp/vdr-suite-acceptance-current.json
```

Expected acceptance evidence:

```text
Real VDR acceptance manifest validation passed.
Real VDR acceptance passed.
```

The current expected real VDR acceptance scope is 20/20 probes, including safe reads, SearchTimer discovery/list/preview, workflow validation and workflow planning.

### Completion rule

A runtime-related phase is only complete when all applicable evidence exists:

```text
local build: success
required local guardrails: success
runtime lifecycle behaviour: verified when touched
real VDR acceptance: success when runtime/API behaviour is touched
GitHub docs-check: success
GitHub fast-regression-test: success
GitHub daemon build: success
```

---

## Current Architecture Highlights

- VDR remains the primary backend domain and source of truth.
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Runtime diagnostics are integrated through structured runtime measurement boundaries.
- Backend identity is present in snapshot change feed entries, snapshot read metadata and cached snapshots.
- Backend registry service, serializer and controller expose backend identity through service and REST boundaries.
- Snapshot cache can store snapshots per backend while preserving the legacy single-snapshot interface.
- Snapshot access and snapshot read services support backend-aware reads.
- VDR controller exposes default VDR reads, backend-specific reads and multi-backend snapshot summary reads.
- PollingService and BackendPollingCoordinator support backend-aware polling coordination.
- Initial PollingService startup snapshots intentionally skip recordings and full events so daemon startup remains lightweight.
- VdrEventQuery provides the first backend-neutral selective EPG query contract.
- Events and EPG are treated as heavy domains and are not automatically full-refreshed by default.
- Recordings are also a heavy domain for startup and multi-backend runtime planning; startup snapshots must not synchronously load recordings for every backend.
- EPG search operates over selective event windows and does not require a persistent full EPG mirror.
- SearchTimer preview exposes `epgInput.status`, `epgInput.available` and `epgInput.warnings`: ready empty input is a valid zero-result preview, while warming, stale, unknown and unavailable input is non-authoritative.
- SearchTimer preview EPG cache refresh uses backend-scoped selective event queries and is exposed through an explicit read-only refresh endpoint.
- Recording pages must eventually render before recordings are loaded and show backend-scoped loading state until the selected backend is ready.
- Recording actions use backend-native recording identity.
- Content classification uses source-aware evidence for genre, rating, metadata and policy work.
- Person architecture uses source-aware evidence, roles, confidence, normalized names, character names and provider references.
- SearchTimer discovery runtime now uses the RESTfulAPI provider when an HTTP backend context exists and falls back to the static provider only without a backend context.
- SearchTimer discovery string escape decoding now uses the shared `JsonStringDecoder` utility.

---

## Selective Backend Query Rule

VDR-Suite should prefer selective backend queries over full-domain transfers whenever possible.

Heavy domains must not use full-domain runtime refreshes as the default strategy.

Heavy domains currently include:

- EPG
- recordings
- metadata
- posters
- fanart
- preview data
- scraper-derived data

Preferred runtime strategies are:

- startup snapshots that exclude heavy domains
- channel-scoped queries
- time-window queries
- object-specific queries
- backend-scoped on-demand recording refreshes
- change-hint driven refreshes
- warm backend-scoped caches for interactive preview paths

Performance goal:

Backend workload should remain comparable to established VDR frontends such as live whenever equivalent information is requested.

Recording-specific startup rule:

Recordings are intentionally excluded from initial startup snapshots. Recording lists are loaded on demand or by explicit backend-scoped refresh paths so daemon startup and multi-backend initialization do not synchronously transfer large recording payloads.

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
