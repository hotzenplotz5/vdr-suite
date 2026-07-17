# Suite Bridge Local SVDRP Transport

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [Suite Bridge Backend Agent Handshake](suite-bridge-agent-handshake.md)
- [Backend Agent and Control Plane Boundary](../adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [Suite Bridge Handoff](../../vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md)

---

## Status

Implementation slice: `SB.10b`

State: isolated typed local SVDRP transport implemented; automated and live VDR
acceptance are still required before the slice is completed.

SB.10b changes no plugin source, command, capability, schema or version.

---

## Purpose

SB.10a defined a transport-neutral Agent boundary for exactly two read-only
logical operations:

```text
DiscoverSchema1
Snapshot
```

SB.10b implements the first concrete local transport behind that interface.

The transport connects directly to VDR's configured SVDRP endpoint, invokes one
fixed Suite Bridge command, returns one bounded typed result and closes the
connection.

It does not use `svdrpsend`, a shell, a subprocess or the existing mutation-
specific channel-move executor.

---

## Ownership

### Backend Agent owns

- local SVDRP endpoint configuration;
- connection establishment;
- deadline enforcement;
- SVDRP framing and reply parsing;
- transport error classification;
- bounded response handling;
- deterministic socket cleanup;
- invocation through the typed SB.10a interface.

### Plugin owns

- `CAPS 1` and `SNAP` command handling;
- local reply codes;
- local JSON payloads;
- truthful plugin capability and snapshot values.

### Control Plane owns

- no direct SVDRP connection;
- no raw plugin reply;
- no local VDR credential or endpoint;
- later backend health and capability presentation through the Agent boundary.

---

## Source Boundary

SB.10b implementation files:

```text
core/agent/include/SuiteBridgeSvdrpTransport.h
core/agent/src/SuiteBridgeSvdrpTransport.cpp
core/agent/tests/test_suite_bridge_svdrp_transport.cpp
core/agent/tests/test_suite_bridge_svdrp_transport_live.cpp
tools/check_suite_bridge_svdrp_transport_boundary.py
```

Build ownership remains in:

```text
mk/agent-sources.mk
mk/agent-tests.mk
mk/test-groups.mk
```

SB.10b does not modify:

- `RuntimeConfig`;
- `BackendRuntimeContext`;
- `DaemonRuntime`;
- RESTfulAPI adapters;
- `vdr-plugin-suite-bridge` runtime source;
- plugin capability or schema catalogues;
- public API routes;
- mutation executors.

Daemon integration belongs to a later SB.10 slice.

---

## Typed Command Boundary

The public C++ transport method accepts `SuiteBridgeLocalCommand`, not a string.

The only wire requests are:

```text
DiscoverSchema1 -> PLUG suitebridge CAPS 1\r\n
Snapshot        -> PLUG suitebridge SNAP\r\n
```

No caller can submit:

- arbitrary SVDRP command text;
- another plugin name;
- another Suite Bridge subcommand;
- caller-controlled arguments;
- shell syntax;
- a command sequence.

A future command requires a new typed enum value, explicit wire mapping, tests,
documentation and capability impact review.

---

## Connection Lifecycle

Every `execute()` call owns one independent TCP connection:

```text
validate local configuration
  -> resolve localhost or numeric address
  -> create non-blocking close-on-exec socket
  -> connect within bounded deadline
  -> read complete SVDRP greeting
  -> require greeting code 220
  -> send one fixed request
  -> read one complete reply
  -> close socket
  -> return typed result
```

The transport retains no open socket and no hidden SVDRP session between calls.

This keeps reconnect and polling policy outside the transport. SB.10c owns the
later observation lifecycle and retry policy.

---

## Endpoint Policy

The default endpoint is:

```text
127.0.0.1:6419
```

The isolated transport accepts:

- `localhost`, normalized to `127.0.0.1`;
- numeric IPv4 addresses;
- numeric IPv6 addresses.

It does not perform DNS lookup. This avoids unbounded or environment-dependent
name-resolution behavior in the local transport slice.

An empty host means the transport is not configured and returns
`SuiteBridgeTransportStatus::Unavailable` without opening a socket.

Invalid host, port or deadline values return `Failed`.

Connection refusal and other endpoint failures return `Failed`, not
`Unavailable`. The SB.10a handshake therefore does not misclassify a configured
but unreachable VDR as a legacy plugin.

---

## Deadline Model

The configuration contains three positive limits:

- connect timeout;
- per-I/O-phase timeout;
- total operation timeout.

Each phase deadline is the earlier of:

- the phase-specific deadline;
- the total operation deadline.

The socket is non-blocking. `poll()` bounds connect, send and receive waits.
Connect completion is verified through `SO_ERROR`.

Timeout results remain distinct from generic transport failure.

The transport owns no retry or reconnect loop.

---

## SVDRP Framing

The transport validates the server greeting before sending a command.

A reply line consists of:

```text
three decimal reply digits
one separator character
reply text
line ending
```

Allowed separator characters are:

- `-` for a continuation line;
- space for the final line.

All lines in one multiline reply must use the same reply code.

The parser accepts CRLF and LF input and normalizes payload line separation to
`\n`.

The final reply code is preserved separately from the payload. A syntactically
valid reply such as `504` is a successful transport exchange and is evaluated by
the handshake as a reply rejection rather than a socket failure.

The parser rejects:

- malformed reply codes;
- invalid separators;
- inconsistent multiline codes;
- incomplete continuation replies;
- connection close before final line;
- replies above the byte limit;
- replies above the line-count limit.

---

## Bounded Resources

Current fixed limits:

| Resource | Limit |
| --- | ---: |
| Greeting bytes | `1024` |
| Command reply bytes | `8192` |
| Reply lines | `64` |
| Wire commands per connection | `1` |
| Persistent sockets | `0` |
| Transport-owned threads | `0` |

The parser reads in fixed-size chunks and stops at the configured bound.

Socket ownership uses deterministic RAII cleanup. File descriptors are marked
close-on-exec even though SB.10b launches no process.

---

## Transport Result Mapping

| Situation | Transport status |
| --- | --- |
| Empty host / deliberately not configured | `Unavailable` |
| Valid greeting and complete command reply | `Success` |
| Connect, greeting, send or reply deadline exceeded | `Timeout` |
| Invalid configuration | `Failed` |
| Connection refused or endpoint failure | `Failed` |
| Invalid greeting | `Failed` |
| Malformed or incomplete reply | `Failed` |
| Bounded size or line-count violation | `Failed` |

For `Success`, the result carries the parsed local reply code and normalized
payload.

Diagnostics are fixed bounded phrases and do not include raw payloads,
credentials or caller-controlled command text.

---

## Deliberate Non-Reuse

### `SvdrpChannelMoveExecutor`

The existing channel-move executor constructs a shell command and uses
`popen()` with `svdrpsend`. It is mutation-specific and does not provide the
required typed, deadline-bounded or reply-structured Agent contract.

It must not become the generic Suite Bridge transport.

### `BasicHttpClient`

The HTTP client owns HTTP serialization and parsing and does not provide the
SVDRP greeting, multiline-reply or transport status contract.

### `RestfulApiEventStreamClient`

The event-stream client owns a background thread and reconnect loop. SB.10b is a
single synchronous transaction and owns no polling lifecycle.

---

## Automated Tests

The deterministic loopback fixture covers:

- exact `CAPS 1` wire request;
- exact `SNAP` wire request;
- successful single-line reply;
- CRLF and LF normalization;
- multiline reply completion;
- non-`900` reply preserved as transport success;
- unexpected greeting code;
- malformed greeting;
- malformed reply separator;
- incomplete multiline reply;
- inconsistent multiline reply code;
- greeting timeout;
- command-reply timeout;
- oversized reply;
- unavailable configuration;
- invalid endpoint configuration;
- non-numeric host rejection;
- connection failure;
- deterministic peer closure.

The source boundary guard proves:

- no shell or subprocess use;
- no `svdrpsend` use;
- no thread or mutex in the transport;
- no database, filesystem, RESTfulAPI or daemon coupling;
- no reuse of `SvdrpChannelMoveExecutor`;
- exactly two fixed Suite Bridge requests;
- non-blocking connect, `poll()`, `SO_ERROR`, close-on-exec and no-signal send;
- explicit greeting, size and multiline contracts.

---

## Manual Live VDR Smoke Test

The manual target is:

```text
make test-suite-bridge-svdrp-transport-live
```

Optional environment variables:

```text
VDR_SUITE_SUITEBRIDGE_SVDRP_HOST
VDR_SUITE_SUITEBRIDGE_SVDRP_PORT
```

The live test performs the complete SB.10a handshake through the real SB.10b
transport. It requires:

- a controlled VDR instance;
- the accepted Suite Bridge plugin installed and loaded;
- SVDRP access permitted from the selected local address;
- pre-test capture of channel, Timer, Recording and setup state;
- successful `CAPS 1` and `SNAP` only;
- `mutationsEnabled=false`;
- unchanged unrelated VDR state;
- deterministic plugin and VDR rollback where the test installation changed
  live files;
- a clean repository worktree.

The live test prints bounded schema and activity facts. It does not print the raw
JSON payload.

---

## Non-Goals

SB.10b adds no:

- daemon wiring;
- persistent Agent worker;
- reconnect loop;
- capability freshness state;
- public API;
- plugin command;
- plugin version change;
- plugin capability change;
- mutation;
- Timer, Recording, EPG, media or OSD surface;
- raw SVDRP tunnel;
- remote Control Plane access to the SVDRP port.

---

## Next Slice

After automated and controlled live acceptance of SB.10b, SB.10c may add the
read-only Agent observation lifecycle:

- initial discovery and baseline;
- bounded polling;
- reconnect backoff;
- freshness timestamps;
- epoch replacement;
- overflow degradation;
- clean stop behavior.

SB.10c composes the transport. It does not widen the transport command surface.

---

## Back

- [Back to Suite Bridge Backend Agent Handshake](suite-bridge-agent-handshake.md)
- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
