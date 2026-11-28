# Phase 64: SuiteBridge native Timer CREATE mutation

This successor wires the private NTCREATE command service to one dedicated VDR
callback. The callback parses a validated backend-neutral specification into a
local `cTimer`, attaches the stable assignment/binding correlation marker,
obtains a bounded Timer write lock, and performs exactly one `cTimers::Add`
mutation.

F5.6.5 extends the existing private EXEC command with an optional trailing hex-encoded AUX field. The original command is unchanged when AUX is empty. The callback preserves plugin metadata before the existing marker; the durable payload and readback verification retain those bytes independently from the scheduling fingerprint. Native priority and lifetime also accept zero, as the existing API and Web editor already do.

Callback success is only `AppliedUnverified`. It does not prove persistence,
binding, or assignment completion. The correlation marker makes the created
Timer identifiable in the next complete inventory; authoritative readback must
still match the marker and exact expected specification before persisting the
`NativeTimerBinding` or completing the assignment.

NTCREATE remains absent from public SVDRP help. The normal Backend Agent now
constructs the private transport against loopback SuiteBridge, while packaged
command advertisement and Timer-write advertisement remain closed until the
bounded real yaVDR acceptance succeeds.
