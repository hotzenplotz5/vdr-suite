#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

domain = (ROOT / "core/vdr/include/HbbtvDomain.h").read_text()
resolver = (ROOT / "core/vdr/src/SuiteBridgeHbbtvResolver.cpp").read_text()
control = (ROOT / "core/daemon/src/HbbtvControlPlaneReadService.cpp").read_text()
daemon = (ROOT / "core/daemon/src/DaemonHbbtvRuntime.cpp").read_text()
authority = (
    ROOT / "core/daemon/src/EmbeddedBackendHbbtvAuthority.cpp"
).read_text()
runtime = (ROOT / "core/daemon/src/DaemonRuntime.cpp").read_text()
api = (ROOT / "api/rest/src/HbbtvApiRuntime.cpp").read_text()
router = (ROOT / "api/rest/include/ApiRouter.h").read_text()
security = (ROOT / "core/security/include/SecurityHttpGate.h").read_text()
authorization = (ROOT / "core/security/include/AuthorizationService.h").read_text()
client = (ROOT / "web/frontend/api/client-api.js").read_text()
live = (ROOT / "web/frontend/live-tv-view.js").read_text()
provider_contract = (
    ROOT / "vdr-plugin-suite-bridge/suitebridge_hbbtv_provider_contract.h"
).read_text()

required = {
    "domain": (
        (domain, "struct BroadcastApplicationRef"),
        (domain, "struct BroadcastApplicationDescriptor"),
        (domain, "struct BroadcastApplicationDiscoverySnapshot"),
        (domain, "backendGeneration"),
        (domain, "descriptorRevision"),
    ),
    "resolver": (
        (resolver, '"vdr-plugin-web"'),
        (resolver, '"broadcast.hbbtv.discovery"'),
        (resolver, "snapshot.backendGeneration = backendGeneration;"),
    ),
    "control-plane": (
        (control, "hbbtv_backend_generation_mismatch"),
        (control, "hbbtv_channel_not_in_backend_snapshot"),
        (control, "resolver->discoverApplications("),
    ),
    "daemon": (
        (daemon, "EmbeddedBackendHbbtvAuthority"),
        (daemon, "BackendAgentLifecycleService& backendAgentLifecycleService"),
        (daemon, "ensureHbbtvResolver()"),
        (authority, "agentLifecycleService_.statusForBackend(backendId, now)"),
        (authority, "generations.latestGeneration(backendId)"),
        (authority, "agent.backendGeneration == latestGeneration"),
        (authority, "BackendAgentConnectionState::Online"),
        (authority, "lifecycleService_.statusForBackend(backendId, now)"),
        (runtime, "configureDaemonHbbtvRuntime("),
        (runtime, "resetDaemonHbbtvRuntime();"),
    ),
    "http": (
        (api, '"/api/vdr/broadcast/hbbtv/applications"'),
        (api, '"autostart"'),
        (api, '"present"'),
        (api, r'\"available\":'),
        (router, '#include "HbbtvApiRuntime.h"'),
        (router, "HbbtvApiRuntime::instance().tryHandleGet("),
    ),
    "security": (
        (security, 'path == "/api/vdr/broadcast/hbbtv/applications"'),
        (
            security,
            "if (isHbbtvDiscoveryRead || isHbbtvPresentationRead ||",
        ),
        (
            security,
            "hbbtvRequest.permission =",
        ),
        (
            security,
            "hbbtvRequest.action = isHbbtvPresentationRead",
        ),
        (security, ': "broadcast.hbbtv.view";'),
        (authorization, 'permission == "broadcast.hbbtv.view"'),
    ),
    "frontend": (
        (client, "function fetchClientHbbtvApplications(options)"),
        (client, "requestJson('/api/vdr/broadcast/hbbtv/applications', options)"),
        (client, "fetchClientHbbtvApplications: fetchClientHbbtvApplications"),
        (live, "vdr-suite-hbbtv-availability"),
        (live, "beginHbbtvAvailability(channel)"),
        (live, "attempt < 4"),
        (live, "HbbTV verfügbar"),
    ),
    "provider-pin": (
        (provider_contract, "hotzenplotz5/vdr-plugin-web"),
        (
            provider_contract,
            "9ee1697a435e01058df6890323bf979a1ad2fd87",
        ),
        (
            provider_contract,
            "34ded5090fbad021338c491355566dbdb4d98f9d",
        ),
    ),
}

errors = []
for group, checks in required.items():
    for content, token in checks:
        if token not in content:
            errors.append(f"{group}: missing token: {token}")

for forbidden in (
    "urlBase",
    "urlLocation",
    "urlExtension",
    "openArbitraryUrl",
    "executeJavascript",
    "rawKey",
    "LoadUrl",
    "ProcessKey",
):
    if forbidden in api:
        errors.append(f"http: provider/browser detail leaked: {forbidden}")

for forbidden in (
    "HBBAPPS",
    "HBBRUN",
    "HBBPRES",
    "LoadUrl",
    "ProcessKey",
    "executeJavascript",
    "VK_LEFT",
    "VK_ENTER",
):
    if forbidden in live:
        errors.append(f"frontend: private provider detail leaked: {forbidden}")

if errors:
    raise SystemExit("\n".join(errors))

print("Phase 67 HbbTV discovery public surface: PASS")
