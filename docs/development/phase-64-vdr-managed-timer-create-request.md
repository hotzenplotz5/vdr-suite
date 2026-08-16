# Phase 64 — VDR Managed Timer CREATE Request

This adapter converts the backend-neutral `NativeTimerSpecification` plus a pre-reserved VDR-Suite managed Timer correlation into one provider-local `VdrTimerOperationRequest` for CREATE.

The cross-backend specification remains free of native IDs and raw plugin data. The VDR adapter adds the stable `timerAssignmentId` / `nativeTimerBindingId` marker only in provider-local aux. Existing provider-local aux is preserved byte-for-byte before the marker, and a malformed or conflicting reserved marker fails closed.

HHMM values are validated by `NativeTimerSpecification` before conversion to the integer form expected by the existing VDR operation primitive. Midnight (`0000`) is valid and maps to integer `0`; the old `VdrTimerOperationRequest::hasTimeWindow()` convenience helper is not used as an authority gate.

The builder maps only managed write fields: channel, title/directory, day/weekdays, start/stop, priority/lifetime, enabled and VPS. It leaves the native timer ID empty because native identity does not exist before CREATE readback.

This component does not dispatch, persist, select a provider, call SuiteBridge or mutate VDR. Durable starting, generation/provider fencing and exactly-one native invocation belong to the later managed execution path.
