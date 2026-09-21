# Phase 63 Slice 3 Closeout — Durable Agent Command Delivery

## Status

Phase 63 Slice 3 is completed, accepted on the real yaVDR host and merged.

The slice was deliberately split into a contract PR and a runtime PR:

```text
PR #142  durable command-delivery contract
  -> PR #143  durable command-delivery runtime
  -> exact-head GitHub CI
  -> guarded real yaVDR acceptance
  -> squash merge to main
```

Phase 63 remains incomplete. The next bounded slice is fenced native operation
execution through one side-effect-free typed native probe.

## Accepted starting point

The completed read-only ingestion foundation before Slice 3 was:

```text
main commit: 39ed86fc3a425697f738f8f555394d54e4e1a684
main tree: e03bb84951cef7ec5f6b2f338ba456116cd766a2
```

That baseline included:

- Phase 63 Slice 1 Agent enrollment, identity, generation and lease;
- Phase 63 Slice 2 generic Observation and Snapshot Ingestion;
- accepted `backend-health` ingestion;
- accepted `channels.conf` ingestion;
- generation-/instance-fenced read-only publication and restart persistence.

## PR #142 — Durable command-delivery contract

Title:

```text
Define durable Agent command delivery contract
```

Exact contract head and tree:

```text
head: 377165c7dff165542a2ab171bc94c97574e044e8
tree: 61b61eb44b848062a1da62e4d8d45346daa5497a
```

Squash merge:

```text
merge commit: 2f613b1b1a889d104b78b96f2d7d638724c315c4
merge tree: 61b61eb44b848062a1da62e4d8d45346daa5497a
```

The contract defined:

- distinct operation, job, attempt, claim, command, Agent-instance and backend-
  generation identities;
- durable Control-Plane assignment, receipt, result and reconciliation state;
- protected Agent-local inbox and result outbox;
- durable receipt before acknowledgement;
- durable `not_started`, `starting`, `accepted_by_executor` and
  `effect_reported` boundaries;
- exact duplicate replay and conflicting duplicate rejection;
- generation, Agent-instance, deadline, attempt and claim-epoch fencing;
- `outcome_unknown` and `waiting_reconciliation` without blind redispatch;
- separate command capabilities;
- no public Agent/provider endpoint;
- no production native command or VDR mutation.

The contract diff contained only:

- `docs/development/phase-63-command-delivery.md`;
- `docs/development/phase-63-slice-2-closeout.md`;
- `tools/check_phase63_command_delivery_contract.py`;
- `mk/phase63-runtime-acceptance.mk`.

It changed no installed runtime behavior and required no real-system acceptance.

### PR #142 validation

Exact-head VDR-Suite CI:

```text
run number: 7300
run id: 31046104286
head: 377165c7dff165542a2ab171bc94c97574e044e8
conclusion: success
```

Green jobs covered documentation, strict Make/test inventory, frontend
regression, packaging/install staging, the complete fast regression suite, the
new contract guard and production daemon build.

## PR #143 — Durable command-delivery runtime

Title:

```text
Add durable Agent command delivery runtime
```

Base:

```text
base commit: 2f613b1b1a889d104b78b96f2d7d638724c315c4
base tree: 61b61eb44b848062a1da62e4d8d45346daa5497a
```

Exact accepted runtime head and tree:

```text
head: e1abe3a9bbcb821398d39249e2d6ada9d8977c3a
tree: 4b4b3c89498bf15397d27dffbf1cbcb114673825
```

Squash merge:

```text
merge commit: 271254a5e5baf83f4a32e974da3d6bec7e33064b
merge tree: 4b4b3c89498bf15397d27dffbf1cbcb114673825
```

The runtime implemented one explicitly non-mutating command type:

```text
probe.noop
```

It added:

- Control-Plane-owned durable assignment, capability, receipt, result and
  reconciliation state;
- authenticated Agent-only poll, receipt and result routes;
- protected Agent-local command state and result outbox;
- durable `starting` before execution;
- exact assignment, receipt and result replay;
- conflicting duplicate rejection;
- Agent-instance, backend-generation, lease, deadline, attempt and claim-epoch
  fencing;
- explicit command capability publication;
- `outcome_unknown` recovery without blind re-execution;
- a local guarded administration utility;
- a guarded real-system acceptance runner;
- no native VDR executor and no production mutation.

Packaged `COMMAND_TYPES` remained empty, so command delivery stayed disabled by
default outside explicit guarded acceptance.

### PR #143 GitHub validation

Exact-head VDR-Suite CI:

```text
run number: 7325
run id: 31073617466
head: e1abe3a9bbcb821398d39249e2d6ada9d8977c3a
conclusion: success
```

Green gates covered:

- documentation checks;
- strict Make/test inventory and complete graph dry-run;
- frontend regression;
- packaging and install staging regression;
- complete fast regression;
- Backend Agent lifecycle HTTP regression;
- command-delivery contract, runtime and acceptance guards;
- command repository/client tests;
- production daemon build.

## Guarded real yaVDR acceptance

The exact accepted head was built and installed locally on the real yaVDR host.
Candidate daemon, Agent, enrollment, administration and command-administration
binaries were byte-compared with the installed files before acceptance.

Final result:

```text
PHASE_63_COMMAND_DELIVERY_UPGRADE_ACCEPTANCE=PASS
HEAD=e1abe3a9bbcb821398d39249e2d6ada9d8977c3a
AGENT_ID=agt_f4af54c8acc53bbc27bbb47a265e3d7d
BASELINE_COMMAND_COMPLETED=yes
COMMAND_REPLAY=yes
LOST_RECEIPT_RESPONSE_RECOVERED=yes
LOST_RESULT_RESPONSE_RECOVERED=yes
DAEMON_RESTART_PERSISTED=yes
AGENT_RESTART_RECOVERED=yes
STALE_GENERATION_COMMAND_NOT_REPLAYED=yes
EXISTING_AGENT_IDENTITY_PRESERVED=yes
CREDENTIAL_GENERATION_PRESERVED=yes
VDR_NATIVE_STATE_UNCHANGED=yes
ORIGINAL_CONFIGURATION_RESTORED=yes
VDR_ACTIVE=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=/root/vdr-suite-phase63-command-acceptance-e1abe3a9
```

Acceptance proved:

- one baseline command completed durably;
- equivalent assignment, receipt and result replay was idempotent;
- deliberately lost receipt and result responses recovered without duplicate
  execution;
- Control-Plane command state survived daemon restart;
- Agent-local state survived Agent restart and generation replacement;
- an old-generation command was not replayed;
- Agent identity and credential generation were preserved;
- VDR-native state remained unchanged;
- original configuration and local state were restored;
- VDR, daemon and Agent remained active.

The runner used repository-owned administration utilities. It performed no
manual SQLite inspection, no enrollment, no revocation, no Agent replacement and
no VDR mutation.

## Final merged Slice-3 state

```text
main commit: 271254a5e5baf83f4a32e974da3d6bec7e33064b
main tree: 4b4b3c89498bf15397d27dffbf1cbcb114673825
```

Slice 3 now provides the durable transport and persistence substrate required by
a later native executor:

```text
Control-Plane durable command assignment
  -> Agent durable receipt and protected inbox
  -> durable starting boundary
  -> persisted result outbox
  -> exact replay and reconciliation
```

It does not yet prove:

- a local SuiteBridge/VDR native executor boundary;
- VDR thread or lock-safe native execution;
- local provider ownership or selection;
- a domain resource revision;
- a production protected write;
- Phase 64 TimerIntent orchestration.

## Next bounded slice

The next contract is:

```text
Phase 63 Slice 4 — Fenced Native Operation Contract
```

It defines exactly one side-effect-free command type:

```text
vdr.native.probe
```

The purpose is to prove the Agent-to-local-adapter-to-SuiteBridge-to-VDR
execution boundary, local replay, plugin-instance epoch fencing and separate
readback without enabling `mutations=enabled` or selecting a provider for normal
domain operations.

A separate runtime PR is required after the contract is merged. Provider
ownership/selection and every production Timer, Recording, SearchTimer, Remote,
configuration or metadata write remain later bounded slices.

## Retained evidence

The following real-system evidence remains intentionally retained:

```text
/root/vdr-suite-phase63-command-acceptance-e1abe3a9
/root/vdr-suite-phase63-binary-backup-before-e1abe3a9
```

Earlier Phase-63 read-only acceptance evidence also remains retained unless the
user explicitly authorizes removal.

## Completion statement

Phase 63 Slice 3 is complete.

Phase 63 as a whole is not complete.
