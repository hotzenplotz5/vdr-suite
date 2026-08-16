# Phase 64 SuiteBridge native Timer command-path wiring

This slice installs the already fenced native Timer CREATE and DELETE adapters
in the normal Backend Agent process. The packaged process constructs both
adapters against the explicit loopback SuiteBridge endpoint and injects them
into the durable command client. A claimed Control Plane command can therefore
flow through the Agent executor to the private SuiteBridge command service
without an alternate transport or manual handoff.

The transport endpoint is restricted to loopback and both host and port must be
configured together. Adapter construction performs no mutation and opens no
connection by itself.

The shipped Timer advertisement remains closed: the packaged
`COMMAND_TYPES` value is empty and the command client still suppresses
`vdr.timer.create` and `vdr.timer.delete` from its availability request.
Opening that advertisement is a separate acceptance-gated action after the
exact candidate has passed the bounded real yaVDR test.
