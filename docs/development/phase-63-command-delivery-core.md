# Phase 63 Slice 3A — Durable Agent Command Delivery Core

## Status

Bounded runtime foundation for the merged Phase-63 command-delivery contract.
This slice implements durable command state only. It does not expose command
HTTP routes, poll the Control Plane, execute a native VDR operation, select a
provider or enable a protected write.

Base at slice creation:

```text
main commit: 2f613b1b1a889d104b78b96f2d7d638724c315c4
merged contract: Phase 63 Slice 3 command delivery
```

## Included

- a bounded `vdr-suite-agent/1` command envelope with distinct operation, job,
  attempt, claim, command, Agent-instance and Backend-generation identities;
- fail-closed envelope validation and payload/deadline bounds;
- Suite-owned SQLite command ledger schema and repository boundary;
- idempotent assignment creation and conflicting-fingerprint rejection;
- generation-, instance-, attempt- and claim-bound receipt/result recording;
- deterministic pending-assignment lookup for one exact current Agent binding;
- exact result acknowledgement before retirement eligibility;
- a service-private local Agent command store using atomic replacement files;
- durable receipt persistence before an assignment is reported accepted;
- restart-safe `not_started`, `starting`, `accepted_by_executor` and
  `effect_reported` boundaries;
- exact duplicate receipt/result replay and conflicting replay rejection;
- persisted result outbox enumeration before new transport work;
- focused C++ coverage integrated into `test-fast`.

## Local persistence boundary

The local store writes one bounded record per `commandId` beneath its supplied
Agent state directory. The directory is forced to mode `0700` and command
records to mode `0600` on POSIX systems. Writes use a same-directory temporary
file followed by atomic rename. A malformed, duplicate-key, oversized or
invalidly encoded record is not reconstructed as authority.

No credential, authorization header, cookie, CSRF value, provider URL or
private key belongs in a command record. The command payload is bounded to
16 KiB and diagnostics to 4 KiB.

## Safety properties

- an assignment is not accepted when its Agent ID, Agent instance or Backend
  generation differs from current local authority;
- an expired assignment is not persisted as accepted;
- the same command identity plus fingerprint returns the existing durable
  receipt/result;
- the same command identity with another fingerprint fails as a conflict;
- `starting` is persisted before a later executor may be invoked;
- a result remains in the local outbox across restart until the exact
  fingerprint is acknowledged;
- Control-Plane receipt/result writes require the complete persisted fence;
- transport timeout is not interpreted as success, failure or safe retry.

## Explicit exclusions

This slice contains no:

- Agent command HTTP route or polling loop;
- command serialization on the wire;
- production command capability advertisement;
- native executor or VDR lock crossing;
- Recording, Timer, SearchTimer, Remote, configuration or metadata mutation;
- provider ownership or SuiteBridge selection;
- public Agent/provider endpoint;
- Phase 64 TimerIntent implementation;
- automatic migration of existing direct-adapter authority.

The only command type used in tests is `delivery-probe`, a non-mutating fixture
name. It is not registered as a production capability and cannot be dispatched
through the installed Agent.

## Automated evidence

`make test-backend-agent-command-delivery` compiles the complete command core,
including its SQLite repository implementation, and proves:

- bounded envelope validation;
- durable receipt before acceptance;
- restart reconstruction;
- equivalent duplicate replay;
- conflicting duplicate rejection;
- stale Agent-instance/generation rejection;
- deadline rejection;
- one-way dispatch-state transitions;
- durable result persistence and equivalent replay;
- conflicting result rejection;
- outbox replay after restart;
- exact acknowledgement and persisted retirement state.

The target is included in `test-fast`, so normal exact-head CI compiles and runs
it alongside the existing Agent lifecycle/client suites.

## Exit and next slice

This core slice is complete only after exact-head GitHub CI is green. It changes
library/runtime code but does not wire installed daemon or Agent behaviour, so
it does not itself require a yaVDR service restart or runtime acceptance.

The next bounded Draft must wire the protected outbound-only Agent transport to
this core, add authenticated assignment/receipt/result routes, preserve
reconnect ordering, and provide an exact-head non-mutating real yaVDR
acceptance runner. Native commands remain excluded until a later separate
contract and acceptance.
