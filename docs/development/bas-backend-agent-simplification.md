# BAS.1 – Backend-Agent call-chain findings

Source baseline: `main@c8c971b7cfa01032acc4511be0cc370bfb9f9650`.
BAS branch: `work/bas-simplification`. Analysis is source-only; no runtime measurements or yaVDR acceptance are implied. Media Tools' outstanding MT.META7 and real acceptance are unchanged.

## Actual paths and bounded findings

1. **Inventory and selected preview (already optimized):** `BackendAgentClientRuntime::publishMediaImportSourceObservation()` sends one authenticated `/api/agent/v1/observations/media-import-sources` observation. `BackendAgentMediaImportRuntime::listTrustedSourceObservationCached()` calls `RootOwnedMediaImportInventory::listTrustedSourceSnapshot()` once, returning IDs, labels and the selected source descriptor from the same directory walk. It then calls `inspectResolvedSource()`. Do not add a second scan or a second preview engine. Existing `BackendAgentMediaImportCatalog` reuses successful codec/duration observations for fingerprinted `scan_` IDs, including durable checked-preview storage. Regression: `core/agent/tests/test_backend_agent_media_import_runtime.cpp`, `addons/media-tools/tests/test_local_media_import_inventory.cpp`, catalog tests.

2. **Redundant reads proven in legacy read helpers:** `BackendAgentMediaImportRuntime::listTrustedSourceIds()`, `listTrustedSourceLabels()` and `inspectTrustedSource()` each call `available()`, which invokes `inventory_.hasTrustedSource()` and fully scans sources. The same function then calls inventory listing or resolving, causing a second full scan for that read. Replace the preliminary *inventory* scan with existing `verifiedInstallation()`, retaining existing context/install/lock checks and the final inventory-based result. Regression: authenticated valid/empty/invalid inventory, busy maintenance lock, unavailable install, selected preview.

3. **Heartbeat work requires care:** `publishMediaImportSourceObservation()` first obtains installation evidence and the observation method independently verifies installation again. Later `discoverBackendAgentCommandAvailability()` calls the executor's `available()`, which performs another inventory walk. These are actual repeated reads, but source advertisement and executable command eligibility are security-relevant; no caching across authorization/dispatch boundaries is approved merely because a heartbeat has just been published. Measure separately before altering these paths. Regression: source disappearance, command availability, generation change, run-lock contention and result/readback.

4. **Error visibility:** `inspectResolvedSource()` presently initializes `source_invalid` before `inspectImportPreview()`, translating all exceptions at that step into the same code. `ffprobe_failed`, `source_changed`, `capacity_failed`, `target_not_writable`, `inventory_missing` and `target_collision` already exist and have Webfrontend translations; do not duplicate them. Distinguish known filesystem/extension errors from the existing inspection exception only when provable, preserving the wire's bounded, path-free `failureCode`. Regression: missing/unreadable/unsupported source and existing FFprobe/capacity/duplicate cases.

5. **Import, Operations and SSE remain separate:** The browser reads the authorized paginated source list; Core's catalog drives preview requests via the existing Agent observation response, and import is a protected Agent command with durable receipt/revalidation, recording reconciliation and readback. The frontend listens to source `update/resync` and Operation SSE and re-reads only on events/reconnect. No scheduled Operation polling or second import implementation is needed. Regression: frontend media-import composition, agent dispatch, Core catalog, operation/readback and SSE reconnect.

## Sequence and evidence boundary

BAS.2: remove only the independently proven extra full scans on legacy read helpers without weakening mutations. BAS.3: refine source-specific error classification within existing codes/protocol bounds. BAS.4: review the repeated verification paths against actual use; keep any independent use-time authority and authorization fences. BAS.5: focused source/CI tests and targeted live yaVDR CLI/Web/import/SSE/readback acceptance, **not** inferred from hosted CI.

Status: `BAS.1 SOURCE-CALL-CHAIN DOCUMENTED`; real call-count benchmarking, executed tests and live acceptance remain separate.


## BAS.2 – Implemented source changes (2026-11-22)

- The three legacy Agent read helpers no longer perform a full `available() -> hasTrustedSource() -> inventorySources()` scan before their own inventory read. The earlier nonempty-list semantics for IDs and labels are retained. Installation/context verification remains.
- The **live heartbeat** now calls `listTrustedSourceObservationWithEvidence()`, which produces the verified install evidence and the single source scan for the same observation. The production runtime verifies installation once per observation rather than twice. The default virtual implementation retains old injected executor behavior. The wire JSON and core catalog are unchanged.
- No new cache, scanner, worker, service, polling path, grant, activation gate or protocol version. `available()`, `authorize()`, `execute()`, source pinning and protected import readback remain independent.
- Regression additions in `core/agent/tests/test_backend_agent_media_import_runtime.cpp`: ID/label/preview equivalence, absent source, installation evidence from the observation, maintenance lock refusal and unchanged FFprobe failure.
- The older `listTrustedSourceObservationCached()` API remains usable and delegates to the new combined method; there is no client API change.

The removed calls are structurally demonstrable in the source: **one full scan removed per legacy read helper invocation**, and **one repeated installation-evidence verification removed per successful production heartbeat source-observation call**. This is not a measured latency claim; independent capability advertisement still checks current source availability.

## BAS.3 – Implemented error diagnosis

`mediaImportSourceInspectionFailure()` classifies only exceptions already thrown by the existing inspector. It retains `source_invalid` for unknown failures but returns `source_missing` for filesystem ENOENT, `source_unreadable` for EACCES, `format_unsupported` for the importer's existing extension rejection, and `source_changed` for its existing changed/nonempty-regular-file assertion. This is attached at `inspectImportPreview()`, not at authorization or dispatch. All codes are path-free, lower-case and within the existing preview protocol bounds. Existing `ffprobe_failed`, `capacity_failed`, `target_not_writable`, `target_collision`, `inventory_missing` and other causes are preserved. The Webfrontend reuses its existing failure-code translation map, with the new cases added.

Tests: synthetic exception classifications in the existing C++ Agent runtime test and a Webfrontend composition regression covering the displayed codes. Source vanishing *before* the inventory scan still yields `inventory_missing`: it is not falsely presented as a confirmed ENOENT from an inspection that never ran.

## BAS.4 – Verified scope of checks (source audit)

- **Retained:** authenticated Agent transport, handler provenance/installation proof, maintenance lock, backend generation/agent identity, canonical actor/backend authorization, pinned source revalidation before execution, idempotency/receipt journal, operation fencing, confirmation and native Recording-cache readback. These are not interchangeable UI validations.
- **Consolidated:** duplicate install-evidence verification in one Agent heartbeat observation; repeated full inventory walk on each legacy read call. Existing `verifiedInstallation()` and authorization methods are reused, not weakened or bypassed.
- **Not removed without evidence:** the later `available()` call in command capability discovery and separate use-time authorization. They may observe changed state after an earlier read; removing either as a blanket rule would change the current mutation boundary.
- **Frontend:** the browser's source selection consistency checks, existing `mediaImportSources` and Operation SSE subscriptions and reconnect readback remain unchanged. A failed Agent inventory observation must not block unrelated VDR commands.

BAS.4 source audit is recorded; actual regression execution must be evidenced before it is accepted.

## BAS.5 – Integration matrix (as of source candidate)

| Journey | Source evidence | CI | Real yaVDR |
| --- | --- | --- | --- |
| Agent inventory/known source and selected preview | Shared inventory scan, updated focused fixture | PENDING | NOT_VERIFIED on this candidate |
| CLI vs Web same source | CLI `vdr-suite-media-import plan SOURCE_ROOT VIDEO_ROOT FILE` uses the source path locally; Web selects the opaque Agent inventory ID. Both reuse `inspectImportPreview()` and `probeImportVideoCodec()`; CLI and Agent have different configured FFprobe timeouts. | PENDING | NOT_VERIFIED on this candidate |
| FFprobe, capacity and concrete failures | Existing inspector; error classification tests | PENDING | NOT_VERIFIED on this candidate |
| Protected import, operation and native recording readback | Existing pinned executor/operations unchanged | PENDING | NOT_VERIFIED on this candidate |
| Source/Operation SSE and reconnect | Existing browser event listeners unchanged | PENDING | NOT_VERIFIED on this candidate |
| Agent unavailable, maintenance and recovery | Existing refusal, added regression fixture | PENDING | NOT_VERIFIED on this candidate |

Candidate tests: `make build-media-tools-agent-composition-test` then run the synthetic disposable `.build/addons/test_backend_agent_media_import_runtime` in its specified CI fixture; `node web/frontend/tests/test_media_import_composition.js`, existing Agent/client catalog and Media Tools regression targets. The root-only synthetic fixture is **not** a live import. Later real yaVDR acceptance must check one selected CLI/Web source, non-mutating preview/FFprobe, an expressly approved test import with SSE and native readback, and recovery without blindly replaying an uncertain operation.

Status: **BAS.1 documented; BAS.2/BAS.3 source-implemented; BAS.4 source audit recorded; BAS.5 CI and real yaVDR acceptance OPEN.** No claim of completed phase, merge readiness, deployment or successful tests without executed evidence. Public API v1 and Media Tools' deferred milestones remain separate.

## Draft PR and initial CI evidence (22.11.2026)

Draft PR [#564](https://github.com/hotzenplotz5/vdr-suite/pull/564) targets `main` for source review and CI only. Baseline candidate `99fcc4f241cb30c6194784ed1831c663e2b9e22b` triggered [VDR-Suite CI run 45025539381](https://github.com/hotzenplotz5/vdr-suite/actions/runs/45025539381). Its documentation, architecture and Make-inventory jobs completed PASS. The frontend aggregate job stopped at `tools/check_mu9e_grant_mutation_ui.py` because the untouched `web/frontend/settings-account-admin.js` on **main itself** contains `role.admin` (existing SHA `dd2558eb73a20433b80e32a742a443835f5a17ab`), which the also-unchanged checker forbids. BAS did not change either file; this external baseline failure is not repaired in BAS and is not a BAS/frontend success claim. Later exact-HEAD jobs must be inspected separately. The existing `addon-platform-isolated.yml` is push-to-main/dispatch-only, so a BAS Draft PR alone does not execute its synthetic root Agent composition fixture.

## 22.11.2026 – REAL Media Import anomaly and scoped correction

The operator's yaVDR evidence distinguishes the successful import from the
optional metadata assignment. A local Recording `Rom (2005) S01E01` appeared
with the importer's generic title/subtitle, no persisted poster, and the
confirmed import Operation at 19:41:38 had payload version **1** with no
metadata snapshot. The same host's `Lost Treasures of Rome S01E03` Operation
at 19:40:22 had payload version **8** with a valid v2 snapshot and confirmed
`Media Import TMDb assignment saved` in the daemon journal. Four intervening
v7 Operations were duplicate-cleanup actions, not metadata-bearing imports.
This is **not** evidence that the daemon's v2 metadata writer is generally
broken or that the preview's artwork was necessarily a selected TMDb record.

The source audit found a specific generic selection-loss path:
`refreshSelectedPreview()` retained manual TMDb selections but discarded
a confirmed *automatic* selection even when the opaque source ID was unchanged.
The next import could therefore use the original version-1 payload without a
snapshot. On this BAS branch, the selected provider/episode identity and
automatic provenance now survive a same-source NFO, capacity, or poster refresh;
a changed source still clears the identity, and no automatic import is added.
The **existing** confirmation reports when a lookup is still pending or no
provider choice is confirmed; offline/local-only import remains allowed and
unchanged. No provider lookup, permission gate, or new background job is added
to the import click.

Focused composition regressions cover automatic selection retention across
two technical refreshes, fresh matching after an actual source change,
late-result fencing, local-only/offline confirmation and no unrequested
operation. This is a SOURCE fix; the exact new code's complete Node test and
live yaVDR acceptance remain open until they actually run. The previously
imported `Rom` video is not re-imported: Home's existing Series backfill
has already displayed one enriched episode and a Series poster, while
native `info` verification remains separate.

## BAS.6 – Full Backend-Agent scope, first composition simplification

BAS is **not limited to Media Tools**. The actual runtime spans
`core/agent/src/BackendAgentClient.cpp`,
`BackendAgentCommandClient.cpp`, the SuiteBridge handshake/observation
transports, native Timer create/delete/modify, Recording marks/cut, channel
and OSD observation, protected command state/receipts, credential rotation
and `apps/agent/main.cpp` composition.

- **BAS.6 source change:** two identical closures in
  `apps/agent/main.cpp` formerly built the same live command context for
  Media Import and HEVC/Media Shrink. They now share one local
  `currentCommandContext` closure. Each use still reads the current Agent
  state, instance ID, credential and backend generation; no credential is
  captured or cached at construction. Executor opt-ins, transport, authorization,
  heartbeat and command semantics are unchanged.
- **BAS.7 – Health/Channel/OSD observations:** inspect the already distinct
  pending-observation journals, snapshot generations and resync paths.
  Simplify only duplicated pure construction when equivalent; do not combine
  their unrelated protocol lineages or change observation ordering.
- **BAS.8 – Native Timer/Recording command flows:** inspect create/delete/modify
  and marks/cut through the single local receipt/result journal, generation
  fence, timeouts and native readback. Identify actual duplicated work first;
  never consolidate independent use-time permission or physical VDR guards.
- **BAS.9 – Transport, lifecycle, capability and recovery:** inspect
  enrollment/credential rotation, reconnect, heartbeat/fast command polling
  and capability publication. The fast poll already reuses one availability
  snapshot per heartbeat; don't reintroduce redundant probing, polling or
  loosen command replay fences.
- **BAS.10 – Final cross-domain acceptance:** relevant Agent build and
  focused native command, observation, Media Tools, SSE and reconciliation
  regressions plus controlled yaVDR checks. No merge/ready-for-review claim
  before the final-head graph and genuinely necessary hardware evidence.

BAS.6 is **source-implemented**; its exact-head build/CI and real-runtime
acceptance must be cited separately. BAS.8–10 remain planned. Parallel
API v1 remains on its own branch and is not merged or modified by BAS.

## BAS.7 – Independent Health / Channel / OSD observation audit and refactor (23.11.2026)

**Observed production owners and non-interchangeable transport semantics:**

- **Backend Health:** `BackendAgentClientRuntime::preparePendingBackendHealthObservation`
  stores an immutable in-flight `completeSnapshot` or `changeBatch` identity
  and heartbeat sequence in the protected Agent identity. Retries send the
  exact saved observation and only promote the sequence on accepted/replayed
  Core response. A 409 resync starts a new Health snapshot lineage.
- **Channels:** `preparePendingChannelObservation` reads the native
  `channels.conf` (including administrator-selected symlinks) and compares
  the canonical resource revision. Unchanged observations are skipped; a
  changed source starts a new `completeSnapshot` generation. Its separate
  protected `identity.channels.pending.json` is retried byte-for-byte and
  deleted only after accepted/replayed acknowledgement. The Agent heartbeat
  submits any pending Channel observation before starting a new one.
- **OSD:** `publishOsdObservation` sends the ephemeral current frame, or a
  resync/degraded state without stale frame contents, on the heartbeat cadence.
  It has no disk pending journal or blind replay, and failure must not block
  Health/Channel/native commands. The receiver's bounded latest-value cache
  and freshness/Agent-authority checks remain unchanged.
- **SuiteBridge:** `SuiteBridgeObservationService` and
  `SuiteBridgeObservationWorker` own a separate native discovery,
  stale/offline state and snapshot baseline, not one of the Control Plane's
  authenticated Health/Channel/OSD wire protocols.

**Concrete safe duplication:** `BackendAgentClient.cpp` previously repeated
the same bounded 0→1 / n→n+1 `int64_t`-safe snapshot progression in five
places (Health+Channel generation reset at Connect, Health reset, Channel
reset, changed Channel snapshot). BAS.7 puts only that arithmetic in the
file-local `advanceObservationSnapshotGeneration` function. Each caller
retains the existing domain-specific error code, affected fields, pending
journal behavior, 409 recovery, resource revision and persistence ordering
of its own protocol. No new service, queue, lease, guard, auth rule, data
format, endpoint, poll or OSD frame state.

**Regression:** `core/agent/tests/test_backend_agent_client.cpp` already
covers Health first-generation→replay→resync, Channel unchanged skip→new
snapshot generation→durable pending retry, legacy identity and connect.
BAS.7 adds `test_observation_lineage_generation_exhaustion`: exact
`int64_t`-maximum Health and Channel generations independently reject
reconnect with their original distinct diagnostic codes, without advancing
to heartbeat or submitting an observation. Target:
`make test-backend-agent-client`. This is a fixture on disposable `/tmp`
paths, not a live service mutation.

**Acceptance:** source refactor and regression committed on the BAS branch.
Build/targeted CI on the final head and real Device/OSD observational
acceptance remain explicitly OPEN until actual evidence is checked. No
deployment to the user's already functioning yaVDR for this refactor.

## BAS.8 – Native Timer / Recording command identity audit (23.11.2026)

**Existing verified architectural boundary:** The common delivery runtime in
`core/agent/src/BackendAgentCommandClient.cpp` already centralizes the
protected `LocalState`, receipt/result publication, command assignment,
restart reconciliation and the generic result contract. Each of five native
domains retains its own typed context, transport, evidence parser, execution
guard and use-time readback: Timer Create, Delete, Modify, Recording Marks
Modify and Recording Cut. Their separate outcome mappings are deliberately
**not** combined; accepting an unverified native outcome as successful
would be a real regression.

**Safe duplication removed:** The five domain context adapters all populated
the same four identity values in the same order: backend ID, Agent ID,
Agent-instance ID and backend generation. A file-local
`nativeCommandIdentity<DomainContext>` constructs that immutable value.
Each existing typed adapter still produces its original concrete context;
no command types, journal filenames, native actions, ownership fences or
authorization checks changed.

**Regression targets:** `make test-phase63-command-delivery-runtime`,
`make test-backend-agent-client` and the existing native
Timer/Marks/Cut suites. Tests in GitHub CI or on a disposable fixture are
distinct from real timed recordings, editing marks and cut operations on
a live VDR; no live mutations requested or executed.

## BAS.9 – Command poll / heartbeat composition without stale capture (23.11.2026)

**Observed repeated work:** `BackendAgentClientRuntime::pollCommands`
(fast interactive poll) and `::heartbeat` (lease and observation cadence)
contained byte-equivalent mappings of command state path, advertised
command types, three native Timer transports, legacy OSD input, Media Import
and Media Shrink executors. They also each constructed the same live
authenticated Agent command context.

**Source refactor:** Two file-local pure functions now create fresh
`BackendAgentCommandClientConfig` and
`BackendAgentCommandClientContext` values at each call. No credentials,
native transport pointers, backend generation, advertised availability,
poll receipt or command state are cached as part of the refactor. The
heartbeat still reconciles the durable journal before observations,
discovers capability once on its own cadence and polls with refresh=true;
fast polls still reuse that heartbeat's availability snapshot with
refresh=false. Existing resync, fail-fast, delayed heartbeat and
lost-response handling remain intact.

**New focused regression:** The real Agent-client fixture
`test_low_latency_command_poll_between_heartbeats` asserts that command
polls before and after the intervening heartbeat retain the authenticated
backend ID, Agent-instance ID, backend generation and credential from the
running runtime. This supplements its previous checks of poll cadence,
capability refresh flags, old-generation quarantine and lease renewal.
Targets: `make test-backend-agent-client` plus established fast tests.

**Acceptance status:** BAS.7–9 changes are committed on the *authorized*
`work/bas-simplification` branch only. New and existing targeted tests,
full package/Agent build and final-head CI must be checked against the
actual run results. BAS.10 still requires cross-domain integration,
documented partial coverage, and focused real yaVDR observation before
declaring the overall phase complete. No branch merge, package install,
daemon/VDR restart or media mutation occurred from this development step.
