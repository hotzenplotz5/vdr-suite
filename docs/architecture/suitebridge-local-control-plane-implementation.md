# SuiteBridge local control-plane implementation

## Status

ADR-0064 Foundation and the first productive **Critical Control** migration slice
are implemented on the internal VDR-Suite/SuiteBridge boundary.

This does not change Phase 69, the public API, actor authorization or mutation
semantics.

## Private transport and peer boundary

The SuiteBridge plugin owns:

\`\`\`text
/run/vdr/vdr-suite-control/control.sock
\`\`\`

The endpoint uses \`AF_UNIX\` + \`SOCK_SEQPACKET\`; there is no TCP listener.
The package provisions the parent as \`0700 vdr:vdr\`; the socket is \`0660\`.
The server validates \`SO_PEERCRED\` and admits the VDR effective uid or uid 0.
This covers the packaged VDR process/backend-agent and the root daemon without
creating a public authentication surface.

## Protocol v1

The binary header is fixed and bounded. Every request carries protocol
major/minor, one closed operation id, the server-owned service class, a non-zero
request/correlation id, an absolute \`CLOCK_MONOTONIC\` deadline and a bounded
payload length.

There is no generic command or plugin-call tunnel. Unknown operations,
class/priority spoofing, version mismatch, oversized frames and stale requests
are rejected before dispatch.

The initial registry is deliberately small:

| operation | class | state |
| --- | --- | --- |
| NLCAP / Live capability | Critical Control | migrated |
| NLIVE OPEN | Critical Control | migrated |
| NLIVE STATUS | Critical Control | migrated |
| NLIVE CLOSE | Critical Control | migrated |

## Admission, deadline and lane

The initial endpoint owns one finite Critical-Control queue and exactly one
Critical-Control worker. Transport isolation therefore does **not** introduce
new parallel native Live execution.

Admission never waits for queue capacity. Saturation returns typed
\`Overloaded/queue_full\`. Deadlines are checked at admission and again
immediately before execution. A request that expires while queued is returned
as \`DeadlineExpired\` and is never executed later.

Metrics cover admitted/executed counts by operation, rejected/overloaded,
deadline-expired-before-execution, protocol and peer rejection, queue high-water
mark, aggregate queue wait and execution duration. Per-request logs contain only
operation family, request id and a bounded reason code; payloads are not logged.

## Lifecycle and shutdown

The endpoint starts in SuiteBridge \`Start()\`. Startup fails closed if the local
control socket cannot be created. During \`Stop()\`, admission is stopped first,
queued requests are rejected as unavailable, the worker is joined, the socket
is removed, and only then are Live sources and status observation torn down.

## SVDRP compatibility without mutation-style replay

The daemon Live runtime owns both the dedicated local transport and the existing
\`SuiteBridgeSvdrpTransport\`.

\`SuiteBridgePrioritizedLiveTransport\` selects the local endpoint first. SVDRP
is used only when the local transport returns \`Unavailable\` before request
dispatch. Timeout, overload, protocol failure or other post-selection
uncertainty is returned to the existing Live liveness logic and is never
replayed over SVDRP.

That preserves the PR #350 rule: missing fresh evidence is indeterminate, not a
terminal Live-session result.

## Head-of-line regression coverage

\`test_suite_bridge_local_control_transport\` holds a simulated legacy/SVDRP lane
blocked while a Live STATUS request uses the dedicated Unix endpoint. STATUS
must complete inside its small budget and must not touch the compatibility
transport.

\`test_suitebridge_control_plane\` separately proves finite overload, expiry
while queued, non-execution after expiry, queue high-water accounting and
deterministic socket removal on shutdown.

Run:

\`\`\`text
make test-suitebridge-control-plane
\`\`\`

The architecture guard is also attached to \`test-architecture\`, and the full
control-plane target is attached to \`test-fast\`.

## Still staged on existing paths

Not migrated by this slice:

- native probe/readback (next Critical-Control candidate);
- Legacy OSD;
- Teletext and HbbTV;
- Timer CREATE/DELETE/MODIFY;
- Recording marks/cut;
- RMETA/META/ARTW/ETYPES and diagnostics.

Those operation families still need their operation-specific execution-lane
proofs. No provider re-entrancy assumption is introduced here.
