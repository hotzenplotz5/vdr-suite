#!/usr/bin/env python3
"""Phase 68.E bounded Legacy OSD viewer-binding and delivery guard."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "domain": ROOT / "core/vdr/include/LegacyOsdViewerDomain.h",
    "service_h": ROOT / "core/daemon/include/OsdViewerBindingService.h",
    "service": ROOT / "core/daemon/src/OsdViewerBindingService.cpp",
    "session": ROOT / "core/daemon/src/LegacyOsdSessionService.cpp",
    "api": ROOT / "api/rest/src/LegacyOsdApiRuntime.cpp",
    "daemon": ROOT / "core/daemon/src/DaemonLegacyOsdRuntime.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
    "agent": ROOT / "core/agent/src/BackendAgentOsdObservationService.cpp",
}

errors = []
contents = {}
for label, path in paths.items():
    if not path.is_file():
        errors.append(f"missing Phase-68.E file: {path.relative_to(ROOT)}")
        contents[label] = ""
    else:
        contents[label] = path.read_text(encoding="utf-8")

required = {
    "domain": [
        "struct OsdViewerBinding",
        "legacyOsdSessionId",
        "backendGeneration",
        "osdSurfaceId",
        "osdEpoch",
        "lastAcknowledgedFrameSequence",
        "lastDeliveredFrameSequence",
        "lastAcknowledgedEventSequence",
        "controlAuthorized = false",
    ],
    "service_h": [
        "MaximumBindings = 64",
        "MaximumBindingsPerSession = 8",
        "OsdViewerBindingResult attach(",
        "OsdViewerDeliveryResult read(",
        "OsdViewerBindingResult detach(",
    ],
    "service": [
        "sessionService_.status(",
        "viewer_backpressure_resync_required",
        "viewer_sequence_gap",
        "osd_surface_or_epoch_changed",
        "viewer_resume_resync_required",
        "legacy_osd_backend_generation_changed",
        "osdFrameWithinLimits(frame)",
    ],
    "api": [
        '"/api/vdr/legacy-osd/viewers"',
        '"/api/vdr/legacy-osd/viewers/detach"',
        '"Cache-Control"] = "no-store"',
    ],
    "daemon": [
        "std::unique_ptr<OsdViewerBindingService>",
        "std::make_unique<OsdViewerBindingService>(*sessions)",
        "configure(*sessions, *viewers)",
    ],
    "security": [
        '"osd.viewer.attach"',
        '"osd.viewer.detach"',
        'requestToAuthorize.permission = "osd.view"',
    ],
    "agent": [
        "A bounded latest-value cache",
        "readOsdObservation(",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            errors.append(f"{label} missing required token: {token}")


# Public API owns viewer-binding lifecycle only. ADR-0048 keeps sequenced
# OSD frame/delta delivery on the separately defined compatibility plane.

for forbidden_public_delivery in (
    '"/api/vdr/legacy-osd/viewers/frame"',
    "frameJson(",
    "deliveryJson(",
    '"osd.viewer.frame"',
):
    if forbidden_public_delivery in contents["api"] or \
       forbidden_public_delivery in contents["security"]:
        errors.append(
            "68.E must not expose sequenced OSD delivery through public REST: "
            + forbidden_public_delivery
        )

viewer_runtime = "\n".join(
    contents[label] for label in ("domain", "service_h", "service", "api")
)
for forbidden in (
    "OsdControllerLease",
    "osd.control",
    "cRemote",
    "remote.Put",
    "SVDRPCommand",
    "SkinDesigner",
    "std::deque",
    "std::queue",
    "ofstream",
    "sqlite3",
):
    if forbidden in viewer_runtime:
        errors.append(
            "Phase-68.E viewer slice pulled forbidden/later boundary forward: "
            + forbidden
        )

service = contents["service"]
for forbidden_owner in (
    "BackendAgentLifecycleService",
    "BackendAgentOsdObservation",
    "SecurityPermissionGrantRepository",
    "Database",
):
    if forbidden_owner in service:
        errors.append(
            "viewer service created a parallel lifecycle/cache/security owner: "
            + forbidden_owner
        )

api = contents["api"]
session_start = api.find("std::string sessionJson(")
session_end = api.find("int errorStatus(", session_start)
session_api = api[session_start:session_end]
for forbidden_payload in (
    "frame.title",
    "statusMessage",
    "presentTitle",
    "followingTitle",
    "frame.items",
):
    if forbidden_payload in session_api:
        errors.append(
            "68.D session admission/status serializer leaks frame payload: "
            + forbidden_payload
        )

for payload_token in (
    "PRIVATE_VIEWER_FRAME_TEXT",
    "frame.title",
    "frame.items",
):
    if payload_token in contents["daemon"] or payload_token in contents["security"]:
        errors.append(
            "OSD frame payload crossed into daemon/security logging or audit owner: "
            + payload_token
        )

if errors:
    for error in errors:
        print("ERROR:", error)
    raise SystemExit(1)

print("Phase-68.E OSD viewer-binding delivery guard passed")
