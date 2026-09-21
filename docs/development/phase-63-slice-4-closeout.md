# Phase 63 Slice 4 — Fenced Native Operation Runtime Closeout

## Scope

This closeout belongs only to the `vdr.native.probe` runtime implemented by
[Phase 63 Slice 4](phase-63-fenced-native-operation-runtime.md).

## Immutable candidate identity

Complete after the final CI-green head and real yaVDR run:

```text
PR: pending Draft PR
base: 8150abc2c64c25f1a74a49378368b846f6484df0
head: pending final head
CI run: pending
real acceptance evidence: pending
```

## Required real-host result

The closeout is complete only when the exact final head emits:

```text
PHASE_63_FENCED_NATIVE_OPERATION_ACCEPTANCE=PASS
BASELINE_NATIVE_PROBE_COMPLETED=yes
CONTROL_PLANE_REPLAY_NO_NATIVE_REEXECUTION=yes
LOST_LOCAL_RESPONSE_RECOVERED=yes
PLUGIN_EPOCH_REPLAY_FENCED=yes
NATIVE_RECEIPT_EVIDENCE_SEPARATE=yes
NATIVE_RESULT_EVIDENCE_SEPARATE=yes
AUTHORITATIVE_READBACK_VERIFIED=yes
DAEMON_RESTART_PERSISTED=yes
AGENT_RESTART_RECOVERED=yes
VDR_NATIVE_STATE_UNCHANGED=yes
EXISTING_AGENT_IDENTITY_PRESERVED=yes
CREDENTIAL_GENERATION_PRESERVED=yes
ORIGINAL_CONFIGURATION_RESTORED=yes
VDR_ACTIVE=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
```

Do not mark this closeout complete from unit tests, CI, log inspection or manual
SQLite queries. Preserve the head-specific evidence and backup directory.
