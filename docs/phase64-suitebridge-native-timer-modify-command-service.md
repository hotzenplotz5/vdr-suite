# Phase 64: private SuiteBridge native Timer modify service

The private `NTMOD` family implements replay-safe UPDATE and TOGGLE mutation
boundaries. It validates the complete command/provider fence, requires a durable
Agent `starting` timestamp, compares the live native Timer fingerprint under
the VDR Timer write lock, and refuses recording or pending Timers.

A successful callback remains `accepted_unverified`; authoritative inventory
readback is still required before the control plane may persist the new binding
revision. The family is intentionally absent from public SVDRP help and Agent
command advertisement until the bundled yaVDR acceptance passes.
