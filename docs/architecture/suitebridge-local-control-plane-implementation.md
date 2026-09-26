# SuiteBridge local control-plane implementation

## Status

ADR-0064 Foundation, both initial **Critical Control** slices, Legacy OSD
**Interactive Control**, HbbTV + Teletext **External Plugin Interactive**, and
the first **Background Provider** slice (RMETA/META/ARTW) are implemented on
the internal VDR-Suite/SuiteBridge boundary.

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
| NCAP / native-probe capability | Critical Control | migrated |
| NPROBE EXEC | Critical Control | migrated |
| NPROBE READ | Critical Control | migrated |
| CAPS schema 1 | Interactive Control/Read | migrated for OSD flow |
| OSDSNAP | Interactive Control/Read | migrated |
| OSDINPUT | Interactive Control/Read | migrated |
| HBBAPPS | External Plugin Interactive | migrated |
| HBBRUN LAUNCH/STATUS/INPUT/CLOSE | External Plugin Interactive | migrated |
| HBBPRES META/CHUNK | External Plugin Interactive | migrated |
| HBBMEDIA | External Plugin Interactive | migrated |
| TTXC | External Plugin Interactive | migrated |
| TTXP | External Plugin Interactive | migrated |
| META | Background Provider | migrated |
| ARTW | Background Provider | migrated |
| RMETA | Background Provider | migrated |

## Admission, deadline and lane

The endpoint now owns four independent finite execution lanes:

- one **Critical Control** queue/worker for Live and native-probe fencing;
- one **Interactive Control/Read** queue/worker for Legacy OSD capability,
  semantic snapshots and fenced native input;
- one **External Plugin Interactive** queue/worker for serialized HbbTV and
  Teletext provider-service work;
- one **Background Provider** queue/worker for serialized TVScraper-backed
  RMETA/META/ARTW work.

A blocked HbbTV or TVScraper provider request therefore cannot consume either
the Critical or Legacy-OSD worker. Within each class execution remains serialized. This adds
transport/lane isolation without claiming generic VDR or provider parallel
safety.

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

The daemon Live runtime and the explicitly activated Backend-Agent native-probe
runtime both own a dedicated local transport plus their existing
\`SuiteBridgeSvdrpTransport\` compatibility transport.

\`SuiteBridgePrioritizedLiveTransport\` selects the local endpoint first. SVDRP
is used only when the local transport returns \`Unavailable\` before request
dispatch. Timeout, overload, protocol failure or other post-selection
uncertainty is returned to the existing Live liveness logic and is never
replayed over SVDRP.

That preserves the PR #350 rule: missing fresh evidence is indeterminate, not a
terminal Live-session result.

The daemon HbbTV resolvers use the same selection rule through
\`SuiteBridgePrioritizedHbbtvTransport\`: the Unix endpoint is preferred and
SVDRP is used only for a pre-dispatch \`Unavailable\` result. Timeout or other
uncertainty after local selection is never replayed, including HbbTV LAUNCH,
INPUT or CLOSE.

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
control-plane target is attached to both \`test-fast\` and hosted \`test-ci-fast\`.

## HbbTV provider/thread audit

HbbTV remains behind the existing typed `SuiteBridgeHbbtvCommandService` and
`SuiteBridgeHbbtvAdapter`; the local endpoint does not duplicate provider
semantics. The adapter synchronously invokes `vdr-plugin-web` through
`cPluginManager::CallFirstService()`, as the previous SVDRP handler already
did from VDR's SVDRP server thread.

The provider implementation was audited against its pinned discovery/runtime
contracts. Discovery and media state are mutex-protected bounded reads. Runtime
state is mutex-protected and UI launch/input/close work uses the provider's
existing VDR remote/main-context scheduling. HbbTV calls therefore remain
serialized on one SuiteBridge provider worker; no same-provider re-entrancy
assumption is introduced.

Presentation is deliberately isolated from Legacy OSD. On the first read of a
new frame, the provider can perform BGRA-to-RGBA conversion and QOI encoding
while holding its presentation mutex. That can be materially heavier than an
OSD input/snapshot. The dedicated External Plugin Interactive worker prevents
that work from blocking either Critical Live/native-probe control or Legacy OSD.

The lane regression deliberately blocks HbbTV presentation and requires both
Live capability and OSD snapshot to complete inside the bounded test budget.
The transport regression separately proves that a local HbbTV timeout is not
replayed through SVDRP.

## Teletext provider/thread audit

Teletext remains behind the existing typed `SuiteBridgeTeletextCommandService`
and `SuiteBridgeTeletextAdapter`. The provider ABI is pinned to
`vdr-plugin-osdteletext` commit
`70496310d1fa5200ff808d1552f0f5d252893870`.

The provider's `SnapshotStore::Read()` copies the selected page snapshot and
service-state metadata while holding its own mutex. The mutex is released before
`cRenderPage::RenderTeletextCode()` and normalization of the bounded 25x40
cell matrix. The service path performs no network or filesystem I/O and exports
no VDR-native pointer lifetime across the call.

Teletext therefore shares the serial External Plugin Interactive worker with
HbbTV. This keeps all migrated external-plugin Service calls conservative and
non-reentrant while still isolating them from Critical Live/native-probe work
and Legacy OSD. No separate Teletext worker is justified by the audited
provider contract.

The daemon uses `SuiteBridgePrioritizedTeletextTransport`; SVDRP is used only
for a pre-dispatch `Unavailable` result. A local timeout is returned to the
caller and is not replayed through the compatibility path.

## TVScraper background/provider audit

The production Home/metadata call graph was checked before migration.

- Home rails already start their major loads concurrently.
- Series/native Recording metadata reads are bounded in the browser to four
  concurrent requests.
- That browser concurrency does **not** prove that TVScraper's VDR plugin
  services are re-entrant.
- `META` and `ARTW` detach the selected EPG event while holding the schedule
  lock and release that VDR lock before provider/serialization work.
- `RMETA` is different: it must retain `LOCK_RECORDINGS_READ` while passing
  the real `cRecording*` synchronously to TVScraper.

Therefore RMETA/META/ARTW share one serial Background Provider worker. This
removes their head-of-line coupling with Critical, Legacy OSD and external
interactive provider work without introducing concurrent TVScraper Service()
calls. It intentionally does not claim that four browser metadata requests now
execute four TVScraper calls in parallel.

The regression deliberately blocks RMETA and requires Live capability and OSD
snapshot to continue within the bounded test budget. This directly covers the
production failure mode that motivated ADR-0064.

`ETYPES` is left for a separate slice because its paginated transport parser
and real-event/channel/schedule lock contract are materially different from the
Home-facing RMETA/META/ARTW path. `MCOMPARE` remains diagnostic-only SVDRP.

## Still staged on existing paths

Not migrated by this slice:

- Timer CREATE/DELETE/MODIFY;
- Recording marks/cut;
- ETYPES and diagnostics.

Native probe now keeps its existing durable starting/receipt/result/readback fencing while moving only its local transport boundary. The remaining operation families still need their operation-specific execution-lane proofs. No provider re-entrancy assumption is introduced here.
