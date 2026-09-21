# Phase 64 — VDR Managed Timer CREATE Readback Evidence

This adapter builds backend-neutral managed CREATE readback evidence from VDR Timer objects only when a separate complete native Timer inventory evidence object proves the authoritative native-id set for the same backend generation and observation time.

The builder cross-checks the exact native-id set before minting `NativeTimerCreateReadbackEvidence`. A shortened, duplicated or otherwise inconsistent VDR Timer vector therefore cannot be treated as a complete readback.

VDR `aux` remains opaque provider-local data. Only the reserved VDR-Suite managed Timer marker is decoded. Foreign `aux` content is ignored as ownership evidence, malformed or conflicting reserved markers fail closed, and raw `aux` never becomes part of the backend-neutral `NativeTimerBinding` observation model.

Each correlated timer is normalized through `VdrNativeTimerObservationMapper`. The builder deliberately does not collapse duplicate managed correlations: two distinct native Timer identities carrying the same Suite assignment/binding correlation remain two candidates so the Timer-domain CREATE verifier can classify the result as ambiguous.

This component performs no HTTP request, Agent transport, SuiteBridge call, persistence or native mutation. It requires complete native Timer inventory evidence supplied by an authoritative reader and only translates/cross-checks VDR provider-local observations.
