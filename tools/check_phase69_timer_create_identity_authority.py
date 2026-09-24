#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "operation_h": ROOT / "core/operations/include/MutationOperationIdentity.h",
    "operation_cpp": ROOT / "core/operations/src/MutationOperationIdentity.cpp",
    "binding_h": ROOT / "core/timers/include/NativeTimerBindingIdentity.h",
    "binding_cpp": ROOT / "core/timers/src/NativeTimerBindingIdentity.cpp",
    "test": ROOT / "core/timers/tests/test_timer_create_identity_authority.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "preparation": ROOT / "core/timers/src/NativeTimerCreateOperationPreparationService.cpp",
    "public_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "public_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C Timer CREATE identity file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "operation_h": [
        "generateMutationOperationId",
        "mutationOperationIdCanonical",
    ],
    "operation_cpp": [
        'constexpr const char* kPrefix = "op_"',
        "constexpr std::size_t kRandomBytes = 16",
        "std::random_device",
        "return {};",
        "mutationOperationIdCanonical",
    ],
    "binding_h": [
        "generateNativeTimerBindingId",
        "nativeTimerBindingIdCanonical",
    ],
    "binding_cpp": [
        'constexpr const char* kPrefix = "ntb_"',
        "constexpr std::size_t kRandomBytes = 16",
        "std::random_device",
        "return {};",
        "nativeTimerBindingIdCanonical",
    ],
    "test": [
        "generateMutationOperationId",
        "generateNativeTimerBindingId",
        "mutationOperationIdCanonical",
        "nativeTimerBindingIdCanonical",
        '"op_create_dispatch_1"',
        '"native-timer-binding:1"',
    ],
    "daemon_sources": [
        "core/operations/src/MutationOperationIdentity.cpp",
        "core/timers/src/NativeTimerBindingIdentity.cpp",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing Timer CREATE identity marker in {label}: {token}")

for source in required["daemon_sources"]:
    if contents["daemon_sources"].count(source) != 1:
        raise SystemExit(f"daemon must link identity source exactly once: {source}")

for label in ["operation_cpp", "binding_cpp"]:
    for forbidden in [
        "backendAgentGenerateOpaqueId",
        "MetadataIdentity",
        "BrowserSessionRetentionService",
        "steady_clock",
        "system_clock",
        "idCounter",
    ]:
        if forbidden in contents[label]:
            raise SystemExit(
                f"Suite identity issuer acquired foreign/clock authority in {label}: {forbidden}")

# Identity allocation belongs before preparation/orchestration. The accepted
# preparation service still consumes the stable IDs supplied in its request and
# must not silently manufacture replacements on replay or conflict.
for forbidden in [
    "generateMutationOperationId",
    "generateNativeTimerBindingId",
    "backendAgentGenerateOpaqueId",
]:
    if forbidden in contents["preparation"]:
        raise SystemExit(
            f"CREATE preparation must consume, not issue, identities: {forbidden}")

# This prerequisite remains dormant. It must not open public-v1 mutation
# routing/security merely because canonical ID issuers now exist.
for label in ["public_h", "public_cpp", "security"]:
    for forbidden in [
        "MutationOperationIdentity",
        "NativeTimerBindingIdentity",
        "generateMutationOperationId",
        "generateNativeTimerBindingId",
    ]:
        if forbidden in contents[label]:
            raise SystemExit(
                f"Suite identity issuance leaked into public HTTP/security in {label}: {forbidden}")

print("Phase-69.C Timer CREATE identity authority check passed")
print("Boundary: operation and binding IDs remain Suite-issued by owning domains; public admission consumes only durable results")
