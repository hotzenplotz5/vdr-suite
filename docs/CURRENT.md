# VDR-Suite Current State

## Recording Public-v1 checkpoint and delivery order (2026-10-21)

**Authoritative working branch: `main`; strategy: R1 -> R2 -> R3 -> router modularization checkpoint -> E1/E2/M1/M2/S1/P1.** Avoid a general `ApiRouter`/`PublicApiRuntime` refactor until the Recording semantics have settled at R3. The later router checkpoint must only relocate responsibilities without changing URLs, JSON contracts, grants, identity ownership, or domain behavior.

- **R1 source + CI COMPLETE at `dd8b3317f9347146463b3ce78636228aa2ebb84f`**: Public `rec_` resolution via existing identity/cache playback resolver; v1-only validation/dry-run; client-native fields rejected; scoped action permission and browser CSRF on validate and execute; legacy routes retained. All ten GitHub check runs green at that SHA. This is code/CI verification, **not** a new productive yaVDR hardware acceptance.
- **R2 in progress (initial precondition-only increment):** public RENAME/DELETE real mutation is still disabled; a real `execute` must require syntactically strong `If-Match`, but parsing it does *not* substitute an authoritative recording revision. No derived cache-only ETag is accepted for a write. Unsafe client `targetPath` is rejected and MOVE is held for R3. Proper public DTO, authority/If-Match comparison, execution, verified readback, DELETE identity retirement and RENAME rebind remain open. Explicit disposable hardware fixture is required for any later live write.
- **R3 not started:** logical public folder target, verified move and persistent `rec_` rebind. No automatic mutation retry.
- Older dated entries below may describe earlier candidate-only or branch-specific stages; do not overgeneralize them to current source or installed host.

## Operational status authority

**This file is the sole repository authority for volatile operational phase status.**

Stable architecture, historical evidence and workflow rules live in their respective ADRs, roadmap and closeout documents. Exact live GitHub state must still be re-read before implementation, review-state or merge actions.

## Consolidated API v1 / VIDAA Timer work (2026-10-19, hardware CREATE accepted for one bounded case)

### Public/native Timer specification boundary preserved (2026-10-19)

The public MODIFY request previously included an internal
`NativeTimerSpecification` type directly, violating the existing
Phase-69.C API-domain separation. It now uses a bounded, backend-neutral
public change-field DTO. Conversion to the internal native specification
happens only inside the authenticated daemon callback, followed by
the unchanged native validation before accepting a durable operation.
No internal TimerAssignment desired specification is exposed publicly.

### Native Timer MODIFY contract, OpenAPI and focused test (2026-10-19)

The POST `/api/v1/native-timers/{timerId}/modifications` contract is now
audited in the Public-v1 OpenAPI inventory, with one complete UPDATE body
or one explicit TOGGLE boolean; it requires a device-scoped
`timers.modify` grant, strong `If-Match`, `Idempotency-Key`, exact backend
and durable acceptance. The focused HTTP test covers 202/replay,
revision conflict, external adoption refusal, incomplete/duplicate
fields, malformed preconditions and missing backend service. Current
GitHub CI must still verify compilation and route integration.

### Public native Timer modification HTTP request contract (2026-10-19)

A bounded asynchronous `POST /api/v1/native-timers/{timerId}/modifications?backendId=...`
now exists at PublicApiRuntime source level. The strict JSON body is
either `{"kind":"toggle","enabled":false}` / `true`, or
`{"kind":"update",...full native Timer specification...}` with
exactly 11 specified fields. The route requires exact backend, strong
`If-Match`, `Idempotency-Key`, valid JSON Content-Type and a
registered durable operation-admission callback. Unknown/duplicate
keys, partial field updates and unsafe native command strings are
rejected. Device authorization and the Daemon admission callback must
be joined before MODIFY becomes accessible in production.

### Explicit Device grants for timer MODIFY/DELETE (2026-10-19)

The Device-grant administration allowlist previously supported
`timers.create` but **not** `timers.modify` or `timers.delete`.
The latter already had an authenticated HTTP route but could never
be approved for a paired TV. Both rights can now be explicitly granted
or revoked by an administrator on one concrete backend. Wildcard,
empty and parent scopes are denied. This does not grant new privileges
to any existing VIDAA pairing or bypass the exact grant check on a
public write endpoint. An end-to-end grant/revoke regression is included.

### Native MODIFY joined the productive Daemon polling loop (2026-10-19)

Daemon lifetime now owns the UPDATE/TOGGLE dispatcher, Agent assignment
service, native Timer observation verifier and completion service.
Polling enumerates the two MutationOperation families and acquires
an independently parsed, directly fetched RESTfulAPI Timer observation
from the exact backend Agent generation. The shared operation is marked
succeeded only after all requested Timer fields match and the binding
revision is verified. This source-level production composition does
not expose MODIFY HTTP admission, require a host restart or imply
hardware acceptance. Legacy Phase-64 architecture checks must still
validate this successor.

### Native UPDATE/TOGGLE productive runner (2026-10-19, source)

A bounded Daemon MODIFY runner now shares the Phase-64 operation,
intent, assignment and native binding repositories. Before claim it
rechecks the exact current binding fingerprint, assignment/intent
revisions, backend generation, ownership, active Agent lease and
observation freshness. It allocates one provider-fenced native
UPDATE/TOGGLE command, applies only typed result evidence, and
marks the MutationOperation succeeded **only after** the
`NativeTimerModifyReadbackVerificationService` verifies the exact
updated native Timer observation and completion advances the shared
operation state. Unknown Agent results are reconciled, not resent.
Production polling composition and authenticated public API admission
are still outstanding.

### Direct native MODIFY readback reader (2026-10-19)

A new RESTfulAPI readback reader fetches the exact native Timer state
from VDR, not a cached Daemon snapshot. It requires two successful
direct reads: a strict complete inventory and a full Timer field
projection with an identical unique native ID set. HTTP errors,
truncation, inconsistent snapshots, absent IDs and incomplete Timer
fields fail closed. The resulting `NativeTimerObservation` is suitable
for the Phase-64 MODIFY verification service, which must still compare
all requested fields and update the binding before operation completion.
No public MODIFY endpoint has been enabled.

### MODIFY durable Control-Plane operation outcomes (2026-10-19)

The Daemon has a typed UPDATE/TOGGLE Agent-result bridge, based on the
existing Phase-64 `NativeTimerModifyDispatchService`. It validates the
Agent command envelope, persisted starting/dispatch times, complete
operation payload, target specification, binding identity and provider
generation before advancing a shared `MutationOperation`. This bridge
does **not** by itself mark a Timer modified: the target native Timer must
still be observed and verified by the separate MODIFY readback stage.
Polling and public HTTP MODIFY admission are not wired yet.

### MODIFY Agent typed result wired to durable StateStore (2026-10-19)

Native UPDATE and TOGGLE now persist their canonical typed result
evidence through `BackendAgentCommandStateStore` on successful execution,
unknown outcomes and subsequent restart/replay. The Agent compares the
reconstructed result to the exact durable local MODIFY extension before
resending, and rejects evidence conflicts. The same narrowly defined
expired-before-dispatch exception applies to MODIFY operations, but no
operation that might have reached the VDR may omit typed evidence.
No public mutation or real hardware modification was invoked.

### Timer MODIFY native result evidence (2026-10-19)

Native UPDATE and TOGGLE now have a canonical, bound, length-safe Agent
result evidence contract, matching the existing DELETE operation pattern.
It carries the durable local-starting, dispatch and completion times and
rejects tampered or mismatched Agent assignments. The focused native
MODIFY contract test checks roundtrip and trailing-field tampering.
The public write method and Daemon outcome dispatcher remain separate
work; this change does not modify VDR timers.

### Public Timer MODIFY – Agent capability lifecycle (2026-10-19)

The native UPDATE/TOGGLE Phase-64 assignment helper previously
revoked every advertised `vdr.timer.update`/`vdr.timer.toggle`
capability and recreated old dormant triggers each time an operation
was assigned. This would have made any public MODIFY API permanently
unexecutable despite a healthy live SuiteBridge capability report.
The unique operation index remains enforced, but assignment no longer
removes capabilities. Agent schema initialization and capability refresh
also remove these legacy triggers, with the architecture guard
explicitly rejecting capability revocation. Source-level change only;
public MODIFY and live hardware acceptance are not yet claimed.

### Native Timer DELETE expiry regression (2026-10-19)

The follow-on fast-regression failure at
`test_backend_agent_timer_delete_fresh_durable_starting.cpp:292`
was caused by the new typed-result StateStore check: a command
rejected after its deadline **before dispatch** correctly has no
native starting extension. The StateStore now accepts that one exact
expired, effect-free result without a typed native outcome, while
every started DELETE still requires verified canonical evidence.
The regression also confirms persisted reload and rejects a changed
outcome category.

### Agent DELETE result replay persistence corrected (2026-10-19)

Current-head Fast Regression and Recording-R2 builds both failed at
`test_backend_agent_timer_delete_durable_executor_outcome.cpp:342`:
fresh DELETE wrote canonical `resultEvidence`, but the Version-3
`BackendAgentCommandStateStore::load` reconstructed that field only
for native CREATE and left the DELETE field empty. The subsequent
reconcile therefore (correctly) rejected the generic result as conflicting
with its immutable local evidence. DELETE now reconstructs the typed
result from the completed, validated local extension on reload and
checks the exact canonical equality before persisting, mirroring
CREATE's existing guarantee. Focused replay regression asserts
reload stability and rejects tampered evidence without rerunning VDR.

### Phase-64 Agent DELETE contract check recognizes bounded daemon consumer (2026-10-19)

After three native DELETE source-manifest checks were updated, CI advanced
to a fourth guard that still prohibited using the pure DELETE domain
validator inside `mk/daemon-sources.mk`. The later verified daemon
successor now qualifies as a second audited consumer **only** when
provider-fenced Agent assignment, typed result correlation and
complete-inventory absence readback are present. The Agent remains the
only native executor; other source manifests are still prohibited.

### DELETE successor architecture-check fix (2026-10-19)

Two CI workflows on HEAD `5e7d714` failed at the same legacy Phase-64
`NativeTimerDeleteOperationCompletionService.cpp` source-manifest
prohibition. Slice-21/22/23 audit scripts now admit only the actual
`mk/daemon-sources.mk` successor if it simultaneously links preparation,
dispatch, completion, typed Agent outcome handling and the bounded DELETE
runner with existing exact-binding preflight, provider fences and
native-inventory absence verification. No unrelated manifest, operation,
API or direct VDR mutation is allowed. The fix addresses the identified
CI root cause; subsequent CI must still prove C++ build and the
protected public DELETE regression.



**Active integration is the existing backend branch** `work/public-v1-vidaa-unified-20261018`, open Draft [PR #500](https://github.com/hotzenplotz5/vdr-suite/pull/500), **not** the earlier standalone Slice-2 branch named in the historical sections below. The independent client source branch is `work/vidaa-epg-timer-operation-readback-20261017`. Do not create successor branches, merge this draft to `main`, or deploy on yaVDR without the usual separate gates.

### Real public-v1 one-shot Timer CREATE acceptance (2026-10-19; current evidence)

**Hardware milestone:** The installed daemon identified itself as
`git-f7e341ad6b97` ([commit](https://github.com/hotzenplotz5/vdr-suite/commit/f7e341ad6b9730bc35bf223c26a04f5b1df39ce8));
yaVDR ran VDR 2.8.3 with SuiteBridge 0.14.1. An administrator-approved
**temporary** paired Device received backend `default` permissions
`epg.view` and `timers.create`. Real public
`POST /api/v1/epg/timer-intents` with a strong event ETag and unique
idempotency key returned **202**, durable Operation
`op_d9589fb8b43b7d91ac4309423a2fc372` became **`succeeded`**,
and the native VDR readback found **one and only one new** timer for
**Tagesschau**, channel `C-1-1051-10301`, event `13520`,
**2026-10-20 20:00–20:15 Europe/Berlin**. Before: 0 VDR Timers;
after: 1; `VDR_READBACK=PASS`; `TEST_RESULT=PASS`. The temporary
test Device was revoked, and the administrator logged out. The native
Timer remains in place. This is an actual Public-v1-to-VDR end-to-end
test, **not** SVDRP NEWT or a mock. See
[exact bounded evidence and prerequisites](../api/epg-timer-intents.md).

**Operational finding and remaining gates:** First HTTP **503** was
caused by inactive/disabled Backend Agent with expired lease, stale
provider facts and a mismatched SuiteBridge provider epoch. The Agent
was deliberately started, lease/provider facts refreshed, exact
backend-local native clock and `NTPREVIEW CHECK 1` returned attested
`isolated_clear`, and the real POST then succeeded. Agent **autostart
remains disabled**; host reboot/identity reuse still need verification.
The actual programme recording on 2026-10-20, subsequent recording
integrity, ambiguous `outcome_unknown` resolution, restart/reconciliation,
concurrency, overlaps, encryption, recurrence and multi-backend cases
are **not yet accepted**. Historical Agent reconciliation records must
not be replayed/reset; existing VIDAA Device grants stay unchanged.
See [packaging boot opt-in](../debian/README.md#after-enrollment-boot-activation-operator-opt-in-not-package-auto-start).
Earlier read-only sections below are timestamped *pre-acceptance*
checkpoints; assertions there that no live CREATE/readback occurred
are **superseded for this single case** by the acceptance here.

**Client follow-on:** The separate VIDAA work branch
`work/vidaa-epg-timer-operation-readback-20261017` now retains the last
accepted Operation across EPG navigation in the app session, distinguishes
`outcome_unknown`/conflicts, and does not retry native CREATE. Its
[exact-head CI #1412](https://github.com/hotzenplotz5/vdr-suite-vidaa/actions/runs/39277372021)
passed at `b6720d3ab67f86ffc055898c7ca24f568bfb1bc9`.
**Hisense deployment and functional TV acceptance are still PENDING.**
PR #500 is still a Draft pending its own complete integration and merge
gates. The live daemon's source ID is not the present PR head.

### Native DELETE absence verification now covered by successor guards (2026-10-19)

The original readback expectation and verification architecture guards
continue rejecting arbitrary runtime uses. Their only newly permitted
consumers are the explicit daemon DELETE runner and daemon composition
files, and only when the successor contains exact operation/Agent-result
fences, a complete generation-fenced native inventory and a
verification-before-completion sequence. This replaces obsolete
"readback is always premature" assumptions without bypassing them.

### Typed DELETE executor guard follows its extracted helper (2026-10-19)

The Phase-64 executor architecture check now verifies that the complete
dedicated Agent handler persists typed result evidence, instead of
erroneously searching only the fresh-execution subroutine tail for a
projection helper defined before that subroutine. The one-shot executor
and receipt/durability fences are unchanged.

### Native DELETE capability lifecycle fixed (2026-10-19)

The Phase-64 `ensureNativeTimerDeleteAssignmentSchema` previously
deleted the authenticated Agent's `vdr.timer.delete` capability row and
recreated its obsolete dormant trigger on **every assignment**. The
productive DELETE loop therefore could never reach an executable Agent
command after a healthy capability refresh. It now keeps the unique
operation index, removes the legacy dormant trigger and leaves live
capability facts to the existing provider-fenced Agent polling path.
A regression checks that a replay assignment preserves the advertised
capability. Explicit successor audit rules admit only the named daemon
native-inventory consumer while retaining the Phase-64 restrictions on
unrelated callers.

### Native DELETE repository successor boundary (2026-10-19)

The old Phase-64 repository source guard now explicitly audits only
named daemon DELETE files against the durable typed Agent result and
native absence readback; it does not permit general API, Agent or VDR
repository bypasses. The legacy CREATE repository scope is unchanged.

### HTTP DELETE fallback compatibility correction (2026-10-19)

The public DELETE route is dispatched explicitly, but all other
previously read-only Public-v1 resources must still report HTTP 405
for DELETE, not fall through unhandled. The public runtime
unsupported-method matrix is restored to the original GET/POST
exception while the new audited DELETE handler remains separate.
The OpenAPI drift guard now anchors on the exact DELETE dispatch
path check, not its GET equivalent.

### Native DELETE successor architecture guards updated (2026-10-19)

Legacy Phase-64/Phase-69 source guards that universally prohibited
productive native DELETE source are extended only for the exact named
DELETE runtime/outcome files. The successor requires typed Agent evidence,
the existing public DELETE preflight, shared Operation dispatch and complete
native readback. The previous generic outcome-timestamp string check was
upgraded to require typed persisted resultEvidence instead. Existing CREATE
checks remain unchanged. CI is still the actual acceptance authority.

### Native Timer DELETE API-v1 admission integrated (2026-10-19; source)

Public `DELETE /api/v1/native-timers/{timerId}?backendId=...`
now enters the existing Agent/ADR-0042 pipeline. The HTTP listener routes
DELETE with strong `If-Match`, `Idempotency-Key`, and the exact backend
scope admitted by the SecurityHttpGate. Only a verified Device credential
with exact `timers.delete@backend` is authorized. Admission requires
the existing bound TimerAssignment and NativeTimerBinding, current snapshot
fingerprint, generation, active Agent lease and fresh observation, then
reserves an idempotent `timer.delete` operation. External and unbound
Timers remain nondeletable until explicit adoption. OpenAPI now audits
the public method and its headers; the focused runtime test enters
`test-fast`. Native absence, not command acceptance, defines success.
Hardware tests and CI confirmation remain outstanding.

### Native DELETE productive daemon pipeline linked (2026-10-19; source)

The existing daemon polling loop now discovers accepted DELETE operations,
assigns a single provider-fenced command, applies canonical Agent outcome
evidence, and verifies absence through a **complete native inventory** before
marking the shared MutationOperation successful. The polling path uses the
same Phase-64 dispatcher and completion services. Uncertain results are
reconciled and never blindly repeated. The new source files are included
in the daemon link source inventory. Public DELETE is not enabled yet
and the Agent's deliberately dormant DELETE capability has not been
silently overridden. The running VDR and timer schedule are untouched.

### Native DELETE durable Control-Plane outcome application (2026-10-19)

The daemon-side `NativeTimerDeleteResultOutcomeApplication` now
consumes the actual typed Agent result, correlates the exact operation,
binding, provider command identity, accepted-to-dispatching revision and
persisted assignment timestamp, and delegates to the existing Phase-64
`NativeTimerDeleteDispatchService::applyOutcome`. A trustworthy
post-dispatch readback expectation is generated **only** from typed
dispatch evidence, never from a generic success response. A missing,
malformed or conflicting result does not mutate the operation.
This source is not yet composed into the polling loop or publicly
accessible, and the dormant Agent capability remains disabled.

### Native DELETE Agent typed outcome persistence (2026-10-19; source)

The native DELETE Agent now persists its real local starting/dispatch/completion
timestamps and immutable operation/provider correlation into typed
`BackendAgentCommandResult.resultEvidence`, as CREATE already does.
Recovery checks replay the same canonical evidence; missing or changed evidence
fails closed instead of manufacturing a readback boundary. The durable result
can therefore be consumed by a future daemon DELETE outcome-application
service without guessing native dispatch time. This is an actual delivery-path
change, but public DELETE and the dormant capability gate are **not enabled**
by this commit. No backend mutation or host test was performed.

### Integration CI guard correction (2026-10-19)

The new native Timer detail route is explicitly added to the audited
partial OpenAPI GET inventory and both Phase-69 public route-literal
classifications. Exact-head CI before this correction failed solely on
these inventory gates in docs-check and fast-regression. The route is
still read-only, and native DELETE remains 405. No production host,
VIDAA identity, other branch or package was modified.

### Public-v1 native Timer detail GET wired (2026-10-19, CI pending)

The existing integration branch now exposes
\`GET /api/v1/native-timers/{timerId}?backendId=...\`. SecurityHttpGate
requires the exact \`timers.view\` backend grant. The Daemon resolves
one matching native snapshot ID, rejects duplicates, checks Backend Agent
generation and the existing durable NativeTimerBinding using
\`NativeTimerPublicDetailRevision\`. A strong conditional ETag is
returned *only* for matched, non-drifted, non-recording managed/adopted
bindings and verified external bindings (whose \`bindingState\` is
\`adoption_required\`). Unbound Timer snapshots return a readable
\`binding_required\` state with **no ETag**. Mismatched fingerprints
and generation fail closed with 503, never a synthetic revision.

\`DELETE\` remains **405**, not remotely executable; public
\`timers.delete\`, explicit external adoption, operation idempotency,
Agent/provider dispatch and post-dispatch absence readback remain
separately gated. OpenAPI includes this bounded GET only. The focused
HTTP test and daemon syntax check were added to \`test-fast\` but
their CI pass is not yet asserted. No yaVDR host changes or merges.

### Native Timer single-resource revision evidence (2026-10-19; source-only)

The new read-only \`NativeTimerPublicDetailRevision.h\` compares the
full native VDR Timer snapshot against the durable NativeTimerBinding
observed-state fingerprint and the independently supplied current Agent
backend generation. Only an exact, present, non-recording, non-drifted
managed/adopted binding yields a version token; externally owned bindings
remain \`adoptionRequired\`, and unbound snapshot Timers have no
write-precondition revision at all. Unsafe mismatches fail closed.
The focused regression is in \`test-fast\`. This is **not yet a wired
HTTP GET or DELETE endpoint** and does not add an implicit adoption path.
The existing production CREATE, VIDAA identity and yaVDR host are unchanged.

### Public-v1 native Timer DELETE source preflight (2026-10-19)

The source-only preflight checks exact backend and native Timer identity,
backend generation, binding revision, canonical observed-state fingerprint,
server-required observation freshness, missing/drift and recording state.
Externally created Timers fail closed with adoptionRequired until an
explicit, authorized adoption/TimerAssignment relationship exists.
This is a read-only guard and test-fast regression, not yet an HTTP DELETE
endpoint, installed feature or hardware acceptance. Public ETag/If-Match,
timers.delete authorization, idempotency, Agent dispatch, native readback and
OpenAPI integration remain pending. See
[Public Native Timer DELETE preflight](development/public-v1-native-timer-delete-preflight.md).

### Agent autostart packaging handoff and next Timer v1 gates (2026-10-19)

On the existing integration branch, the experimental `vdr-suite-backend-agent`
package now stages a protected post-enrollment helper,
`/usr/sbin/vdr-suite-backend-agent-autostart`, with `--check` and
`--enable` and isolated no-root regression contracts. The helper checks the
already-running `vdr:vdr` unit, local enrolled identity, configured HTTPS
Control Plane, current online lease and fresh backend-health evidence before
enabling **future** systemd boot startup; it never requests a service restart.
The existing `debian/rules` `dh_installsystemd --no-enable --no-start`
policy remains deliberately unchanged for unconfigured package installs.
It is **not yet wired into a completed setup/first-run wizard**; it currently
requires a same-host Control Plane/Agent, and remote-Agent automated readiness
is a separate design task. Relevant source:
[helper](../tools/vdr_suite_backend_agent_autostart.py),
[package integration](../debian/vdr-suite-backend-agent.install),
[regression test](../tools/test_packaging_backend_agent_autostart.py) and
[operator guide](../debian/README.md#after-enrollment-boot-activation-operator-opt-in-not-package-auto-start).
No published PPA package, live yaVDR unit or boot policy changed because of
these source commits. **Package/CI completion, on-host activation and
post-reboot evidence remain PENDING.** No claim that a new package installation
automatically configures and starts an enrolled agent.

**Next Timer API v1 workstream order:** (1) verify current integration
branch/PR exact-head CI and safe package contracts without rerunning the
accepted real CREATE; (2) for the already-enrolled yaVDR Agent, first read-only
host preflight, then independently authorized `systemctl enable` and a later
controlled reboot with same technical identity, fresh lease/provider facts and
matching SuiteBridge epoch; (3) after 2026-10-20 20:15 Europe/Berlin, check
the existing Tagesschau timer's *actual recorded file* and playable integrity
without creating a second Timer; (4) deploy/test the existing VIDAA code on
the Hisense TV, including remote focus, one submit, explicit Operation GET and
uncertain-outcome handling; (5) implement Public-v1 native Timer DELETE and
then MODIFY/enable-disable with protected permissions, exact native
identity/revision/generation, idempotency, provider readback and
reconciliation, including timers not originally created by Suite; and
(6) separately expand conservative CREATE scheduling to overlaps,
recurrences, encrypted channels, multiple tuners/backends. Public-v1
`GET /api/v1/native-timers`, EPG TimerIntent POST, TimerAssignment reads,
and actor-owned Operation GET already exist in source; the general
Public-v1 native Timer DELETE/MODIFY APIs are **not yet accepted**. Never
repeat the successful 2026-10-19 CREATE simply to reproduce it, replay
historical `waiting_reconciliation` commands, revoke the existing VIDAA
Device, or merge the draft PR without its outstanding gates.

**Current source wiring, verified by code inspection:** `core/daemon/src/DaemonRuntimeInitialization.cpp` now **does register** `registerEpgTimerIntentSubmit()`. It composes fresh EPG detail/permission/backend policy and backend-generation fences with the read-only native VDR-local `NTCREATE CLOCK 1` projection and `NTPREVIEW CHECK 1` prospective conflict attestation; only then can `EpgTimerIntentAtomicAdmissionService` persist the controlled Intent + Assignment + Operation. The source-level provider/clock/preview checks remain intentionally conservative, especially with overlapping, repeating, encrypted or unverified-device scheduling. The old statements below that “no productive submit is registered” are **historical** and must not be used as current source truth.

**Real yaVDR read-only CLOCK observation (2026-10-19):** The operator's running VDR **2.8.3** and SuiteBridge (still reporting plugin version **0.14.1**) now return `900` for `NTCREATE CAP 1` (`timer-create enabled`) and `900 vdr-suite-ntcreate-clock/1` for the UTC window `1792519200` to `1792522800`, projecting exactly `2026-10-20 20:00–21:00` VDR-local time with `7200`-second offsets at both boundaries. General `CAPS 1` still reports `mutations: disabled`; this is a separate capability. This supersedes the **historical 2026-10-18 `501 unsupported` CLOCK observation** for the *installed CLOCK command only*. The installed plugin binary's provenance/build SHA has not been verified; unchanged `0.14.1` version text is not proof of identical binaries. No plugin build, installation, timer mutation or daemon restart occurred during this read-only observation. **Still not accepted:** valid candidate `NTPREVIEW CHECK 1` on real VDR, exact backend Agent/generation wiring, real public EPG admission, native CREATE dispatch/readback/uncertain outcome, or VIDAA hardware behavior. Do not reinstall or rebuild solely because the earlier historical CLOCK probe failed.

**Real yaVDR read-only NTPREVIEW handler observation (2026-10-19):** With VDR 2.8.3 running, `svdrpsend plug suitebridge NTPREVIEW CHECK 1` returned exactly `501 vdr-suite-ntpreview-rejected/1 invalid_or_stale`, as required by the intentionally incomplete request in `suitebridge_native_timer_preview_service.cpp`. The loaded module was observed in `/proc/<vdr-pid>/maps` as `/usr/lib/vdr/plugins/libvdr-suitebridge.so.14`. This confirms the NTPREVIEW command parser is reachable on the running plugin, **not** a valid signed specification, actual read-only VDR Timer/Device inventory, `isolated_clear`, or Public-v1 HTTP/timer CREATE acceptance. No production Timer was changed. Next read-only checks: determine genuine channel/timer evidence, exact plugin binary provenance, and build a valid bounded candidate without a write. No install or rebuild is justified by this handler probe alone.

**Real yaVDR pre-preview inventory (2026-10-19):** The operator's read-only `LSTC` queries confirmed channel 1 `Das Erste HD`, cable SID `10301`, NID `1`, TID `1051`, RID `0` (candidate canonical VDR channel ID `C-1-1051-10301`), and channel 2 `ZDF HD`, SID `11110`, NID `1`, TID `1079`, RID `0`. `LSTT` returned `550 No timers defined`. Running VDR maps SuiteBridge from `/usr/lib/vdr/plugins/libvdr-suitebridge.so.14`; `dpkg-query -S` reported no owning package for that path, and `sha256sum` reported `735497a2a62b8909b918600b6bb3a06e2b5fdfcfe392e05ea770a5b9e60598d3`. This hash is observational provenance, not a source SHA or deployment proof. A valid, digest-bound `NTPREVIEW CHECK 1` on this real channel is **not yet executed**; none of these reads establishes tuner availability or native conflict clearance. Do not claim a package provenance, run a build, or mutate VDR on the basis of this inventory.

**Real yaVDR valid native preview ACCEPTED (2026-10-19 05:43 VDR-local):** The operator verified `LSTC :ids 1` resolves `Das Erste HD` to the canonical VDR channel ID `C-1-1051-10301`, then issued a complete digest-bound `NTPREVIEW CHECK 1` against the running SuiteBridge process epoch `pie_0da79abd87186f230777a3d079255807`, candidate unix window `1792519200..1792522800` (2026-10-20 20:00–21:00 local). The **real running VDR 2.8.3** returned `900 vdr-suite-ntpreview/1 ... sha256:2637c89aa538f2b208fa07e24c0b5f92987536e74e3a8592f353104f4f6f0b6b ... 1792381404 sha256:fe55def60ee2138de415ab42b376cde5ec7a2dfea30ec4f93f2a1c1e7e7d26a7 0 2 isolated_clear`. Thus, at the observed snapshot, the plugin's complete read-only native Timer and Device inventory evaluated **zero existing Timers and two devices** and certified an isolated, compatible future one-shot candidate. This is a **positive native hardware conflict-preview acceptance**, not merely handler presence; its timestamp-bound proof expires and must be reacquired for each real EPG request. It is **not** evidence that the installed Suite daemon is on this source branch, that the EPG event/paired-Device HTTP contract succeeds, or that any native CREATE/readback took place. No Timer, build, install, or restart was performed. Remaining release gates are runtime source/binary parity, authenticated real EPG event admission, one separately approved safe CREATE with durable Operation/native readback and outcome-unknown handling, then VIDAA TV acceptance.

**Real yaVDR programme identity and running Daemon provenance (2026-10-19):** Immediately after the positive native preview, the operator called `svdrpsend LSTE 1 AT 1792519200`; the running VDR 2.8.3 returned channel `C-1-1051-10301`, event ID `13520`, title **Tagesschau**, start `1792519200` (2026-10-20 20:00 local) and duration `900` seconds (ends **20:15 local**). The prior `NTPREVIEW` was deliberately a **synthetic 20:00–21:00 `VDRSuitePreview` candidate**, not an EPG-derived preview for the Tagesschau: exact title, EPG duration, and native specification digest differ. Its `isolated_clear` result is real but cannot be replayed or promoted into real EPG admission. The running `vdr-suite-daemon` executable is `/usr/sbin/vdr-suite-daemon`, SHA-256 `3f0049ffbb6a0dde92d430cccec6d1d68a2c5861d47586fc3922e9c852152eba`, observed via `/proc/1907/exe`. This is binary identity only; build commit/source parity, runtime EPG event route, paired Device grant, Agent/provider ownership, authenticated GET, native CREATE and operation readback remain unverified. Next steps are **read-only** daemon provenance, listening endpoint and authorized public EPG event lookup; no automatic POST or unapproved installation.

**Real yaVDR EPG identity and running Suite daemon (2026-10-19 05:47 VDR-local):** Operator `svdrpsend LSTE 1 AT 1792519200` read the authoritative VDR EPG entry on `C-1-1051-10301` (`Das Erste HD`): native Event ID `13520`, title `Tagesschau`, Unix start `1792519200`, event duration **900 seconds**, thus EPG end `1792520100` (2026-10-20 20:15 VDR-local). The earlier positive 60-minute preview ending `1792522800` was a *synthetic one-shot candidate*, not the exact EPG event/recording specification; Public-v1 admission must independently re-attest its server-owned actual EPG schedule/defaults and cannot reuse the earlier snapshot revision. The operator read the running Daemon `/proc/1907/exe`, which resolves to `/usr/sbin/vdr-suite-daemon`, with SHA-256 `3f0049ffbb6a0dde92d430cccec6d1d68a2c5861d47586fc3922e9c852152eba`. This **binary checksum does not identify an installed source commit** or prove that consolidated EPG CREATE is live. Next bounded read-only gate: extract the installer's `git-<12hex>` embedded source marker and inspect the listener/public-v1 discovery path, without disclosing credentials; no POST, native CREATE, build, or deployment before explicit authorization.

**Client progress:** VIDAA now implements an explicit EPG programme → confirm → one-shot `POST /api/v1/epg/timer-intents` → deliberate `GET /api/v1/operations/{operationId}` path in source. `202` is an admission only; a lost or ambiguous result does not cause a second CREATE. Hisense deployment and hardware acceptance remain **PENDING**. Exact current GitHub HEAD, relevant source CI and package rehearsal must be checked before any next gate, rather than inferring success from the historical snapshots below.

## Historical 2026-10-18 standalone Slice-2 timeline (superseded as current branch status)

Backend work branch: `work/public-v1-epg-timer-intent-admission-20261017`, current source candidate `e17786ff6f6557a2cc7e5e06e82bca0fbb3351c3`. Last **fully verified** source CI: `911beb67502f679ed88fb0b934243540f00221d4` / [39118345979](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39118345979) (six jobs PASS). The new CLOCK evidence-assembly product commit `e17786ff` has its own [CI 39119610820](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39119610820); its final result must be checked before claiming source acceptance.

**Domain**: atomic EPG Intent + Assignment + Native CREATE Operation + idempotency mapping, exact rollback and replay, source-owned time guard are implemented and source-tested. **HTTP/Daemon**: authorized, read-only replay is now wired. **Productive EPG submit is NOT registered**; public CREATE remains unavailable / `503`. No authentic current proposed-timer conflict, capability/readback and backend-local offset proof has been composed in the runtime. Inspect the actual yaVDR/VDR/epgsearch read-only capabilities before completing this gate; no unsupported fake planner evidence, fallback to private API or unannounced VDR mutation.

**Installed**: no new yaVDR daemon; **TV**: no new Hisense recording acceptance; **merge/main**: no change. See [Public-v1 Coverage Roadmap](planning/public-v1-coverage-roadmap.md) and [EPG Timer intent contract](../api/epg-timer-intents.md).

### Native capability and VDR-local clock findings (2026-10-18)

Real yaVDR read-only observations: VDR **2.8.3**, epgsearch **2.4.6**, RESTfulAPI HTTP conflict endpoint **200**, `PLUG epgsearch LSCC` returns **901 no conflicts found**, native SuiteBridge **0.14.1** responds to `PLUG suitebridge NTCREATE CAP 1` with `vdr-suite-ntcreate-cap/1 ... timer-create enabled suitebridge ... 1 1 enabled`. The general `CAPS 1` discovery's `mutations: disabled` is a distinct general module capability and does not negate this operation-specific native timer CREATE capability. **Neither result proves conflict freedom for a proposed Timer.** The local checkout HEAD `c9f9a1160` differs from this working branch; no live daemon/plugin was installed by these changes.

Source work in `5ddc9d53ffb61fc9daac393254ba87e26f837cf9` adds a **read-only** `PLUG suitebridge NTCREATE CLOCK 1 <unix-start> <unix-stop>` command to the native plugin handler without changing `EXEC`. It returns exact VDR-process-local calendar/HHMM and UTC offsets for both requested times, plus SuiteBridge provider epoch/revisions. Backend Agent commit `ad4d2cd4f2e29c228244cceb6d5c0bc4c25b28af` accepts only a typed response for the exact request, local zone projection and selected provider fence. This new CLOCK command is **not present in the already installed 0.14.1 plugin** until an explicitly approved new plugin build/install. The command is not advertised in public SVDRP HELP because existing architecture security policy forbids advertising native CREATE there. The derived domain provider fence is bounded by SHA-256 in `7b38f93338db8df4c9d1333c17b3b70201114d94`, retaining the 160-byte planner limit.

The source and Agent parsing still require **a real server/Agent runtime wiring and a native proposed-timer conflict preview**, neither of which is supplied by epgsearch `LSCC`. Do not call `confirmedClear`, register the productive submit, activate VIDAA recording actions, or create a native Timer based solely on successful `CAP`/`CLOCK`. The new native plugin code has standalone host CI but not a completed yaVDR/VDR-header/plugin install acceptance. No new CREATE is available on the running daemon.

### Slice-2 backend CLOCK assembly and conflict-interface audit (2026-10-18)

Source commit [`e17786ff`](https://github.com/hotzenplotz5/vdr-suite/commit/e17786ff6f6557a2cc7e5e06e82bca0fbb3351c3) adds `EpgNativeTimerClockAttestation.h`: a checked translation of a previously **typed, backend-originated** SuiteBridge CLOCK projection into backend-/generation-/provider-fenced `ServerOwnedNativeClockEvidence`. Source regression cases reject a mismatched native HHMM, restarted provider, stale Agent generation and wrong requested window. **The helper is not a live data producer and does not register `EpgTimerIntentSubmit`.** CI is separately tracked at [39119610820](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39119610820).

An external source check of [epgsearch SVDRP](https://manpages.debian.org/testing/vdr-plugin-epgsearch/epgsearch.4.en.html) and its [public service interface](https://github.com/vdr-projects/vdr-plugin-epgsearch/blob/master/services.h) confirms `LSCC` / `TimerConflictList` list conflicts for **currently configured** timers. The conflict checker internally clones timers, but its published interface does **not** accept a candidate Timer to simulate. It cannot produce the required `proposedTimerIncluded + allExistingTimersConsidered + devicesConsidered` evidence. No "clear" proof or VDR mutation may be inferred from `901 no conflicts found`, `NTCREATE CAP`, `NTCREATE CLOCK`, or HTTP `200` on the existing conflict endpoint.

Outstanding, **not accepted**: backend-native prospective Timer/device simulation; productive server-owned EPG/Agent/provider/clock/planner composition; registration of `EpgTimerIntentSubmit` only after all real proofs; live VDR-header/SuiteBridge CLOCK build and read-only acceptance; exact final-head CI and native CREATE/readback/outcome-unknown/hardware acceptance. HTTP CREATE remains fail-closed, VIDAA recording action remains disabled. No root change, plugin deployment, daemon restart, CREATE or merge was performed.

### Local installed CLOCK rejection and source hardening (2026-10-18)

Read-only real yaVDR probe at the locally installed checkout `c9f9a116071cf4cf4679e7bd9f39d345cd963d4d` (branch `work/mu10e3-device-credential-rotation`, **not** the Public-v1 Slice-2 source) returned **900 enabled** for `PLUG suitebridge NTCREATE CAP 1`, general `CAPS 1` with `mutations: disabled`, and **501 `vdr-suite-ntcreate-rejected/1 unsupported`** for `PLUG suitebridge NTCREATE CLOCK 1 1792222800 1792226400`. The RESTfulAPI read-only endpoint returned HTTP 200. These outcomes are consistent with an installed SuiteBridge 0.14.1 lacking the newly added CLOCK verb. They do not prove proposed-timer conflict freedom, and no install or VDR mutation was performed.

Source commit `6f736d78` adds a single typed raw CLOCK-reply-to-domain-attestation entry that rejects 501, stale Provider epochs and expired Agent leases; `24111adb` relocates its transport-backed regression to the approved Timer-domain test boundary rather than widening the legacy Phase-64 architecture allowlist. The branch remains **source-only**, with productive EPG CREATE disabled. Exact-head CI: [39121803291](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39121803291), check its final conclusion separately.

### Slice-2 native prospective conflict decision kernel (2026-10-18)

Source commit [`54199108`](https://github.com/hotzenplotz5/vdr-suite/commit/541991083992bf71974b4bcf331a47686297b3cc) introduces a **pure, non-mutating** conservative native Timer-preview decision kernel and focused C++ regressions (`test-slice2-native-timer-preview-kernel`, part of `test-fast`). It can classify a single one-shot as a *potentially clear candidate* only when a complete native Timer snapshot contains no overlapping active timer and a complete device inventory reports a non-preempting, channel-compatible device. Recurrences, overlaps, encrypted channels, unknown future device capacity, invalid or incomplete snapshots all fail closed. This is deliberately not a clone of epgsearch's multi-device scheduling simulation.

**Explicit boundary:** `candidateClear` is a local, untrusted kernel classification, **not** `ProspectiveNativeTimerConflictProof`, `confirmedClear`, a live VDR attestation or permission for CREATE. There is no SuiteBridge VDR producer, provider-bound wire verification, generation-fenced projection, or Daemon submit registration for it yet. Existing `EpgTimerIntentAtomicAdmissionService` therefore cannot accept a new public CREATE via this kernel. The old installed yaVDR plugin remains unchanged. [Kernel CI](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39123447566) must be independently checked; docs-only next head needs its own final CI check for full acceptance.

### Slice-2 native read-only prospective candidate producer (2026-10-18)

Source series `54199108` through `97245f5e` now includes a bounded C++ native VDR snapshot producer, a separate `NTPREVIEW CHECK 1` SuiteBridge SVDRP handler, a typed Backend-Agent transport verifier, and a conversion helper that binds the already verified prospective result to the full native Timer specification, Agent lease/generation and selected Provider epoch/fence. The source snapshot code uses `GetTimersRead`, `GetChannelsRead`, indexed `cDevice::GetDevice(index)`, `ProvidesTransponder`, `MaySwitchTransponder` and a repeated Timer state check; it does **not** write Timers, tune devices, update CAM assignments or invoke Native CREATE. Its conflict checks cover the complete VDR-minute-rounded candidate interval, including subminute edges. The native proof age is counted from the VDR observation time rather than restarted by the receiving Agent. The preview engine only returns `isolated_clear` for a complete read-only native inventory with no active overlapping Timer and at least one unoccupied, compatible device for an exactly projected free-to-air one-shot. Recurrences, overlaps, encrypted channels, races, unavailable device evidence and incomplete snapshots return **unknown**, not `confirmedClear`. A bounded rolling SHA-256 record covers all inspected facts up to 4096 Timers.

**2026-10-18 native audit hardening:** Upstream VDR 2.8.3 `cDevice::GetDevice(channel, ..., Query=true)` can release an inactive CAM slot despite `Query=true`; `cDvbDevice::ProvidesChannel` also modifies its bonded-receiver flag while `const`. Neither method is called in the revised native preview. Source commit [`cd39c4e5`](https://github.com/hotzenplotz5/vdr-suite/commit/cd39c4e5752fd8dad205148ad021bee929476292) passed pinned VDR 2.8.3 header CI [39127037496](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39127037496). Minute-window and boundary regression source commit [`e500da40`](https://github.com/hotzenplotz5/vdr-suite/commit/e500da40a3ae57b3b8f4e2fd2b8d3d0898f6c4b7) also passed [pinned native header CI 39127157793](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39127157793); [full source CI 39127157979](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39127157979) must be checked independently. This is **not** an installed-plugin, production read-only, scheduler or hardware acceptance. Public CREATE remains disabled and VIDAA not accepted.

**Limitations / release gate:** This is a strictly conservative prospective subset, not epgsearch's full tuner-allocation scheduler. A source mock may validate the parser, but its success must NEVER be treated as live VDR evidence. The new native VDR object and SuiteBridge entrypoint/service files now **pass syntax compilation against the exact official VDR 2.8.3 release headers**, pinned to upstream commit `339b9969b0cfaf27d3941f135e17a09b984cd094`, in [native CI 39126057133](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39126057133) on source commit `afb50c4b`. This proves source compatibility only: the plugin is **not built as a deployed binary**, installed, or accepted on real hardware. The existing SuiteBridge on yaVDR still returns `501 unsupported` for `NTCREATE CLOCK`. The Daemon has **not** registered the productive `EpgTimerIntentSubmit` callback, and no public EPG Timer CREATE is enabled. A final full-head CI acceptance plus native build/read-only preview acceptance, exact backend-scoped Daemon composition, Native CREATE/readback/outcome-unknown tests and public HTTP acceptance remain mandatory. Relevant source CI [39125145695](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39125145695) must be checked at the exact head.

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase 69 Kickoff](development/phase-69-public-api-kickoff.md)
- [Phase 69 Closeout](development/phase-69-closeout.md)
- [Post-Phase-69 Multiuser Productization Workstream](development/post-phase69-multiuser-workstream.md)
- [ADR-0067 Human Account and Backend Access Administration](adr/ADR-0067-human-account-backend-access-administration.md)
- [MU.5 Administration Architecture Acceptance](development/post-phase69-mu5-administration-architecture-acceptance.md)
- [MU.6A Human Account Lifecycle Authority Foundation](development/post-phase69-mu6-account-lifecycle-foundation.md)
- [MU.6B Public Account Item + Revision/ETag](development/post-phase69-mu6b-public-account-item.md)
- [MU.6C Public Account Lifecycle Mutation](development/post-phase69-mu6c-public-account-lifecycle-mutation.md)
- [MU.8A Safe Credential / Session Metadata](development/post-phase69-mu8a-credential-session-metadata.md)
- [MU.8B Session Revoke](development/post-phase69-mu8b-session-revoke.md)
- [MU.8C Human-password Credential Revoke](development/post-phase69-mu8c-credential-revoke.md)
- [MU.9A Account Administration Read UI](development/post-phase69-mu9a-account-admin-read-ui.md)
- [MU.9B Account Lifecycle Mutation UI](development/post-phase69-mu9b-account-lifecycle-ui.md)
- [MU.9C Session Revoke UI](development/post-phase69-mu9c-session-revoke-ui.md)
- [MU.9D Human-password Credential Revoke UI](development/post-phase69-mu9d-credential-revoke-ui.md)
- [MU.9E Backend Grant Mutation UI](development/post-phase69-mu9e-grant-mutation-ui.md)
- [MU.9F Account CREATE UI](development/post-phase69-mu9f-account-create-ui.md)
- [MU.9 Closeout](development/post-phase69-mu9-closeout.md)
- [Post-MU.9 Administration Usability](development/post-phase69-multiuser-administration-usability.md)
- [MU.10B Administrator Pairing Approval](development/post-phase69-mu10b-admin-pairing-approval.md)
- [MU.10C1 Device Credential Issuance](development/post-phase69-mu10c-device-credential-issuance.md)
- [MU.10D1 Device Authentication](development/post-phase69-mu10d-device-authentication.md)
- [MU.10E1 Device Grants](development/post-phase69-mu10e1-device-grant-administration.md)
- [MU.10E2 Device/Credential Revocation](development/post-phase69-mu10e2-device-revocation.md)
- [MU.10E3 Credential Rotation](development/post-phase69-mu10e3-device-credential-rotation.md)
- [Post-Phase-69 P2 Legacy Basic Retirement Closeout](development/post-phase69-p2-legacy-basic-retirement-closeout.md)
- [Phase 69.B Closeout](development/phase-69b-closeout.md)
- [Phase 69.C Closeout](development/phase-69c-closeout.md)
- [Phase 69.D Closeout](development/phase-69d-closeout.md)
- [Phase 69.E Closeout](development/phase-69e-closeout.md)
- [Phase 69.F Client Error and Mutation-Fallback Safety](development/phase-69f-client-error-mutation-fallback.md)
- [Phase 69.F SearchTimer Client Fallback Removal](development/phase-69f-searchtimer-fallback-removal.md)
- [Phase 69.F Same-Handler Read Alias Fallback Removal](development/phase-69f-read-alias-fallback-removal.md)
- [Phase 69.F Client Contract Matrix](development/phase-69f-client-contract-matrix.md)
- [Phase 69.F Public Backend Collection](development/phase-69f-public-backend-collection.md)
- [Phase 69.F Home EPG Single-Route Hardening](development/phase-69f-home-epg-fallback-removal.md)
- [Phase 69.F Timer Live Single-Route Hardening](development/phase-69f-timer-live-fallback-removal.md)
- [Phase 69.F Public-v1 Discovery Reference Client](development/phase-69f-public-v1-discovery-reference-client.md)
- [Phase 69.F Public-v1 Channel Reference Client](development/phase-69f-public-v1-channel-reference-client.md)
- [Phase 69.F Public-v1 TimerAssignment Collection Reference Client](development/phase-69f-public-v1-timer-assignment-collection-reference-client.md)
- [Phase 69.F Public-v1 TimerAssignment Item Reference Client](development/phase-69f-public-v1-timer-assignment-item-reference-client.md)
- [Phase 69.F Public-v1 Timer CREATE Admission Reference Client](development/phase-69f-public-v1-timer-create-reference-client.md)
- [Phase 69.F Public-v1 Operation Reference Client](development/phase-69f-public-v1-operation-reference-client.md)
- [Phase 69.E Compatibility Policy Foundation](development/phase-69e-compatibility-policy-foundation.md)
- [Phase 69.E Retained Legacy Route Classification](development/phase-69e-legacy-route-classification.md)
- [Phase 69.D first public collection](development/phase-69d-timer-assignment-collection.md)
- [Public Timer CREATE Admission](development/phase-69c-public-timer-create-admission.md)
- [Native Timer CREATE Reconciliation Runtime Composition](development/phase-69c-native-timer-create-reconciliation-runtime.md)
- [Native Timer CREATE Outcome Evidence](development/phase-69c-native-timer-create-outcome-evidence.md)
- [Native Timer CREATE Outcome Application](development/phase-69c-native-timer-create-outcome-application.md)
- [Native Timer CREATE Readback Reconciliation](development/phase-69c-native-timer-create-readback-reconciliation.md)
- [Native Timer CREATE Productive Runtime](development/phase-69c-native-timer-create-productive-runtime.md)
- [Phase Map](planning/phase-map.md)
- [Golden User Journeys](planning/golden-user-journeys.md)
- [Current Project Status](development/current-status.md)
- [Phase 68 Closeout](development/phase-68-closeout.md)
- [Phase 68 Kickoff](development/phase-68-legacy-osd-kickoff.md)
- [Phase 68.D View-Session Closeout](development/phase-68d-view-session-authorization-closeout.md)
- [Phase 68.E Viewer-Binding Closeout](development/phase-68e-viewer-bindings-closeout.md)
- [Phase 67 Closeout](development/phase-67-closeout.md)
- [Phase 67 Teletext Closeout](development/phase-67-teletext-closeout.md)
- [Phase 66 Closeout](development/phase-66-closeout.md)
- [Post-Phase-66 Home Rebuild Closeout](development/post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Recording Detail Closeout](development/post-phase66-recording-detail-closeout.md)
- [Post-Phase-66 Home Performance Hardening](development/post-phase-66-home-performance-hardening.md)
- [Post-Phase-66 Native Recording Editing Closeout](development/post-phase66-recording-editing-closeout.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](development/phase-65d1-persistent-browser-playback-shell-closeout.md)
- [ADR-0054 Broadcast Companion Services](adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0058 Media Home](adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR-0063 Mutation Complexity Proportionality and Reuse](adr/ADR-0063-mutation-complexity-proportionality-reuse.md)
- [ADR Index](adr/index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Accepted Home rebuild merge checkpoint:
ea5967b983aee9ccc3f855b685db01abbfb2326a

Latest accepted post-phase runtime merge checkpoint:
2b0d0990974244eac90e9be711843678c77e301d

Latest completed numbered runtime phase:
Phase 69 - Public API and Client Compatibility Hardening

Current active numbered runtime phase:
none - Phase 70 - Recommendation and Content Knowledge Graph not started

Next strict numbered runtime phase:
Phase 70 - Recommendation and Content Knowledge Graph

Completed Phase-67 verticals:
Teletext read path / browser-TV view / page navigation
HbbTV discovery / authorized application session / presentation-media runtime

Accepted Phase-68 verticals:
68.A - Read-only semantic OSD observation
68.B - Agent-local buffering and resynchronization
68.C - Authenticated read-only Agent OSD transport
68.D - Authorized bounded Legacy OSD view sessions
68.E - Bounded viewer bindings and multi-viewer delivery
68.F - Exclusive controller lease and osd.control fencing
68.G - Allowlisted native OSD input

Current active numbered runtime slice:
none - Phase 70 - Recommendation and Content Knowledge Graph not started

Current active cross-cutting productization workstream:
Multiuser / Account and Backend Access Administration

Current Multiuser architecture slice:
MU.5 - Administration architecture contract - ADR-0067 [COMPLETED]

Latest source-merged Multiuser Device scope:
MU.10C1, MU.10D1 and MU.10E1-E3 [MERGED — PR #440-#444; latest MU.10E3 PR #444]. Repository integration is not real yaVDR/VIDAA acceptance.

Latest separately demonstrated real yaVDR Multiuser acceptance:
MU.10B Administrator Pairing Approval [ACCEPTED - PR #437 / REAL YAVDR PASS / HOSTED CI #9801 GREEN]

Historical completed Post-MU.9 acceptance marker (retained for the established static UX guard, not a current successor):
Post-MU.9 Administration Usability [MERGED - PR #433 / REAL YAVDR PASS / HOSTED CI GREEN]

Historical completed MU.9 UI slice markers:
MU.9E - Backend Grant Mutation UI [IMPLEMENTATION MERGED - PR #429 / FOCUSED YAVDR PASS / POST-MERGE CI #9763 GREEN]
MU.9F - Account CREATE UI [IMPLEMENTATION MERGED - PR #430 / FOCUSED YAVDR PASS / HOSTED CI GREEN]

Completed MU.7 sub-slice:
MU.7 - Grant-set read + desired-state grant mutation [COMPLETED - REAL YAVDR PASS / PR #413 MERGED]

Current cross-cutting acceptance boundary:
MU.10C1-E3 are source-merged; their real yaVDR/VIDAA installation and end-to-end acceptance remain separate from MU.10B's proven server acceptance. Recheck the live host before claiming deployment.

Post-Phase-69 Multiuser productization status:
MU.0 through MU.7 are completed. MU.6D Atomic Account CREATE + durable idempotency passed real yaVDR acceptance and merged as PR #411.
ADR-0067 remains the accepted architecture. MU.7 completed the bounded grant-administration boundary: secret-free grant-set read with deterministic ETag, global accounts.grants.view/modify authority, desired-state ensure/revoke, explicit server-owned permission/scope allowlist, browser CSRF and final-usable-administrator protection. REAL YAVDR acceptance passed; PR #413 merged. MU.8 implementation is complete through MU.8C. MU.8A merged as PR #415 but its separately documented real-runtime acceptance remains pending. MU.8B passed focused yaVDR acceptance and merged as PR #416. MU.8C passed focused yaVDR acceptance, merged as PR #417, and both its PR CI and post-merge main CI are green. MU.9A read-only Account administration UI merged as PR #418 after exact merged-main yaVDR acceptance; PR and post-merge hosted CI are green. MU.9B Account Lifecycle Mutation UI merged as PR #420 after real yaVDR acceptance; PR and post-merge hosted CI are green. MU.9C Session Revoke UI merged as PR #426 after focused yaVDR acceptance and green hosted PR CI. MU.9D Human-password Credential Revoke UI merged as PR #428 after focused yaVDR acceptance; PR and post-merge hosted CI are green. MU.9E Backend Grant Mutation UI merged as PR #429 after focused real-yaVDR acceptance. GitHub-hosted runner degradation interrupted queued jobs without executing them; post-merge main CI run #9763 ultimately completed successfully with all six jobs green. MU.9F Account CREATE UI passed focused real-yaVDR acceptance, merged as PR #430, and both PR CI #9764 and post-merge main CI #9765 completed with all six jobs green. MU.9A-F and the MU.9 closeout are complete; no MU.9G is selected. Post-MU.9 Administration Usability is complete. MU.10A Device Pairing Bootstrap passed real yaVDR runtime and real VIDAA first-consumer acceptance, merged as PR #436, and post-merge main CI #9779 completed successfully with all six jobs green. MU.10B Administrator Pairing Approval passed the exact real-yaVDR server acceptance gate. Pending-list/item reads, strong ETag/If-Match, browser CSRF, approve/reject, stale/already-decided/expired conflicts, token-scoped post-decision polling, SQLite lifecycle, verifier hashing and accountability all passed; Actor/Device/Credential/Session/Grant counts remained unchanged. PR #437 hosted CI #9801 completed successfully with all six jobs green.
Subsequent verified source merges: [MU.10C1 PR #440](https://github.com/hotzenplotz5/vdr-suite/pull/440) (credential issuance), [MU.10D1 PR #441](https://github.com/hotzenplotz5/vdr-suite/pull/441) (Device authentication), [MU.10E1 PR #442](https://github.com/hotzenplotz5/vdr-suite/pull/442) (scoped grants), [MU.10E2 PR #443](https://github.com/hotzenplotz5/vdr-suite/pull/443) (revocation) and [MU.10E3 PR #444](https://github.com/hotzenplotz5/vdr-suite/pull/444) (rotation). Each exact PR head has successful hosted CI (#9839, #9850, #9876, #9879 and #9904 respectively); these tests do not substitute for real yaVDR or VIDAA acceptance. The corresponding original MU.10C-E documents retain their time-bound development checkpoints.
Durable workstream status: [Post-Phase-69 Multiuser Productization Workstream](development/post-phase69-multiuser-workstream.md).
Durable MU.5 evidence: [MU.5 Administration Architecture Acceptance](development/post-phase69-mu5-administration-architecture-acceptance.md).
Durable MU.6A evidence: [MU.6A Human Account Lifecycle Authority Foundation](development/post-phase69-mu6-account-lifecycle-foundation.md).
Durable MU.6B evidence: [MU.6B Public Account Item + Revision/ETag](development/post-phase69-mu6b-public-account-item.md).
Completed MU.6C evidence: [MU.6C Public Account Lifecycle Mutation](development/post-phase69-mu6c-public-account-lifecycle-mutation.md). Completed MU.6D evidence: [MU.6D Atomic Account CREATE + Durable Idempotency](development/post-phase69-mu6d-account-create-idempotency.md). Completed MU.7 evidence: [MU.7 Backend Access / Permission Grant Administration](development/post-phase69-mu7-account-grant-administration.md). MU.8A implementation evidence: [MU.8A Safe Credential / Session Metadata](development/post-phase69-mu8a-credential-session-metadata.md). MU.8B merged evidence: [MU.8B Session Revoke](development/post-phase69-mu8b-session-revoke.md). MU.8C merged evidence: [MU.8C Human-password Credential Revoke](development/post-phase69-mu8c-credential-revoke.md). MU.9A merged evidence: [MU.9A Account Administration Read UI](development/post-phase69-mu9a-account-admin-read-ui.md). MU.9B merged evidence: [MU.9B Account Lifecycle Mutation UI](development/post-phase69-mu9b-account-lifecycle-ui.md). MU.9C merged evidence: [MU.9C Session Revoke UI](development/post-phase69-mu9c-session-revoke-ui.md). MU.9D selected evidence: [MU.9D Human-password Credential Revoke UI](development/post-phase69-mu9d-credential-revoke-ui.md).

69.D is completed. [Phase 69.D Closeout](development/phase-69d-closeout.md)
records the accepted collection boundary:
PR #363 established the standard single-source collection envelope, stable
ordering, bounded keyset pagination and explicit source failure; PR #364 added
the genuine federated Channel read with independently authorized backend
sources, explicit partial-source metadata and 503 when all requested sources
fail. The accepted contracts remain documented in
[Phase 69.D first public collection](development/phase-69d-timer-assignment-collection.md)
and [Phase 69.D federated Channel collection](development/phase-69d-public-channel-federation.md).

69.E is completed. [Phase 69.E Closeout](development/phase-69e-closeout.md)
records the accepted compatibility/deprecation boundary: PR #366 publishes the
machine-readable compatibility policy and deprecation-metadata capability; PR
#367 classifies all 118 retained unversioned route literals into 27 proven
same-handler alias groups / 54 alias members and 64 standalone transition
literals, with 0 deprecated aliases because no classified alias has a stabilized
public-v1 successor.

69.F is completed. It owns the accepted first-/third-party client hardening boundary: common client error
representation, removal of fallback probing after arbitrary errors, wrapper
migration to genuine stabilized v1 contracts and a documented stable boundary
for browser, TV, mobile, desktop and Kodi integrations. The
[Phase 69.F Client Error and Mutation-Fallback Safety](development/phase-69f-client-error-mutation-fallback.md)
candidate is the first bounded 69.F slice: it preserves structured HTTP error
evidence in the browser client and removes alternate-path retries from the four
proven state-changing SearchTimer mutations. The
[Phase 69.F SearchTimer Client Fallback Removal](development/phase-69f-searchtimer-fallback-removal.md)
candidate is the second bounded 69.F slice: the bundled browser now uses one
existing SearchTimer route per operation, removes the speculative
`/api/vdr/searchtimers/live` probe and no longer retries through
`/api/searchtimers...` aliases. The
[Phase 69.F Same-Handler Read Alias Fallback Removal](development/phase-69f-read-alias-fallback-removal.md)
candidate is the third bounded 69.F slice: Overview, Persons and
RecordingPersons now use one existing primary route because their alternate
spellings are proven same-handler aliases. The
[Phase 69.F Client Contract Matrix](development/phase-69f-client-contract-matrix.md)
candidate is the fourth bounded 69.F slice: all current browser Client API
operations are classified against the deliberately smaller stable public-v1
surface and browser-private helpers are excluded from external compatibility.
The [Phase 69.F Public Backend Collection](development/phase-69f-public-backend-collection.md)
is the accepted fifth bounded 69.F slice via PR #373: it stabilizes actor-
filtered `GET /api/v1/backends` with a minimal public representation,
authorization-scope-bound keyset pagination and no new Backend-specific
permission model. The richer first-party `/api/backends` route remains pre-v1
and is not deprecated or silently substituted in the browser.

The [Phase 69.F Home EPG Single-Route Hardening](development/phase-69f-home-epg-fallback-removal.md)
is the accepted sixth bounded 69.F slice via PR #374. It retires the manual
Home EPG arbitrary-error fallback from `/api/epg/now-next?from=-1` to the
semantically distinct `/api/vdr/events` route while preserving fail-soft
empty-event behavior and without rebuilding Home/LiveTV lifecycle.

The [Phase 69.F Timer Live Single-Route Hardening](development/phase-69f-timer-live-fallback-removal.md)
is the accepted seventh bounded 69.F slice via PR #375. It removes the final
catch-all browser route fallback while preserving the server-owned explicit
live-service-unavailable Timer snapshot decision.

The [Phase 69.F Public-v1 Discovery Reference Client](development/phase-69f-public-v1-discovery-reference-client.md)
is the accepted eighth bounded 69.F slice via PR #376. It establishes the
browser-independent JavaScript reference seam for `GET /api/v1`,
`GET /api/v1/capabilities` and `GET /api/v1/backends` without migrating the
richer bundled browser.

The [Phase 69.F Public-v1 Channel Reference Client](development/phase-69f-public-v1-channel-reference-client.md)
is the accepted ninth bounded 69.F slice via PR #377. It extends the independent
reference seam with the stable federated Channel collection while preserving
explicit source selection and partial-result semantics.

The [Phase 69.F Public-v1 TimerAssignment Collection Reference Client](development/phase-69f-public-v1-timer-assignment-collection-reference-client.md)
is the accepted tenth bounded 69.F slice via PR #378. It extends the independent
reference seam with the stable single-backend TimerAssignment keyset collection
without native Timer fallback or collection ETag.

The [Phase 69.F Public-v1 TimerAssignment Item Reference Client](development/phase-69f-public-v1-timer-assignment-item-reference-client.md)
is the accepted eleventh bounded 69.F slice via PR #379. It adds the accepted
revisioned TimerAssignment item GET with opaque ETag exposure and
`If-None-Match -> 304`.

The [Phase 69.F Public-v1 Timer CREATE Admission Reference Client](development/phase-69f-public-v1-timer-create-reference-client.md)
is the accepted twelfth bounded 69.F slice via PR #380. It adds caller-keyed
Timer CREATE admission and returns the durable `operationId`/Location without
automatic retry or polling.

The [Phase 69.F Public-v1 Operation Reference Client](development/phase-69f-public-v1-operation-reference-client.md)
is the accepted thirteenth bounded 69.F slice via PR #381. It adds the final
stable public-v1 contract to the independent reference seam: one actor-owned,
revisioned durable Operation GET with opaque ETag and `If-None-Match -> 304`.

[Phase 69 Closeout](development/phase-69-closeout.md) records the completed
69.A-F acceptance boundary. All eight stable public-v1 method/resource contracts
are exactly covered by accepted reference-client methods, every explicit browser
route fallback inventoried by 69.F is retired, and no additional Phase-69
runtime gap is justified. Phase 70 remains not started and requires its own
accepted runtime ADR before implementation.

69.C is completed. Its closeout records the accepted public revision/ETag,
strong If-Match, durable Idempotency-Key and no-blind-fallback contracts, plus
the exact-head real yaVDR Timer CREATE acceptance from PR #361.

Architecture guard:
ADR-0063 now makes mutation complexity proportional to the concrete authority
and failure model. The Timer CREATE orchestration chain is a high-complexity
reference case, not a mandatory template for simpler mutations such as native
Recording marks. Reuse common mutation guarantees; add Timer-like lifecycle
layers only when their distinct failure/authority responsibility is proven.

ADR-0064 internal control-plane status:
Critical Live/native-probe, Legacy OSD, HbbTV/Teletext and
RMETA/META/ARTW/ETYPES have completed the local prioritized transport migration.
The latency-isolation objective is closed. Timer CREATE/DELETE/MODIFY remain on
typed SVDRP by design; RMARKS/RCUT remain serialized with NMARKS/NCUT on SVDRP
because splitting Recording editing across transports would introduce
unproven marks-file/cut-state concurrency. MCOMPARE remains diagnostic-only.
A future mutation migration requires a new operation-specific architecture
slice; it is not unfinished Phase 69 or unfinished ADR-0064 foundation.

Accepted Phase-69.C checkpoints:
PR #321 -> beb98f6edab39d962bd6415db7be21cf145e05cb
PR #321 hosted CI run 35873370127 / #9091: SUCCESS (6/6)
PR #322 -> 377f59e7b2d8e5ec3c7cbf22522c3c1789fd9849
PR #322 hosted CI run 35876551406 / #9094: SUCCESS (6/6)
PR #323 -> 50c51ea0fab08cbae8149ad33136dec869c6c3b0
PR #324 -> b971b074725351004d676c001ee976f574587390
PR #325 -> 92c127fbe40d8d20ac2294e1b4b28fd3ea3a6560
PR #326 -> 61f933d1ae23aaa26d0ff3715e68e9686450ee7f
PR #326 hosted CI run 35895657815 / #9103: SUCCESS (6/6)
PR #327 -> b80003de33127827fc121d7d92d7f11c6f4451e6
PR #327 hosted CI run 35899013477 / #9105: SUCCESS (6/6)
PR #328 -> 6d822db653093ab5f513b8106929cfcf02fa85af
PR #328 hosted CI run 35902606474 / #9108: SUCCESS (6/6)
PR #329 -> b5ef507a94e4c36a2472d1697c4c973efefe01f0
PR #329 hosted CI run 35907951511 / #9119: SUCCESS (6/6)
PR #330 -> 27d040ec362264bdd3de01b1b8b65c8301269748
PR #330 hosted CI run 35940968821 / #9124: SUCCESS (6/6)
PR #331 -> ac7807c0fce364ab5c918dfa5d1f28427e928db3
PR #331 hosted CI run 35943072158 / #9126: SUCCESS (6/6)
PR #332 -> 1f19726a0b387a7b1de3f078dbf836a51254e724
PR #332 accepted head c1d156408fd3895a33499635ec5475340d6d049b
PR #332 hosted CI run 35960066328 / #9130: SUCCESS (6/6)
PR #333 -> f3363f0ec88db2cbeef6dea24ab4349738094ade
PR #333 accepted head 2f74fb86aaaf6b969eea5822fb807f7f0002585a
PR #333 hosted CI run 35964190859 / #9134: SUCCESS (6/6)
PR #334 -> 94dcffa8fc77b9087472ec09056853819515308a
PR #334 accepted head c4b0be094f69d74e68787f3487d678af4c783550
PR #334 hosted CI run 35967848509 / #9143: SUCCESS (6/6)
PR #341 -> 54ee022361bd796529578de46edb3d5a9a8bec4a
PR #341 accepted head ed1cc9739351045e0824530273a93a55700f5e5d
PR #341 hosted CI run 36094237108 / #9205: SUCCESS (6/6)
PR #342 -> d1e543b977da45f732c768f4efdf8877910dd90e
PR #342 accepted head 78880a00e0cc330b7375ee24086162a89e6cd5ac
PR #342 hosted CI run 36132407130 / #9210: SUCCESS (6/6)
PR #361 -> a38e2363929c2a1f1b60984f6176aa88446a9054
PR #361 accepted head 9316fb48a49f3e61ff89d3c01a225915003fa8e8
PR #361 hosted CI run 36275800779 / #9285: SUCCESS (6/6)
PR #361 real yaVDR acceptance: PR361_REAL_YAVDR_ACCEPTANCE=PASS
69.C=COMPLETED

Phase-69.B closeout checkpoint:
PR #320 -> c4b9fc66d0f1286e72e82406ffd1f49acf4b331a
PR #320 hosted CI run 35864935626 / #9089: SUCCESS (6/6)

Accepted Phase-69.A checkpoints:
PR #314 -> d34058220653496a35fb7e1c073cc6981f4c72db
PR #315 -> bca8ff3bfd1216bda39f29105911f64853bd263c
PR #315 hosted CI run 35818928861 / #9074: SUCCESS (6/6)

Accepted Phase-69.B checkpoints:
PR #316 -> 72a637cc6f18fc1c1c1a77859abb3ab1f15e2b03
PR #316 hosted CI run 35843808669 / #9081: SUCCESS (6/6)
PR #317 -> 5ff36d635072c63e1c46ccb377c830ec99988108
PR #317 hosted CI run 35846333459 / #9083: SUCCESS (6/6)
PR #318 -> e9d59b87349d7e1be9356efc5cd656f41e11ea12
PR #318 hosted CI run 35861961215 / #9085: SUCCESS (6/6)
PR #319 -> 0248db3d63626a87d391f7649a984adc97b45232
PR #319 hosted CI run 35863518112 / #9087: SUCCESS (6/6)
69.B=COMPLETED

Latest accepted Phase-68 transport merge checkpoint:
PR #309 -> ed2f451d824e43c4264dee6caf8ae49dbeb71a88

Phase-68.D accepted candidate:
PR #310 -> 8807d587536daa6c27ae72a3f88d40b0c7de3480
Hosted CI run 35594914471: SUCCESS
real yaVDR acceptance: PASS

Phase-68.E accepted runtime candidate:
PR #311 -> 2b74b26ed33ce1a555a9cc037d44ad6768480eb2
Hosted CI run 35601863603: SUCCESS (6/6)
real yaVDR acceptance: RESULT=PHASE68E_REAL_ACCEPTANCE_PASS

Phase-68.F merge checkpoint:
PR #312 -> e3f9215f5f80dd230e1e855e3ce09f2ac70231ef
Hosted CI run 35610473828: SUCCESS

Phase-68.G accepted real-runtime candidate:
PR #313 runtime candidate -> 7ae51d090cbe06570b8a70e787137232df83f124
real yaVDR acceptance: RESULT=PHASE68G_REAL_NATIVE_OSD_INPUT_PASS

Latest Phase-67 Teletext merge checkpoint:
PR #293 -> d92e7907637122368908ce4c7564a0332db9c487

Phase-67 HbbTV / numbered closeout merge checkpoint:
PR #300 -> 5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30
```

The accepted post-Phase-66 Home rebuild branch `work/home-rebuild` ended at `0cce4d1c9e58abe4d529132e92340ae4cbb7a99c` and was merged to `main` as `ea5967b983aee9ccc3f855b685db01abbfb2326a`. The merge tree is identical to the accepted branch tree. This SHA is a durable Home-rebuild checkpoint, not a substitute for reading the live `main` head.

The later accepted post-phase runtime chain includes the Recordings 2 cinematic detail/presentation work, the related-Genre portrait correction, the Live-TV `ended` lifecycle stabilization, and the subsequent Recording-detail hardening through clickable cast/person search, corrected metadata navigation, strict paused prewarm and inline YouTube trailers. The current live `main` head must still be queried before repository-state actions; the runtime checkpoint above is historical evidence, not a permanent branch tip.

## Post-Phase-69 Recording R2 and VIDAA Device status — `main` as of 2026-10-11

The verified `main` source head for this documentation comparison is `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`, the merged R2 Recording Device audio/index/SRT/WebVTT integration. This is **repository source identity only**: it does not establish the running yaVDR daemon binary, selected TV build, a completed hardware smoke test or deployment approval.

- The R2 Recording successor now has merged `main` code for canonical backend-scoped Recording collections, bounded folder/genre browse, `durationKnown` and public EPG now/next. The Phase-69.F accepted public-contract baseline remains closed; later additions must be validated independently and must **not** be silently relabelled as accepted Phase 69.F references.
- Device-owned `POST /api/v1/recording-playback-sessions`, owned `GET` status and STOP use Device proof, backend-scoped `media.recording.play` authorization, canonical MediaSession/Gateway ownership and reauthorization of protected media artifacts. Browser credentials or `recordings.view` alone do not grant Device playback control.
- The merged Device status projection reports only verified index-backed duration and bounded index-preparing state. The VDR index updater refuses existing `index`, `index.vdr` and symlink paths rather than invoking the potentially destructive VDR 2.7.9 generator over them.
- Device-owned tracks and subtitle routes reuse the canonical Recording track/WebVTT implementation. Track inventory and successful WebVTT responses are bounded; a Device-owned subtitle mutation must not return native paths, private cookies or Browser-session capabilities. Audio changes reuse MediaSession replacement instead of a new unrestricted control route.
- **Unverified at this source-only checkpoint:** exact installed daemon versus this `main` SHA, registered routes on the running server, real Hisense audio/SRT/WebVTT selection and the complete physical playback/cleanup acceptance. These remain explicit runtime/deployment gates, not completed facts.

Evidence: [Device Recording playback](development/public-v1-device-recording-playback.md), [Device tracks/index/SRT integration](development/vidaa-device-tracks-index-srt-integration-20261011.md), [Index-status draft history](development/vidaa-index-status-public-v1-20261011.md).
## Phase 66 and post-phase completion state

**Phase 66 is completed.** Slices 66.1 through 66.8 remain the numbered Phase-66 completion boundary. The later work below is deliberately non-numbered hardening/correctness work and does not reopen Phase 66 or start Phase 67.

Merged post-Phase-66 work includes, among other bounded follow-ups:

- PR #265 Home performance hardening;
- PR #266 Recording Discovery performance hardening;
- PR #267 Series metadata/artwork completion;
- PR #268/#269 native Recording marks/cutting and documentation closeout;
- PR #270 retained Series UI/revalidation hardening;
- PR #272 browser diagnostics route recovery;
- PR #273 removal of legacy EPG overview contamination from Media Home;
- PR #274 bounded persistent Home artwork previews;
- PR #277 bounded random-Genre metadata work;
- PR #278 EPG cache worker timeout recovery;
- PR #279 in-place Recent Movies expansion;
- PR #280 representative Series metadata scheduling;
- PR #281 Home artwork preview diagnostics;
- PR #282 artwork-preview cache-hit fast path;
- PR #283 deferred Series metadata hierarchy completion;
- the subsequent Home-rebuild commit series ending at `0cce4d1c...`, merged as `ea5967b9...`;
- PR #285 restoration of the EPG artwork/metadata cache routes dropped during the Home H2/H2.1 rewrite;
- PR #287 cinematic Recordings 2 detail/hero presentation and canonical-owner playback prewarm;
- PR #288 canonical portrait poster selection for related-Genre Recording cards;
- PR #289 Live-TV `ended` lifecycle stabilization so a transient media-element EOF does not destroy the canonical Live MediaSession;
- PR #291 documentation of the accepted Live-TV lifecycle stabilization;
- PR #294 restoration of Recording actions plus paused Hero prewarm;
- PR #295 Recording detail metadata-cache fallback and technical playback/marks layout repair;
- PR #296 clickable Hero cast with canonical local person search and neutral related-card styling;
- PR #297 bottom Metadata navigation plus end-to-end preservation of `autoPlay:false` through fallback/restart-seek decorators;
- PR #298 explicit server-side TMDB Trailer lookup with privacy-enhanced inline YouTube playback below related Recording cards.

Open or draft branches/PRs are not accepted current-state truth until they are merged and separately documented where required.

## Current accepted Home boundary

The first-party Home state now includes the completed original Phase-66 experience plus the merged rebuild/hardening work:

- canonical Home shell and navigation ownership;
- `Was läuft jetzt` / `Was läuft danach` with compact EPG critical-path loading and bounded artwork enrichment;
- newly recorded content and Recording discovery;
- Movies / recent movies and Genre browsing;
- Series projection with Series -> seasons -> episodes;
- progressive/scoped metadata completion without restoring the old global per-Recording metadata fan-out;
- TVScraper/canonical portrait artwork priority for Home cards;
- manual Series hierarchy overrides, pilot/miniseries and TV-film/special handling;
- persistent Series artwork overrides and reset-to-automatic hierarchy behavior;
- retained Series/folder/detail identity and navigation ownership;
- canonical native Recording artwork embedded into folder projections so folder cards do not need per-Recording metadata HTTP reads;
- real-system acceptance of the folder-poster regression using `The Exorcist`, where the canonical red TVScraper portrait again wins over a weak Recording still.

The earlier Phase-66-closeout note that visible Series metadata/artwork could remain unresolved is historical evidence of the state at that closeout. It is no longer an open current-state item: subsequent merged work established the root causes and completed the bounded Series metadata/artwork/hierarchy corrections.

## Current accepted Recording detail boundary

The merged Recordings 2 detail surface now includes the accepted post-Phase-66 presentation work documented in [Post-Phase-66 Recording Detail Closeout](development/post-phase66-recording-detail-closeout.md):

- cinematic/full-page Hero detail composition;
- compact Recording facts and retained canonical title/subtitle/description metadata;
- prominent playback entry into the existing canonical Recording playback owner;
- restored Recording actions through the existing action workflow;
- Recording-cache metadata/artwork fallback for the selected detail without restoring Home-wide metadata fan-out;
- clickable cast/person presentation backed by the existing local person-search owner;
- related Recording rails that reuse canonical Recording navigation;
- metadata, marks and playback submodes retained inside the same Recordings 2 detail owner;
- Metadata-mode bottom navigation for `Aufnahme | Scraper | Schauspieler | Bilder`, including usable mobile 2 x 2 layout;
- canonical-owner playback prewarm that buffers/prepares only and preserves `autoPlay:false` through compatibility decorators;
- related-Genre cards using canonical `kind=poster` selection so locked manual posters remain authoritative and native portrait metadata wins before weaker preferred artwork fallback;
- conditional explicit Trailer lookup through the server-side TMDB provider and canonical Web Client API owner;
- validated `youtube-nocookie.com` embedding with autoplay disabled and an inline Trailer section below related Recording cards;
- real yaVDR/browser/mobile acceptance for the Hero, playback/prewarm, metadata navigation, cast search and inline Trailer behavior.

This presentation work does not create a new Recording identity, metadata authority or playback lifecycle. TMDB credentials remain server-side, YouTube Trailer playback is not a Recording MediaSession, and normal detail rendering does not perform an external Trailer lookup. Native Recording marks/cutting remains separately governed by the accepted native Recording editing architecture and closeout.

## Live-TV lifecycle stabilization

The accepted Phase-65 persistent playback architecture remains authoritative. A later real-system regression was traced to the browser shell treating `HTMLMediaElement.ended` as a terminal Live-TV stop even though continuous Live/MSE playback is intentionally open-ended.

PR #289 removed that false stop boundary. The accepted behavior is now:

- a transient Live `ended` signal does not destroy the active canonical MediaSession;
- explicit Live stop, backend change, browser-session loss and replacement handoff remain real lifecycle boundaries;
- fatal player `error` remains terminal;
- the real yaVDR/browser regression test passed and Live-TV continued beyond the prior failure window;
- the durable evidence is recorded in [Phase 65.D.1 Persistent Browser Playback Shell Closeout](development/phase-65d1-persistent-browser-playback-shell-closeout.md).

This is post-closeout stabilization of completed Phase-65 playback behavior and does not reopen Phase 65 or Phase 66.

## Performance boundary retained

Home performance work must preserve the accepted reduction of Recording metadata fan-out. In particular:

- do not restore hundreds of browser `/recordings/metadata` reads;
- keep Series completion scoped/progressive rather than global;
- keep folder cards on the embedded folder read model when native metadata is already persisted;
- preserve generation/backend/Home-active fencing and authoritative owner boundaries;
- use preview/cache variants without changing original artwork ownership.

Recording-detail enrichment and playback prewarm must remain scoped to the selected/opened Recording and must not become a global Home metadata or MediaSession fan-out.

## Completed platform foundations retained

- **Phase 62 - Identity, RBAC and Accountability Foundation** remains authoritative for actor identity, backend-scoped authorization, browser session/CSRF and accountability.
- **Phase 63 - Backend Agent and Secure Multi-Site Runtime** remains authoritative for Agent identity, generation/lease fencing, provider ownership and durable command/result handling.
- **Phase 64 - Timer Intent and Multi-Backend Orchestration** remains authoritative for `TimerIntent -> TimerAssignment -> NativeTimerBinding`, managed fulfillment and controlled reassignment/failover.
- **Phase 65 - Streaming Gateway and Media Sessions** remains authoritative for Recording/Live MediaSession, least-transformation delivery and normalized playback ownership.
- **Phase 66 - Media Home and Browse Experience** remains the completed numbered Home/browse phase; later Home/Recording/Live work is non-numbered hardening.

## Completed Phase-67 boundary

**Phase 67 is completed.** Teletext merged through PR #293 and the HbbTV discovery/session/presentation-media vertical merged through PR #300 / `5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30`.

Accepted Broadcast Companion capability includes:

- normalized Teletext service/page/subpage contracts and first-party 25 x 40 rendering;
- private SuiteBridge/provider transport through the existing Phase-63 Agent path;
- backend-generation, channel/application-context and session fencing;
- normalized HbbTV application discovery with backend/service/application provenance;
- authorized Suite-owned HbbTV application sessions with normalized bounded input;
- session-owned presentation/media integration without exposing raw browser/plugin commands;
- preservation of Phase-65 MediaSession ownership whenever Suite-owned media is involved;
- real yaVDR/browser acceptance for Teletext Journey 8 and HbbTV Journey 9 on the supported deployment profile;
- full PR #300 CI, including architecture, frontend, packaging/install-staging and fast regression.

Durable evidence is in [Phase 67 Closeout](development/phase-67-closeout.md) and the earlier [Phase 67 Teletext Closeout](development/phase-67-teletext-closeout.md).

Phase 68 Legacy OSD Compatibility Bridge is **completed for the accepted 68.A-G scope**. The final 68.G real yaVDR acceptance proved fenced allowlisted native OSD input through the authenticated Control Plane -> Agent -> SuiteBridge -> VDR path, including native DOWN/UP effect, idempotent replay and stale-authority rejection. Durable evidence is in [Phase 68 Closeout](development/phase-68-closeout.md) and [Phase 68 Kickoff](development/phase-68-legacy-osd-kickoff.md). Phase 69 public API/client compatibility hardening is completed for the accepted 69.A-F scope. The post-Phase-69 Multiuser workstream is active: MU.0-MU.7 are complete; MU.6D merged as PR #411 and MU.7 grant administration merged as PR #413 after REAL YAVDR acceptance. MU.8 implementation is complete through MU.8C; MU.8A retains its separately documented real-runtime acceptance debt. MU.9 is completed through MU.9F / PR #430: MU.9A merged as PR #418 after exact merged-main yaVDR acceptance, with PR and post-merge hosted CI green; MU.9B Account Lifecycle Mutation UI merged as PR #420 after real yaVDR acceptance; PR and post-merge hosted CI are green. MU.9C Session Revoke UI merged as PR #426 after focused yaVDR acceptance and green hosted PR CI. MU.9D Human-password Credential Revoke UI merged as PR #428 after focused yaVDR acceptance; PR and post-merge hosted CI are green. MU.9E Backend Grant Mutation UI merged as PR #429 after focused real-yaVDR acceptance. GitHub-hosted runner degradation interrupted queued jobs without executing them; post-merge main CI run #9763 ultimately completed successfully with all six jobs green. MU.9F Account CREATE UI passed focused real-yaVDR acceptance, merged as PR #430, and both PR CI #9764 and post-merge main CI #9765 completed with all six jobs green. MU.9A-F are complete; no MU.9G is selected. MU.8A merged as PR #415. MU.8B passed focused yaVDR acceptance, merged as PR #416, and both PR and post-merge hosted CI are green; live daemon installation/restart was not required for that bounded slice. Phase 70 remains not started and still requires its dedicated accepted runtime ADR.

## Historical evidence rule

Exact old candidate SHAs, CI runs and once-open follow-up notes remain valid in their closeouts as historical evidence. They are not current execution authority. Use this file for volatile status and query GitHub for the live head before acting.


MU.10 live authority/gap audit and bounded implementation candidate:
[MU.10 Device/App Pairing Architecture and Gap Audit](development/post-phase69-mu10-device-app-pairing-audit.md) and
[MU.10A Device Pairing Bootstrap](development/post-phase69-mu10a-device-pairing-bootstrap.md).
