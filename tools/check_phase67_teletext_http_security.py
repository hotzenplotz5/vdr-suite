#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
SECURITY_GATE = ROOT / "core/security/include/SecurityHttpGate.h"
AUTHORIZATION = ROOT / "core/security/include/AuthorizationService.h"
SECURITY_TEST = ROOT / "core/security/tests/test_teletext_read_security.cpp"
API_ROUTER = ROOT / "api/rest/include/ApiRouter.h"
DAEMON_RUNTIME = ROOT / "core/daemon/src/DaemonTeletextRuntime.cpp"
DAEMON_SOURCES = ROOT / "mk/daemon-sources.mk"

errors: list[str] = []

for path in (
    SECURITY_GATE,
    AUTHORIZATION,
    SECURITY_TEST,
    API_ROUTER,
    DAEMON_RUNTIME,
    DAEMON_SOURCES,
):
    if not path.is_file():
        errors.append(f"missing Phase 67 Teletext HTTP security file: {path.relative_to(ROOT)}")

gate = SECURITY_GATE.read_text(encoding="utf-8") if SECURITY_GATE.is_file() else ""
authorization = AUTHORIZATION.read_text(encoding="utf-8") if AUTHORIZATION.is_file() else ""
test = SECURITY_TEST.read_text(encoding="utf-8") if SECURITY_TEST.is_file() else ""
router = API_ROUTER.read_text(encoding="utf-8") if API_ROUTER.is_file() else ""
daemon = DAEMON_RUNTIME.read_text(encoding="utf-8") if DAEMON_RUNTIME.is_file() else ""
sources = DAEMON_SOURCES.read_text(encoding="utf-8") if DAEMON_SOURCES.is_file() else ""

for fragment in (
    '"/api/vdr/broadcast/teletext/service"',
    '"/api/vdr/broadcast/teletext/page"',
    'teletextRequest.permission = "broadcast.teletext.view";',
    'teletextRequest.action = "broadcast.teletext.view";',
    'teletextRequest.backendId = teletextBackendId;',
    "authorizationService_.authorize(gate.context, teletextRequest)",
    'appendDecisionEvent(gate.context, decision, "")',
):
    if fragment not in gate:
        errors.append(f"SecurityHttpGate missing Teletext read authorization fragment: {fragment}")

for fragment in (
    'permission == "broadcast.teletext.view"',
    "adminReadPermission(permission)",
):
    if fragment not in authorization:
        errors.append(f"AuthorizationService missing Teletext admin-read fragment: {fragment}")

for fragment in (
    'Permission = "broadcast.teletext.view"',
    '"role.admin"',
    '"role.read-only"',
    '"backend_scope_denied"',
    '"invalid_backend_scope"',
    '"permission_denied"',
):
    if fragment not in test:
        errors.append(f"Teletext security test missing regression fragment: {fragment}")

for fragment in (
    '#include "TeletextApiRuntime.h"',
    "TeletextApiRuntime::instance().tryHandleGet(",
):
    if fragment not in router:
        errors.append(f"ApiRouter missing authorized Teletext route fragment: {fragment}")

for fragment in (
    '#include "TeletextApiRuntime.h"',
    "TeletextApiRuntime::instance().configure(*readService)",
    "TeletextApiRuntime::instance().reset();",
):
    if fragment not in daemon:
        errors.append(f"Daemon runtime missing Teletext API ownership fragment: {fragment}")

if "api/rest/src/TeletextApiRuntime.cpp" not in sources:
    errors.append("daemon source manifest missing TeletextApiRuntime.cpp")

for forbidden in (
    "TTXC 1",
    "TTXP 1",
    "/var/cache/vdr/vtx",
    "OsdTeletext::",
):
    if forbidden in gate or forbidden in router or forbidden in daemon:
        errors.append(f"HTTP/security layer must not own provider/wire detail: {forbidden}")

if errors:
    for error in errors:
        print(error, file=sys.stderr)
    raise SystemExit(1)

print("Phase 67 Teletext HTTP security and exposure contract ok")
