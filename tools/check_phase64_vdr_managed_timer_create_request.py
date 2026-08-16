#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

files = [
    "core/vdr/include/VdrManagedTimerCreateRequestBuilder.h",
    "core/vdr/src/VdrManagedTimerCreateRequestBuilder.cpp",
    "core/vdr/tests/test_vdr_managed_timer_create_request_builder.cpp",
    "docs/development/phase-64-vdr-managed-timer-create-request.md",
    "mk/phase64-vdr-managed-timer-create-request-tests.mk",
    "Makefile",
]
for relative in files:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing VDR managed CREATE request file: {relative}")

header = (ROOT / files[0]).read_text(encoding="utf-8")
source = (ROOT / files[1]).read_text(encoding="utf-8")
test = (ROOT / files[2]).read_text(encoding="utf-8")
doc = (ROOT / files[3]).read_text(encoding="utf-8")
mk = (ROOT / files[4]).read_text(encoding="utf-8")
makefile = (ROOT / files[5]).read_text(encoding="utf-8")

for token in [
    "VdrManagedTimerCreateRequestBuildStatus",
    "invalidBackendIdentity",
    "invalidSpecification",
    "invalidCorrelation",
    "auxConflict",
    "class VdrManagedTimerCreateRequestBuilder",
]:
    if token not in header:
        raise SystemExit(f"missing VDR managed CREATE request header marker: {token}")

for token in [
    "nativeTimerSpecificationValid(specification)",
    "vdrTimerManagedCorrelationValid(correlation)",
    "attachVdrTimerManagedCorrelation(baseAux, correlation)",
    "request.backendId = backendId",
    "request.channelId = specification.channelId",
    "request.start = hhmmToInt(specification.startTime)",
    "request.active = specification.enabled",
    "request.aux = attached.aux",
]:
    if token not in source:
        raise SystemExit(f"missing VDR managed CREATE request source marker: {token}")

for forbidden in [
    "IHttpClient", "RestfulApi", "SuiteBridge", "BackendAgentCommand", "SVDRP",
    "repository_", "MutationOperation",
]:
    if forbidden in header + source:
        raise SystemExit(f"premature VDR managed CREATE request boundary crossing: {forbidden}")

for token in [
    "replay.request.aux == built.request.aux",
    "midnightBuilt.request.start == 0",
    "invalidBackendIdentity",
    "invalidSpecification",
    "invalidCorrelation",
    "auxConflict",
    "test_vdr_managed_timer_create_request_builder passed",
]:
    if token not in test:
        raise SystemExit(f"missing VDR managed CREATE request regression marker: {token}")

for token in [
    "NativeTimerSpecification",
    "provider-local aux",
    "midnight",
    "does not dispatch",
]:
    if token not in doc:
        raise SystemExit(f"missing VDR managed CREATE request documentation marker: {token}")

include_line = "include mk/phase64-vdr-managed-timer-create-request-tests.mk"
if include_line not in makefile:
    raise SystemExit("VDR managed CREATE request make fragment is not included")

for token in [
    "test-phase64-vdr-managed-timer-create-request-architecture",
    "test-phase64-vdr-managed-timer-create-request:",
    "test-fast: test-phase64-vdr-managed-timer-create-request",
    "test-architecture: test-phase64-vdr-managed-timer-create-request-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing VDR managed CREATE request make marker: {token}")

print("Phase-64 VDR managed Timer CREATE request check passed")
