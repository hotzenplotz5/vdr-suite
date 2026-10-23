# Native Timer API v1: yaVDR hardware acceptance (2026-10-21)

## Environment and scope

- Real yaVDR hardware, VDR 2.8.3, backend `default`, native SuiteBridge SVDRP executor and RESTfulAPI readback.
- Dedicated future EPG timer on channel `C-1-1051-10301`; pre-existing timers excluded.
- Source branch: `work/api-v1-timer-binding-freshness-20261020`; PR #503.
- Evidence source: the operator's local acceptance output, including durable mutation-operation IDs, native readback, and independent on-disk journals. This record does **not** claim an uninterrupted first-pass run.

## First disposable fixture

Journal: `/var/lib/vdr-suite/native-timer-hw-fresh-admin-20261021.json`.

| Action | Result |
| --- | --- |
| CREATE | `succeeded` |
| UPDATE | `failed_verified`: fresh authoritative readback proved no side effect |
| TOGGLE OFF | `succeeded` |
| TOGGLE ON | `succeeded` |
| DELETE | `succeeded`; timer absent at readback |

The original UPDATE command was not redispatched. A transient `event_id` mismatch was identified: the original RESTfulAPI mapper parsed a `null` event ID as the next JSON key `weekdays`; subsequent EPG association produced `13546`. Parser and event-ID-only metadata reconciliation were corrected; native timer configuration fences remain strict.

## Second, independent disposable fixture

Journal: `/var/lib/vdr-suite/native-timer-hw-fresh-admin-r2-20261021.json`.
Timer ID: `C-1-1051-10301:0:1792533600:1600:1610`.
Original title: `Tagesschau`; expected post-update title: `Tagesschau-APIv1-HW-R2`.

| Action | Durable operation ID | Result |
| --- | --- | --- |
| CREATE | `op_e3f803a8ac78ade8c88c28eea0d76aec` | `succeeded` |
| UPDATE | `op_5ddfcbf60a52692067cd43ab251816bf` | `succeeded`; modified title verified in native readback |
| Initial TOGGLE OFF | `op_750fd9831bb76e0f56c29e7a26525d72` | `failed_verified`; no effect, timer stayed active |
| Explicit TOGGLE OFF recovery | `op_ec651b4bed69375e8b5b977825afe303` | `succeeded` |
| TOGGLE ON | `op_0843fa1ee6340e9ac1aedf610809a1ec` | `succeeded` |
| DELETE | `op_0c35abdc88f8d4da3fab2a64db965459` | `succeeded`; timer absent at readback |

Final operator output:

```text
BACKEND_AGENT_BUILD=PASS
BACKEND_AGENT_RELOAD=PASS
UPDATE=PASS_VERIFIED_NATIVE_READBACK
PRIOR_TOGGLE_OFF=FAILED_VERIFIED_NO_EFFECT_NO_REDISPATCH
TOGGLE_OFF_RECOVERY ... STATE= succeeded
TOGGLE_OFF=PASS
TOGGLE_ON=PASS
DELETE=PASS
HARDWARE_ACCEPTANCE=PASS CREATE UPDATE TOGGLE_OFF_RECOVERY TOGGLE_ON DELETE; INITIAL_TOGGLE_OFF=FAILED_VERIFIED_NO_EFFECT
```

## Transport mitigation and limits

- Native UPDATE/TOGGLE execute in the **separate backend-agent process**, not the daemon.
- The SVDRP reply timeout for native MODIFY was raised from 1 to 4 seconds, with an overall operation timeout of 12 seconds; CREATE/DELETE timeout configuration is unaffected.
- The successful continuation produced no `NATIVE_MODIFY_TRANSPORT_FAILURE` lines in the queried backend-agent journal, and was performed **after installing and restarting the rebuilt backend-agent**.
- The original failure evidence was `suitebridge:ntmod:transport-outcome-unknown`. That evidence alone does not prove a timeout; the longer timeout is a mitigation, not demonstrated root-cause elimination.
- The successful continuation was a new, separately journaled operation **only after** the earlier `failed_verified` no-effect proof. There was no automatic or blind redispatch.
- A separate older historical operation (`op_28484cf94a4a38103040813e9a1b7b18`, generation 299) still emitted `outcome_application status=4` during diagnostics. It is **not** evidence of a failure of this disposable timer lifecycle and remains an independent cleanup/reconciliation follow-up.

## Conclusion

**Real-hardware feature acceptance: PASS with documented controlled recovery.** All five native timer actions (CREATE, UPDATE, TOGGLE OFF, TOGGLE ON, DELETE) completed successfully with readback during the second fixture lifecycle. **Not yet proven:** fully clean first-attempt lifecycle under repeated/loaded conditions, or complete elimination of intermittent SVDRP transport failures. Keep journals and command audit history; do not reuse terminal operation IDs for mutations.
