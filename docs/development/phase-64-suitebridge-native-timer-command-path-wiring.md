# Phase 64 SuiteBridge native Timer command-path wiring

This slice installs the fenced native Timer CREATE, UPDATE, TOGGLE and DELETE
adapters in the normal Backend Agent process. The packaged process constructs
the adapters against the explicit loopback SuiteBridge endpoint and injects
them into the durable command client. A claimed Control Plane command can flow
through the Agent executor to the private SuiteBridge command service without
an alternate transport or manual handoff.

The transport endpoint is restricted to loopback and both host and port must be
configured together. Adapter construction performs no mutation and opens no
connection by itself.

The exact closed candidate
`692ac34f3aee1ead70cab17425a4f9e74f091792` passed the bounded real-yaVDR
acceptance before activation. The shipped `COMMAND_TYPES` value now contains
exactly `vdr.timer.create,vdr.timer.update,vdr.timer.toggle,vdr.timer.delete`.

Advertisement remains fail-closed at runtime. Every poll rediscovers the private
SuiteBridge capabilities. A command type is included only when its service
reports `enabled`, the provider facts are valid and the required capability is
present. CREATE, UPDATE/TOGGLE and DELETE facts are merged into one
`suitebridge:local` record only when instance epoch, provider generation and
capability revision agree. A conflicting discovery snapshot suppresses all
Timer command types for that poll.

The Control Plane validates that every advertised Timer type has the matching
provider capability and rechecks the persisted provider selection before
delivery and receipt. The public SuiteBridge SVDRP Help advertisement remains
closed for `NTCREATE`, `NTMOD` and `NTDELETE`; these commands remain a
private loopback transport contract.
