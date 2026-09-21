# Phase 64 — Backend Agent Command Reservation

Managed native CREATE needs one crash-safe handoff between Control-Plane
operation state and the already durable Agent command delivery subsystem.
Inserting directly into `backend_agent_commands` is not sufficient because those
rows are immediately eligible for normal Agent polling.

This component adds an immutable **not pollable** reservation table in front of
the existing delivery table. It does not change the established poll/receipt/
result protocol and it does not add another command lifecycle.

The intended ordering is:

```text
reserve -> dispatching -> activate
```

1. The complete `BackendAgentCommandAssignment` and optional exact local-provider
   selection are durably reserved. No Agent poll can see this row.
2. The owning mutation operation is revision-fenced into `dispatching`.
3. The exact reservation is activated by inserting it into the existing Agent
   command repository.

Every crash boundary is recoverable:

- crash before reservation: the accepted operation can still reserve once;
- crash after reservation but before `dispatching`: the command remains not
  pollable and the same immutable reservation is reused;
- crash after `dispatching` but before activation: recovery activates the
  existing reservation rather than constructing or sending another command;
- crash after activation: existing Agent command delivery, receipt/result
  durability and Agent-side durable-starting rules own recovery.

The reservation is unique by `(backendId, operationId, commandType)` as well as
stable `commandId`. Exact replay returns `alreadyReserved`; a changed command or
selection in the same scope fails as a conflict. Assignment fingerprints and
local-provider selection identities are stored and revalidated on read.

Activation is idempotent. If the same command is already present in the existing
Agent command repository, the service returns `alreadyActivated`. A different
active command in the same operation scope fails closed.

Reservations are intentionally retained after activation as immutable recovery
and audit evidence. They are not exposed to normal polling and are not a second
result/state machine.

This primitive is generic: it contains no Timer CREATE payload semantics. The
CREATE orchestration later supplies a fully validated, generation/provider-
fenced command assignment. This layer only closes the delivery crash gap needed
for the **no blind retry** rule.
