# Phase 68 Closeout — Legacy OSD Compatibility Bridge

**Phase 68 is completed.**

The accepted completion scope is 68.A through 68.G.

## Accepted repository identity

Merged predecessor verticals:

- 68.A — PR #304 -> `a37bb0c9cd0c262cde40369bb7c66686c3ba995e`
- 68.B — PR #308 -> `81b9a28759debf5c87fe72afa793bc3db0a486fa`
- 68.C — PR #309 -> `ed2f451d824e43c4264dee6caf8ae49dbeb71a88`
- 68.D — PR #310 -> `6065cfa1ce5fd7faeb9587ad8fcd6497cc7f48a5`
- 68.E — PR #311 -> `7a2747d1b1841e95843c2340700b4f0755549840`
- 68.F — PR #312 -> `e3f9215f5f80dd230e1e855e3ce09f2ac70231ef`

Final 68.G integration/closeout PR: **#313**.

Accepted real-runtime candidate:

```text
7ae51d090cbe06570b8a70e787137232df83f124
```

SuiteBridge identity at the accepted candidate:

```text
version=0.14.0
osd.view=available
osd.control=available
generic mutations=disabled
```

Hosted CI on the final PR #313 head is the merge gate. The real-runtime
acceptance below belongs to the exact candidate above; documentation-only
closeout changes do not require another yaVDR installation or restart.

## Delivered Phase-68 boundary

Phase 68 implements the ADR-0047 compatibility path as seven bounded verticals:

1. **68.A — semantic read-only observation**: bounded native OSD values, surface identity, independent OSD epoch and full-frame sequence.
2. **68.B — Agent-local continuity/resynchronization**: typed OSDSNAP, bounded full-frame buffering and explicit resync semantics.
3. **68.C — authenticated Agent transport**: current Agent identity/generation/lease and backend-scoped `osd.view` protect transient OSD observation delivery.
4. **68.D — authorized view sessions**: bounded `LegacyOsdSession` lifecycle with actor/client/backend/generation/expiry fences.
5. **68.E — viewer bindings**: bounded multi-viewer attachment with exact session/surface/epoch association and resynchronization.
6. **68.F — exclusive controller authority**: independent `osd.control`, exactly one active Suite controller lease per surface scope, lease epoch/revision and read-only-backend fencing.
7. **68.G — allowlisted native input**: normalized navigation/color vocabulary, short deadline, bounded rate/repeat, idempotent submission, dispatch-time fence revalidation, typed Agent/SuiteBridge transport and native VDR input termination.

The view session deliberately remains `view_only`. Control authority is not a
session capability; it is acquired independently through
`OsdControllerLease` after `osd.control` authorization and backend write
policy evaluation.

## Accepted 68.G input vocabulary

```text
up
down
left
right
ok
back
red
green
yellow
blue
```

The first accepted boundary does not expose raw key codes, arbitrary keyboard
text, playback/volume/numeric/menu shortcuts, generic SVDRP, plugin-service
calls, shell/process execution or a generic mutation tunnel.

## Real yaVDR acceptance

The final acceptance ran against the real yaVDR/VDR 2.7.9 runtime and the exact
candidate above.

Before creating the Legacy OSD session, the test opened the real native menu,
waited for a newly accepted Agent heartbeat and then verified that the
authenticated transient OSD observation carried the same current OSD epoch.

Accepted result:

```text
OSD_OBSERVATION_SYNCHRONIZED=PASS
NATIVE_OSD_READY=PASS
BROWSER_SESSION_AUTHENTICATED=PASS
BROWSER_ACTOR_RESOLVED=PASS
VIEW_SESSION_BOUNDARY=PASS
LEGACY_OSD_SESSION_EPOCH_CURRENT=PASS
LEGACY_OSD_SESSION=PASS
VIEWER_ATTACH=PASS
OSD_CONTROL_AUTHORITY=PASS
CONTROLLER_LEASE=PASS
DOWN_INPUT_ACCEPTED=PASS
DOWN_IDEMPOTENT_REPLAY=PASS
DOWN_NATIVE_DISPATCH=PASS
SELECTED_INDEX_AFTER_DOWN=1
DOWN_OSD_SELECTION_CHANGED=PASS
UP_INPUT_ACCEPTED=PASS
UP_NATIVE_DISPATCH=PASS
SELECTED_INDEX_AFTER_UP=0
UP_OSD_SELECTION_RESTORED=PASS
DISPATCH_FENCE_AUDIT=PASS
NATIVE_RESULT_AUDIT=PASS
AUDIT_NO_RAW_KEY_PAYLOAD=PASS
CONTROLLER_RELEASE=PASS
STALE_AUTHORITY_REJECTED=PASS
STALE_AUTHORITY_NO_ASSIGNMENT=PASS
STALE_AUTHORITY_NO_NATIVE_DISPATCH=PASS
VIEWER_DETACH=PASS
OSD_PERMISSION_STATE_RESTORED=PASS
BROWSER_SESSION_LOGOUT=PASS
TEST_OPENED_OSD_CLOSED=PASS
SQLITE_QUICK_CHECK=PASS
VDR_NOT_RESTARTED=PASS
DAEMON_NOT_RESTARTED=PASS
AGENT_NOT_RESTARTED=PASS
AGENT_ENABLEMENT_UNCHANGED=PASS
AGENT_FINAL_STATE_ONLINE=PASS
OSD_INPUT_CAPABILITY_FINAL=PASS
RESULT=PHASE68G_REAL_NATIVE_OSD_INPUT_PASS
```

The native semantic effect was observed, not inferred from command acceptance:
DOWN moved the selected native menu index from **0 to 1** and UP restored it
from **1 to 0**.

The persisted command/result path for both directions reached:

```text
command_type=vdr.legacy-osd.input
state=completed
receipt_category=accepted
receipt_reason=durably_recorded
result_category=succeeded
dispatch_state=effect_reported
verification_state=not_required
error_category=none
retry_classification=none
```

An immediate identical DOWN replay returned the same Agent command identity and
did not create a second assignment. After controller release, a new command
using stale authority was rejected with conflict before assignment and caused no
native dispatch.

## Safety and ownership retained

- VDR remains the native input authority; the typed path terminates at the native remote primitive only after local OSD revalidation.
- Control Plane owns authorization, browser CSRF, session/viewer/controller authority and public mutation admission.
- Backend Agent owns authenticated machine transport, generation/lease/capability freshness and bounded command delivery.
- SuiteBridge owns the private typed local OSD snapshot/input boundary and current local OSD validation.
- `backendGeneration`, `osdEpoch`, session revision, viewer binding, controller lease epoch/revision and command identity remain distinct fences.
- Physical/local VDR remote activity is not replaced by Suite controller authority.
- Frame contents and raw key payloads do not enter normal accountability events.
- No SkinDesigner/osd2web visual takeover, raw native pixel capture or public plugin endpoint is introduced.
- Teletext/HbbTV remain their completed structured Phase-67 domains.
- Phase-65 MediaSession/playback ownership is unchanged.
- Generic SuiteBridge `mutations` remains disabled even though the separately named `osd.control` capability is available.

## Phase-68 acceptance gate

| Gate | Result |
| --- | --- |
| Domain-first product surfaces remain primary | PASS |
| Independent `osd.view` / `osd.control` authorization | PASS |
| Read-only backend control denial | PASS |
| Full-frame continuity/resynchronization and independent OSD epoch | PASS |
| Bounded multi-viewer association | PASS |
| Exactly one active Suite controller lease per surface scope | PASS |
| Stale generation/surface/epoch/lease authority fails closed | PASS |
| Allowlisted input only; no raw/generic command tunnel | PASS |
| Idempotent input submission without duplicate assignment | PASS |
| Dispatch-time authority revalidation | PASS |
| Native VDR effect observed on real yaVDR | PASS |
| Stale authority produces no assignment/native dispatch | PASS |
| Audit contains semantic result without raw key payload | PASS |
| SQLite and runtime processes remain healthy | PASS |
| Golden Journey 10 | PASS |

## Retained non-goals

Phase 68 does not make Legacy OSD the primary VDR-Suite UI, add a browser/TV
renderer or output plugin, promise pixel-perfect support for every VDR skin,
replace structured Teletext/HbbTV domains, expose generic native commands, or
start Phase 69.

Optional later OSD fidelity improvements require a separately justified bounded
change and must preserve full-frame resynchronization and the accepted authority
fences.

## Next numbered phase

```text
Phase 68 - Legacy OSD Compatibility Bridge [COMPLETED]
  -> Phase 69 - Public API and Client Compatibility Hardening [NEXT; NOT STARTED]
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

Phase 69 requires its own explicit runtime start.

## Related documents

- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Phase 68 Kickoff](phase-68-legacy-osd-kickoff.md)
- [ADR-0047 Legacy OSD Compatibility Bridge](../adr/ADR-0047-legacy-osd-compatibility-bridge.md)
- [SuiteBridge Roadmap](../../vdr-plugin-suite-bridge/docs/ROADMAP.md)
