#!/usr/bin/env python3
"""Phase 68.D bounded Legacy OSD view-session authorization guard."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "domain": ROOT / "core/vdr/include/LegacyOsdSessionDomain.h",
    "service": ROOT / "core/daemon/src/LegacyOsdSessionService.cpp",
    "api": ROOT / "api/rest/src/LegacyOsdApiRuntime.cpp",
    "daemon": ROOT / "core/daemon/src/DaemonLegacyOsdRuntime.cpp",
    "runtime": ROOT / "core/daemon/src/DaemonRuntime.cpp",
    "shutdown": ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp",
    "router": ROOT / "api/rest/include/ApiRouter.h",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
}

errors = []
contents = {}
for label, path in paths.items():
    if not path.is_file():
        errors.append(f"missing Phase-68.D file: {path.relative_to(ROOT)}")
        contents[label] = ""
    else:
        contents[label] = path.read_text(encoding="utf-8")

required = {
    "domain": [
        "LegacyOsdSession",
        "LegacyOsdSessionState",
        "controlAvailable = false",
        "controlAuthorized = false",
    ],
    "service": [
        'request.permission = "osd.view"',
        "BackendAgentConnectionState::Online",
        "legacy_osd_backend_generation_changed",
        "SessionLifetimeSeconds",
        "MaximumSessions",
    ],
    "api": [
        '"/api/vdr/legacy-osd/sessions"',
        '"/api/vdr/legacy-osd/sessions/status"',
        '"Cache-Control"] = "no-store"',
        "view_only",
    ],
    "daemon": [
        "BackendAgentLifecycleService& lifecycleService",
        "lifecycleService.statusForBackend(",
        "lifecycleService.readOsdObservation(",
        "SecurityPermissionGrantRepository",
    ],
    "runtime": [
        "configureDaemonLegacyOsdRuntime(",
        "*backendAgentLifecycleService_",
        "*backendAgentIdentityRepository_",
    ],
    "shutdown": ["resetDaemonLegacyOsdRuntime();"],
    "router": [
        '#include "LegacyOsdApiRuntime.h"',
        "LegacyOsdApiRuntime::instance().tryHandleGet(",
        "LegacyOsdApiRuntime::instance().tryHandlePost(",
    ],
    "security": [
        'path == "/api/vdr/legacy-osd/sessions"',
        'path == "/api/vdr/legacy-osd/sessions/status"',
        'requestToAuthorize.permission = "osd.view"',
        '"osd.session.create"',
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            errors.append(f"{label} missing required token: {token}")

if 'permission == "osd.view"' in contents["auth"]:
    errors.append(
        "AuthorizationService must not implicitly grant osd.view; "
        "the permission remains an explicit backend-scoped grant"
    )

joined_runtime = "\n".join(
    contents[label] for label in
    ("service", "api", "daemon", "runtime", "router")
)
for forbidden in (
    "OsdControllerLease",
    "osd.control",
    "cRemote",
    "remote.Put",
    "SVDRPCommand",
    "SkinDesigner",
):
    if forbidden in joined_runtime:
        errors.append(
            "Phase-68.D view-session slice pulled later boundary forward: "
            + forbidden
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
            "Legacy OSD session API leaks frame payload field: "
            + forbidden_payload
        )

reset_index = contents["shutdown"].find(
    "resetDaemonLegacyOsdRuntime();"
)
lifecycle_index = contents["shutdown"].find(
    "backendAgentLifecycleService_.reset();"
)
if reset_index < 0 or lifecycle_index < 0 or reset_index > lifecycle_index:
    errors.append(
        "Legacy OSD runtime must reset before BackendAgentLifecycle teardown"
    )

if errors:
    for error in errors:
        print("ERROR:", error)
    raise SystemExit(1)

print("Phase-68.D OSD view-session authorization guard passed")
