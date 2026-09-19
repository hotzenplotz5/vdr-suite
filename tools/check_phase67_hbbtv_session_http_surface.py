#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

api = (ROOT / "api/rest/src/HbbtvApiRuntime.cpp").read_text()
router = (ROOT / "api/rest/include/ApiRouter.h").read_text()
gate = (ROOT / "core/security/include/SecurityHttpGate.h").read_text()
gate_normalized = " ".join(gate.split())
authorization = (ROOT / "core/security/include/AuthorizationService.h").read_text()
http = (ROOT / "core/http/src/TestHttpServer.cpp").read_text()
daemon = (ROOT / "core/daemon/src/DaemonHbbtvRuntime.cpp").read_text()

required = (
    (api, '"/api/vdr/broadcast/hbbtv/sessions"'),
    (api, '"/api/vdr/broadcast/hbbtv/sessions/status"'),
    (api, '"/api/vdr/broadcast/hbbtv/sessions/input"'),
    (api, '"/api/vdr/broadcast/hbbtv/sessions/close"'),
    (api, '"/api/vdr/broadcast/hbbtv/sessions/presentation"'),
    (api, '"/api/vdr/broadcast/hbbtv/sessions/media"'),
    (api, "mediaLookup_"),
    (api, 'response.contentType = "image/qoi";'),
    (api, 'sessionService_->authorizePresentation('),
    (api, '"hbbtv_application_context_stale"'),
    (router, "HbbtvApiRuntime::instance().tryHandlePost("),
    (router, "HbbtvApiRuntime::instance().tryHandleGet("),
    (gate_normalized, 'requestToAuthorize.permission = "broadcast.hbbtv.launch";'),
    (gate_normalized, 'requestToAuthorize.permission = "broadcast.hbbtv.input";'),
    (gate_normalized, 'requestToAuthorize.permission = "broadcast.session.manage_own";'),
    (gate, '"broadcast.hbbtv.media"'),
    (authorization, 'permission == "broadcast.hbbtv.launch"'),
    (authorization, 'permission == "broadcast.hbbtv.input"'),
    (authorization, 'permission == "broadcast.session.manage_own"'),
    (http, "hbbtvClientContext(gate.context)"),
    (http, "gate.context.correlationId"),
    (daemon, "findActiveGrantsForActor(actorId)"),
    (daemon, "SecurityConfiguration::fromEnvironment()"),
    (daemon, "securityConfiguration.grants.begin()"),
    (daemon, "securityConfiguration.managedBasic.grants.begin()"),
    (daemon, "AuthorizationService().authorize("),
)

for content, token in required:
    if token not in content:
        raise SystemExit(f"missing HbbTV session HTTP token: {token}")

for forbidden in (
    "urlBase",
    "urlLocation",
    "urlExtension",
    "HBBRUN",
    "HBBPRES",
    "HBBMEDIA",
    "socketPath",
    "LoadUrl",
    "RedButton",
    "ProcessKey",
    "executeJavascript",
    "VK_LEFT",
    "VK_ENTER",
):
    if forbidden in api:
        raise SystemExit(f"public HbbTV session API leaked private detail: {forbidden}")

print("Phase 67 HbbTV session HTTP/security surface: PASS")
