# Phase 63 Slice 4 — Fenced Native Operation Runtime

## Status

This runtime slice implements exactly one non-mutating native operation:

```text
commandType = vdr.native.probe
payloadVersion = 1
verificationPolicy = readback_required
sideEffectClass = none
mutations = disabled
```

It is the bounded runtime implementation of
[`phase-63-fenced-native-operation.md`](phase-63-fenced-native-operation.md).
It does not authorize or implement a VDR mutation, provider ownership, provider
selection or a generic native command tunnel.

## Boundary

The Control Plane remains authoritative for assignment, Agent and Backend
fencing, attempt and claim identity, command identity, deadline, replay,
reconciliation and final outcome interpretation. The Agent remains outbound
only.

The Agent advertises `vdr.native.probe` only when all local SuiteBridge facts
are present and compatible:

```text
nativeOperation = vdr.native.probe
nativeOperationSchema = 1
sideEffectClass = none
mutations = disabled
localProviderKind = suitebridge
pluginInstanceEpoch = current non-empty epoch
```

The packaged configuration keeps `COMMAND_TYPES=` empty. Runtime activation is
acceptance-only and explicit:

```text
vdr-suite-backend-agent \
  --config /etc/vdr-suite/backend-agent.conf \
  --native-probe \
  --suitebridge-host 127.0.0.1 \
  --suitebridge-port 16419
```

The host must be loopback. There is no fallback to RESTfulAPI, another local
adapter, a shell process or caller-supplied SVDRP text.

## Typed request and hidden local wire

The public C++ transport exposes only:

```text
discoverNativeProbe()
executeNativeProbe(SuiteBridgeNativeProbeRequest)
readNativeProbe(SuiteBridgeNativeProbeReadbackRequest)
```

The unrestricted request primitive remains private. The existing local SVDRP
connection carries two hidden, fixed grammar commands (`NCAP` and `NPROBE`),
but they are not advertised in plugin help and do not create a new listener or
public endpoint.

The typed execute request binds:

```text
commandId
requestFingerprint
operationId
jobId
attemptId
claimEpoch
backendId
agentId
agentInstanceId
backendGeneration
pluginInstanceEpoch
probeNonce
```

No credential, claim token, path, URL, plugin selection, shell fragment or free
native command is accepted.

## Durable Agent execution

The protected Agent state format is version 2 and remains backward-readable for
version-1 `probe.noop` state. The native path persists these distinct values:

```text
native_capability_evidence
plugin_instance_epoch
probe_nonce
native_execution_sequence
native_receipt_evidence
native_result_evidence
native_readback_evidence
```

Required ordering is implemented as:

1. validate the durable assignment and current Agent context;
2. negotiate the exact SuiteBridge capability;
3. parse the allowlisted probe payload;
4. bind the current `pluginInstanceEpoch`;
5. persist `dispatch_state=starting` atomically;
6. invoke the typed local executor;
7. persist normalized native receipt evidence and
   `accepted_by_executor`;
8. persist normalized native result evidence and `effect_reported`;
9. perform typed authoritative readback;
10. persist normalized readback evidence and the generic verified result;
11. send the already persisted generic result to the Control Plane.

No local call occurs before durable `starting`.

A lost first local execute response leaves the protected record at `starting`.
The next local reconciliation repeats the identical typed request in the same
plugin epoch. A valid duplicate returns the original execution sequence and no
second native state capture. A second unclear response becomes
`outcome_unknown`. A changed plugin epoch fences the old request immediately;
the old command is not executed under the new epoch.

## SuiteBridge receipt ledger

SuiteBridge owns a fixed 64-entry in-memory ledger scoped to the generated
`pluginInstanceEpoch`.

For a new request it:

1. validates the exact operation, schema and all bounded identities;
2. rejects a stale epoch;
3. rejects a conflicting fingerprint for an existing `commandId`;
4. reserves a receipt entry;
5. only then captures the bounded VDR active state;
6. stores the receipt, result and execution sequence;
7. returns the typed evidence.

An exact duplicate returns the stored evidence with the same sequence. When the
ledger is full, the request fails closed; an old receipt is never evicted to
make replay unsafe. Restarting VDR creates a new epoch and an empty ledger.

## Readback invariants

The Agent accepts readback only when all of these are exact:

```text
commandId
requestFingerprint
nativeOperation = vdr.native.probe
nativeOperationSchema = 1
pluginInstanceEpoch = accepted receipt epoch
nativeExecutionSequence = accepted receipt sequence
vdrActive = true
mutationsState = disabled
sideEffectObserved = false
readbackCategory = verified
duplicateDisposition = exact_replay
```

Receipt, result and readback are normalized into three separate protected JSON
evidence values. The Control-Plane command result remains the existing bounded
generic result contract; no native payload or secret is added to its wire
schema.

## Tests and guards

The slice adds:

- strict payload, capability and evidence parser tests;
- durable Agent runtime and restart/reconciliation tests;
- Control-Plane native assignment/capability tests;
- typed local SVDRP serialization tests;
- SuiteBridge ledger, replay, conflict, stale-epoch and capacity tests;
- a static runtime guard;
- a real, non-mutating yaVDR acceptance runner.

Local/CI entry points:

```text
make test-phase63-fenced-native-operation-runtime
make test-phase63-runtime-acceptance-harness
make -C vdr-plugin-suite-bridge check
```

## Real yaVDR acceptance

The runner:

- requires root, a clean exact checkout and an explicit expected head;
- builds every candidate from that checkout;
- backs up installed binaries, plugin, systemd override and Agent command state;
- installs and byte-compares the candidate artifacts;
- keeps packaged command activation disabled and uses a temporary explicit
  loopback-only systemd override;
- places a local acceptance proxy before the existing SuiteBridge SVDRP
  transport;
- proves baseline execution, daemon restart, Agent restart, Control-Plane replay
  without native re-execution, lost-response exact replay and plugin-epoch
  fencing;
- compares VDR configuration/recording-state fingerprints before and after;
- restores all original artifacts and service state;
- preserves failed evidence and backup directories.

Copyable root shell block:

```bash
cd /root/vdr-suite || exit 1
git fetch origin agent/phase63-fenced-native-operation-runtime || exit 1
git checkout agent/phase63-fenced-native-operation-runtime || exit 1
test -z "$(git status --porcelain)" || exit 1
EXPECTED_HEAD="$(git rev-parse origin/agent/phase63-fenced-native-operation-runtime)" || exit 1
test "$(git rev-parse HEAD)" = "$EXPECTED_HEAD" || exit 1
PHASE63_EXPECTED_BRANCH=agent/phase63-fenced-native-operation-runtime \
PHASE63_EXPECTED_HEAD="$EXPECTED_HEAD" \
./tools/run_phase63_fenced_native_operation_acceptance.sh \
  2>&1 | tee "/root/phase63-fenced-native-operation-${EXPECTED_HEAD:0:8}.log"
test "${PIPESTATUS[0]}" -eq 0 || exit 1
```

The default evidence directory is:

```text
/root/vdr-suite-phase63-fenced-native-operation-acceptance-<8-char-head>
```

No evidence or backup directory is removed by the runner.

## Explicit exclusions

This slice does not implement Timer, Recording, SearchTimer, Remote, OSD,
channel, EPG, configuration or metadata mutation. It does not add provider
ownership or selection, adapter fallback, a generic SVDRP tunnel, shell/process
execution, a public Agent/plugin endpoint, Phase-64 functionality or
`mutations=enabled`.
