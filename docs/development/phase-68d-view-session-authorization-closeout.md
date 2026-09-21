# Phase 68.D - Authorized Legacy OSD View Sessions Closeout

## Status

Phase 68.D is accepted.

```text
PR: #310
Accepted candidate: 8807d587536daa6c27ae72a3f88d40b0c7de3480
Hosted CI: 35594914471 SUCCESS
Real yaVDR acceptance: RESULT=PHASE68D_REAL_ACCEPTANCE_PASS
```

Phase 68 remains active. The next coherent slice is **68.E - bounded viewer
bindings and multi-viewer delivery**.

## Scope completed

68.D implements SB.14 stage 7 on top of the accepted 68.C Agent transport:

```text
SuiteBridge semantic OsdFrame
  -> Agent authenticated OSD transport
  -> Control Plane transient receiver
  -> explicit backend-scoped osd.view
  -> bounded view-only LegacyOsdSession
```

The implementation reuses the single existing `BackendAgentLifecycleService`
and its transient OSD receiver. No parallel OSD cache, durable frame repository
or second Agent lifecycle owner was introduced.

The accepted view-session boundary provides:

- explicit `osd.view` authorization at HTTP admission;
- authorization and current authority re-check on session refresh;
- backend, actor and client binding;
- backend-generation fencing;
- bounded transient session count and lifetime;
- fail-closed expiry, revocation and generation-change handling;
- browser CSRF protection for session creation;
- `Cache-Control: no-store`;
- session/fencing metadata only, with no OSD frame/menu text exposed by the
  admission/status API.

`osd.view` remains an explicit backend-scoped grant. It is deliberately **not**
implied by `role.admin`.

## Accepted safety boundary

68.D remains strictly view-only.

It does not add:

- `OsdViewerBinding` fan-out;
- `OsdControllerLease`;
- `osd.control`;
- native key input;
- raw VDR key codes or `cRemote`;
- generic SVDRP or plugin-service tunnels;
- shell/process execution;
- direct RESTfulAPI/osd2web exposure;
- renderer/browser overlay;
- output-plugin rendering;
- SkinDesigner/native pixel capture;
- Teletext/HbbTV refactoring;
- Phase-65 MediaSession changes;
- Phase 69 work.

The existing 68.C rule also remains intact: OSD frame payloads are transient and
do not enter normal durable observation storage, logs or accountability events.

## Hosted CI evidence

Run `35594914471` completed successfully on the accepted candidate.

```text
architecture-check=SUCCESS
frontend-regression-test=SUCCESS
docs-check=SUCCESS
make-test-audit=SUCCESS
packaging-regression-test=SUCCESS
fast-regression-test=SUCCESS
daemon build=PASS
```

The fast regression suite includes the 68.D session/security tests and the 68.C
authenticated OSD transport regression.

CI hardening caught and corrected two relevant integration issues before
acceptance:

1. `role.admin` must not synthesize `osd.view`; the OSD view permission stays
   explicit and backend-scoped.
2. The existing HTTP-server regression target must link the Legacy OSD API
   runtime once `ApiRouter` references it.

Both corrected contracts are covered by regression/static checks.

## Real yaVDR evidence

The isolated worktree acceptance on the real yaVDR/VDR 2.7.9 system completed:

```text
DAEMON_BUILD=PASS
RESULT=PHASE68C_REAL_SOURCE_AUTHENTICATED_RECEIVER_PASS
REAL_OSD_SOURCE_AUTHENTICATED_RECEIVER=PASS
NO_CONTROL_INPUT_BOUNDARY=PASS
WORKTREE_CLEAN_AFTER=PASS
HOSTED_CI=PASS
RESULT=PHASE68D_REAL_ACCEPTANCE_PASS
```

The acceptance did not install or replace production binaries and did not
restart VDR or the production daemon.

## Continuation boundary

68.E may build only the viewer plane on top of this accepted session contract:

- bounded `OsdViewerBinding` ownership;
- multiple simultaneous read-only viewers within explicit limits;
- surface/session/epoch association;
- bounded delivery/backpressure and resynchronization.

68.E must not silently grant controller authority.

Controller leasing is now the following coherent slice, **68.F**, and must prove
exclusive lease ownership, `osd.control`, expiry/revocation and read-only
backend denial before any native input work. Allowlisted input follows as
**68.G**.
