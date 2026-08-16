# Phase 64: SuiteBridge native Timer CREATE mutation

This successor wires the private NTCREATE command service to one dedicated VDR callback.
The callback parses a validated backend-neutral specification into a local cTimer, obtains
a bounded Timer write lock, and performs exactly one cTimers::Add mutation.

Callback success is only AppliedUnverified. It does not prove correlation, persistence,
binding, or assignment completion. Those remain gated by authoritative native-Timer
inventory readback and exact managed specification verification.

NTCREATE remains absent from public SVDRP help. Agent construction, packaged command
advertisement, and Timer-write advertisement remain closed until bounded real yaVDR
acceptance succeeds.
