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
