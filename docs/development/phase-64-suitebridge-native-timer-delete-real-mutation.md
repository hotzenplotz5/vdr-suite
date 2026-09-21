# Phase 64 — SuiteBridge Native Timer Delete Real-Mutation Gate

## Status

This is the first Phase-64 slice that contains and wires a real native VDR
Timer-delete callback.

It is stacked on the native Timer fingerprint-CAS slice / Draft PR #192. The
real-mutation gate was explicitly released before this work started. That
release authorizes implementation of the destructive VDR boundary; it does not
by itself authorize Ready, merge, deployment, or a claim of real-machine
acceptance.

## Mutation boundary

The production SuiteBridge plugin owns one dedicated
`SuiteBridgeNativeTimerDeleteVdrMutationCallback` and injects it into the
existing typed `SuiteBridgeNativeTimerDeleteService`.

The callback is deliberately narrower than a generic VDR mutation facility. It
accepts only the already validated `vdr.timer.delete` request envelope produced
by the existing private NTDEL contract.

No generic SVDRP command, shell command, RESTfulAPI mutation endpoint, browser
API or arbitrary VDR write payload is introduced.

## Exact native target

`backendNativeTimerId` must be a canonical positive decimal VDR Timer ID. The
callback takes the VDR Timer list write lock and resolves that ID with a local
lookup only:

```text
cTimers::GetTimersWrite(...)
  -> Timers::GetById(id, nullptr)
```

The `nullptr` remote identity is intentional: this mutation slice does not
silently retarget a local request to a remote Timer.

If the write lock cannot be acquired or the exact local Timer is no longer
present, the callback returns `RejectedWithoutEffect` without mutation.

## Lock-time fingerprint CAS

The Control Plane continues to own the exact canonical
`NativeTimerBinding.observedFingerprint`. PR #192 converts the already frozen
canonical value to a fixed-size `sha256:<64 lowercase hex>` token at the
Control-Plane -> Agent boundary.

This slice reconstructs the live canonical Timer observed state **while holding
the VDR Timer write lock**, using the same field order and normalization as the
existing Phase-64 binding model:

1. channel ID;
2. event ID or empty;
3. title;
4. directory;
5. day (`YYYY-MM-DD` or empty);
6. seven-character weekday mask;
7. zero-padded HHMM start;
8. zero-padded HHMM stop;
9. flags;
10. priority;
11. lifetime;
12. enabled;
13. VPS;
14. recording;
15. pending.

The callback hashes that live canonical value and compares it to the immutable
`expectedNativeTimerFingerprint` from the typed request under the same write
lock. A mismatch returns `RejectedWithoutEffect` and the Timer is not deleted.

This is the final last-hop CAS check immediately before the destructive call.
It is additional to, not a replacement for, the raw canonical Control-Plane CAS
performed during preparation/dispatch claim.

## Recording safety

A Timer that is currently recording is rejected without effect.

This callback intentionally does **not** call `Skip()`, does not call
`cRecordControls::Process()`, and does not stop an active recording as an
implicit side effect of Timer deletion. Stopping a recording would require its
own explicit product and safety contract.

## Exactly one native delete call

After all fences pass, the callback performs one bounded VDR mutation:

```text
SetExplicitModify()
  -> Del(timer)
  -> SetModified()
  -> commit/release state key
```

There is exactly one `Timers::Del(timer)` call in the callback and no retry
loop. The pre-existing SuiteBridge replay ledger reserves the exact operation,
command, request fingerprint and complete typed request before callback entry,
so an exact replay in the same plugin-instance epoch returns stored evidence
without invoking the callback again.

A changed request, command, operation or expected Timer fingerprint remains a
replay conflict and receives no mutation.

## Outcome semantics

A local VDR deletion is never final Control-Plane success by itself.

The callback returns:

- `RejectedWithoutEffect` for invalid local ID, unavailable write lock, missing
  Timer, live fingerprint mismatch, or currently recording Timer;
- `AppliedUnverified` only after the VDR list mutation has completed locally;
- `OutcomeUnknown` when an exception makes the callback outcome ambiguous.

The private SuiteBridge protocol maps a successful callback to
`accepted_unverified`; the Agent transport now preserves that category. An
ambiguous callback maps to `outcome_unknown`. Final Timer-delete success still
requires the existing authoritative native-Timer absence readback and shared
mutation-operation completion path.

No blind retry is introduced for an ambiguous post-dispatch state.

## What remains closed

This gate does not silently open the shipped Agent command path.

At this slice boundary:

- `BackendAgentClient` still does not construct or inject
  `SuiteBridgeNativeTimerDeleteTransport`;
- packaged `COMMAND_TYPES` still does not advertise `vdr.timer.delete`;
- `availableCommands()` still suppresses Timer delete;
- NTDEL remains absent from public SuiteBridge SVDRP help;
- no browser/public mutation API is added;
- no broad `mutations=enabled` capability namespace is introduced.

The SuiteBridge plugin truthfully reports its dedicated native Timer-delete
callback as enabled, but normal installed Agent configuration still cannot drive
that destructive path.

## Acceptance gate

Because real native VDR Timer deletion is now present in the installed plugin
binary, exact-head CI is necessary but no longer sufficient.

Before this slice can be considered Ready or merged, a bounded real yaVDR
acceptance run is required. That run must use disposable test Timers and prove,
at minimum:

- plugin build/install/start with the exact candidate head;
- private capability reports the enabled callback and correct plugin-instance
  epoch;
- fingerprint mismatch leaves the Timer untouched;
- a recording Timer is left untouched;
- an exact matching disposable Timer is deleted once;
- exact request replay does not delete another Timer or invoke a second logical
  mutation;
- simulated/lost response handling does not authorize a blind retry;
- authoritative readback observes absence only after the real dispatch boundary
  and completes the existing shared operation correctly;
- VDR remains stable through plugin/VDR restart and the test leaves no unrelated
  Timer changes.

## Real yaVDR acceptance evidence — 2026-08-14

A bounded destructive acceptance run was executed on the real `yavdr` host on
2026-08-14 against exact candidate head
`0f40b10baba3d9c39ab69e1e09ad1e3a3eacd1f6`.

Environment and installed candidate:

- VDR: `2.7.9`;
- SuiteBridge plugin version: `0.13.3`;
- installed plugin path: `/usr/lib/vdr/plugins/libvdr-suitebridge.so.11`;
- candidate/plugin SHA-256:
  `742b895547b93cfba9fccce963786612f8b8193df917beef28dffa5bff8d872b`;
- pre-restart plugin-instance epoch:
  `pie_2271622e7bcea75154c7d002957afccf`;
- post-restart plugin-instance epoch:
  `pie_ec3796211cd20906668d09237a877faa`.

The productive Timer baseline was recorded before destructive acceptance and
contained exactly these unrelated Timers:

```text
1  1:16:2026-08-15:0100:0250:50:99:Ein unmoralisches Angebot:eventId=41181
2  1:2:2026-08-16:0100:0240:50:99:Largo Winch - Tödliches Erbe:eventId=37503
```

Those productive Timers were never selected as mutation targets and were still
present with the same settings at final cleanup.

### Fingerprint mismatch — PASS

Disposable Timer `3`, `VDRSUITE_PHASE64_MISMATCH`, was targeted with a
syntactically valid deliberately wrong expected fingerprint.

SuiteBridge returned:

```text
556 ... rejected_without_effect callback_rejected \
  ntdel:vdr:fingerprint-mismatch:cmd_phase64_mismatch_001
```

Immediate authoritative `LSTT` readback showed Timer 3 still present and
productive Timers 1 and 2 unchanged.

### Recording Timer protection — PASS

Disposable Timer `4`, `VDRSUITE_PHASE64_RECORDING`, entered a real recording
state. VDR logged Timer start and recording-file creation and `LSTT 4` showed
recording flags `9`.

After deriving the matching live fingerprint including `recording=1` and
`pending=1`, SuiteBridge returned:

```text
556 ... rejected_without_effect callback_rejected \
  ntdel:vdr:recording:cmd_phase64_recording_002
```

`LSTT 4` still showed the Timer recording and the active TS file grew from
`307734192` to `315758784` bytes after the rejection. The mutation therefore
did not delete the recording Timer and did not stop the recording implicitly.

### Matching native delete — PASS

Disposable Timer `5`, `VDRSUITE_PHASE64_DELETE_ONCE`, was created outside any
current EPG horizon and guarded immediately before dispatch. Its matching live
fingerprint was:

```text
sha256:f714bc2f081f28abbb1bda01efe934ab414c3e65edeec1ae190d0734b36cb842
```

Exactly one private NTDEL request returned:

```text
557 ... accepted_unverified callback_applied \
  ntdel:vdr:deleted-unverified:cmd_phase64_delete_once_001
```

No final-success claim was made from that local callback result alone.

Authoritative native VDR readback then returned:

```text
501 Timer "5" not defined
```

and the full Timer list still contained only productive Timers 1 and 2 plus the
separate mismatch-test Timer 3.

### Exact same-instance replay — PASS

While the original plugin-instance epoch was still active and Timer 5 was
already authoritatively absent, the exact same NTDEL request was replayed.

The service returned the byte-equivalent terminal mutation result again:

```text
557 ... accepted_unverified callback_applied \
  ntdel:vdr:deleted-unverified:cmd_phase64_delete_once_001
```

Timer 5 remained absent and no unrelated Timer changed. If the VDR mutation
callback had been entered a second time, the missing Timer would instead have
produced `not-found`; the stored terminal result therefore demonstrates the
same-instance replay ledger prevented a second logical mutation callback.

### Lost/ambiguous response without blind retry — PASS

Disposable Timer `6`, `VDRSUITE_PHASE64_LOST_RESPONSE`, was guarded with the
matching fingerprint:

```text
sha256:f4cf08435e3f7dc9acdab98c5ad9dba80d57f428697ee561aeff464704114efc
```

One raw SVDRP TCP connection sent exactly one NTDEL request. The client read the
SVDRP `220` greeting, sent the complete request, shut down its write side and
deliberately did **not** read the NTDEL result. No second NTDEL request was
issued.

Reconciliation only was then performed through authoritative VDR readback:

```text
501 Timer "6" not defined
```

The full Timer list still contained productive Timers 1 and 2 plus Timer 3.
This proves that an unknown/lost post-dispatch response was handled by
reconciliation rather than by blind destructive retry in the acceptance flow.

### Restart stability — PASS

VDR was restarted after the destructive and lost-response cases. The service
returned to `active` with a new MainPID, the installed plugin SHA-256 remained
exactly the candidate hash, SuiteBridge initialized and started as version
`0.13.3`, and private NTDEL capability again reported `enabled` under a new
plugin-instance epoch.

After restart, the Timer list still contained productive Timers 1 and 2 plus
only the remaining disposable mismatch Timer 3. Deleted Timers 5 and 6 did not
reappear.

### Final cleanup and unrelated-Timer invariant — PASS

The remaining disposable Timer 3 was re-read immediately before cleanup and
then removed explicitly with `DELT 3`. Final authoritative `LSTT` output
contained exactly the original productive Timers 1 and 2 with their original
settings.

No productive Timer was deleted, modified, retargeted or used as an NTDEL test
target during the run.

### Acceptance boundary

This run proves the real native VDR mutation boundary, lock-time fingerprint
CAS, recording fence, exactly-once same-instance replay behavior, no-blind-retry
lost-response handling, authoritative native absence readback, restart
stability and unrelated-Timer preservation on real yaVDR hardware.

The normal shipped Control-Plane -> Agent Timer-delete command path remained
closed throughout, exactly as required by this slice. This evidence therefore
does **not** claim that packaged configuration now advertises or drives
`vdr.timer.delete`, nor does it weaken the existing requirement that shared
mutation-operation completion depends on authoritative readback when that
higher-level path is later enabled.

The destructive real-machine acceptance requirement for this native
SuiteBridge/VDR mutation gate is satisfied. Keep the stacked PR Draft until a
separate explicit Ready/merge decision is made.
