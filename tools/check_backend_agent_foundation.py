#!/usr/bin/env python3
"""Architecture guard for the bounded Phase-63 Backend Agent foundation."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "core/agent/include/BackendAgentLifecycle.h",
    "core/agent/include/BackendAgentHttpServer.h",
    "core/agent/include/BackendAgentClient.h",
    "core/agent/src/BackendAgentRepository.cpp",
    "core/agent/src/BackendAgentLifecycle.cpp",
    "core/agent/src/BackendAgentHttpServer.cpp",
    "core/agent/src/BackendAgentClient.cpp",
    "core/agent/tests/test_backend_agent_lifecycle.cpp",
    "core/agent/tests/test_backend_agent_client.cpp",
    "apps/agent/main.cpp",
    "apps/tools/backend_agent_enrollment_create.cpp",
    "apps/tools/backend_agent_admin.cpp",
    "docs/man/man8/vdr-suite-backend-agent-admin.8",
    "packaging/systemd/vdr-suite-backend-agent.service",
    "packaging/systemd/backend-agent.conf",
    "tools/phase63-runtime-acceptance/backend-agent-foundation.sh",
    "docs/development/phase-63-backend-agent-foundation.md",
]

failures: list[str] = []

for relative in REQUIRED_FILES:
    if not (ROOT / relative).is_file():
        failures.append(f"missing required Phase-63 file: {relative}")

lifecycle = (ROOT / "core/agent/include/BackendAgentLifecycle.h").read_text()
repository = (ROOT / "core/agent/src/BackendAgentRepository.cpp").read_text()
http = (ROOT / "core/agent/src/BackendAgentHttpServer.cpp").read_text()
client = (ROOT / "core/agent/src/BackendAgentClient.cpp").read_text()
agent_main = (ROOT / "apps/agent/main.cpp").read_text()
enrollment_tool = (ROOT / "apps/tools/backend_agent_enrollment_create.cpp").read_text()
admin_tool = (ROOT / "apps/tools/backend_agent_admin.cpp").read_text()
agent_unit = (ROOT / "packaging/systemd/vdr-suite-backend-agent.service").read_text()
agent_config = (ROOT / "packaging/systemd/backend-agent.conf").read_text()
daemon = (ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp").read_text()
make_agent = (ROOT / "mk/agent-sources.mk").read_text()
make_daemon = (ROOT / "mk/daemon-sources.mk").read_text()
schema = (ROOT / "database/schema/vdr-suite.sql").read_text()
contract = (ROOT / "docs/development/phase-63-backend-agent-foundation.md").read_text()
agent_sources = "\n".join(
    path.read_text()
    for path in sorted((ROOT / "core/agent").glob("**/BackendAgent*.cpp"))
)

for symbol in [
    "AgentId", "backendGeneration", "heartbeatSequence", "capabilityRevision",
    "snapshotGeneration", "CommandId", "OperationId",
]:
    if symbol not in contract:
        failures.append(f"contract does not keep identity axis explicit: {symbol}")

for table in [
    "backend_agent_enrollments", "backend_agents",
    "backend_agent_credential_rotations", "backend_agent_capabilities",
    "backend_agent_observation_receipts", "backend_agent_observation_cursors"
]:
    if table not in repository or table not in schema:
        failures.append(f"agent persistence table missing from runtime/schema contract: {table}")

for route in [
    "enroll", "connect", "heartbeat", "capabilities", "credentials/rotate",
    "observations/backend-health",
]:
    if f'/api/agent/v1/{route}' not in http:
        failures.append(f"machine Agent route missing: {route}")


for required in [
    "idx_backend_agents_active_backend",
    "WHERE revoked_at = 0",
]:
    if required not in repository or required not in schema:
        failures.append(f"active-Agent replacement/history constraint missing: {required}")

for required in [
    "pendingObservationKind", "observationSnapshotGeneration",
    "publishBackendHealthObservation", "observation_resync_required",
]:
    if required not in client:
        failures.append(f"restart-safe observation publication missing: {required}")

for required in [
    "backend_agent_observation_receipts", "backend_agent_observation_cursors",
    "BEGIN IMMEDIATE", "canonical_payload", "resync-required",
]:
    if required not in repository:
        failures.append(f"transactional observation repository contract missing: {required}")

for required in [
    "pendingRotationId", "pendingCredentialSecret",
    "reconcilePendingCredentialRotation", "rotateCredential",
]:
    if required not in client:
        failures.append(f"restart-safe credential rotation missing: {required}")

if "--rotate-credential" not in agent_main:
    failures.append("Backend Agent credential rotation CLI path is missing")

for required in ["--status", "--revoke", "statusForBackend", "service.revoke"]:
    if required not in admin_tool:
        failures.append(f"Backend Agent administration path missing: {required}")

for required in ["StateDirectory=vdr-suite/backend-agent", "StateDirectoryMode=0700"]:
    if required not in agent_unit:
        failures.append(f"Agent state ownership contract missing: {required}")

for required in ["BackendAgentHttpServer", "BackendAgentLifecycleService"]:
    if required not in daemon:
        failures.append(f"daemon composition missing: {required}")

if "updateBackendOnline" in agent_sources:
    failures.append(
        "Agent lifecycle must not become BackendRegistry availability authority before provider selection"
    )

if "AGENT_CONTROL_PLANE_SRC" not in make_agent or "$(AGENT_CONTROL_PLANE_SRC)" not in make_daemon:
    failures.append("Agent control-plane sources are not owned by the daemon build graph")

for forbidden in [
    "VdrTimerAction", "RecordingActionExecution", "SearchTimerCreate",
    "SearchTimerUpdate", "SearchTimerDelete", "RemoteActionExecutor",
    "SvdrpChannelMove", "CommandInbox", "ResultOutbox",
]:
    if forbidden in agent_sources:
        failures.append(f"Phase-63 Agent foundation crossed a mutation boundary: {forbidden}")

for secret_field in [
    "providerUrl", "provider_url", "secretPath", "environment", "cookie", "csrf"
]:
    if secret_field in lifecycle:
        failures.append(f"capability/domain contract exposes forbidden field: {secret_field}")

if "readOnly" not in lifecycle or "!facts.readOnly" not in (ROOT / "core/agent/src/BackendAgentLifecycle.cpp").read_text():
    failures.append("read-only capability gate is missing")

for path_name, text in [("HTTP", http), ("Agent client", client)]:
    if "sqlite3_" in text or "SELECT " in text or "INSERT INTO " in text:
        failures.append(f"{path_name} crossed repository-owned SQLite boundary")

if "MaximumAgentBodyBytes" not in http or "agent_payload_too_large" not in http:
    failures.append("Agent protocol body-size gate is missing")

if "Authorization" in agent_sources and "std::cout" in agent_sources:
    failures.append("Agent protocol implementation must not log Authorization material")

for required in [
    'Prefix = "https://"', "CURLOPT_SSL_VERIFYPEER", "CURLOPT_SSL_VERIFYHOST",
    "CURLOPT_FOLLOWLOCATION", "CURLOPT_PROXY", "CURL_NETRC_IGNORED",
    "reconnectInitialSeconds", "reconnectMaximumSeconds", "0600",
]:
    if required not in client:
        failures.append(f"protected outbound Agent client contract missing: {required}")

if "--config" not in agent_main or any(
    secret in agent_main for secret in ["--token", "--password", "--secret"]
):
    failures.append("Backend Agent process arguments are not secret-safe")

if "tokenHash" not in enrollment_tool or "enrollmentToken" not in enrollment_tool:
    failures.append("controlled enrollment utility is missing")
if "std::cout << package.enrollmentToken" in enrollment_tool:
    failures.append("enrollment utility prints bootstrap secret")

for required in [
    "NoNewPrivileges=true", "ProtectSystem=strict", "UMask=0077",
    "ReadWritePaths=/var/lib/vdr-suite/backend-agent",
]:
    if required not in agent_unit:
        failures.append(f"Backend Agent systemd hardening missing: {required}")

for forbidden in ["TOKEN=", "PASSWORD=", "CREDENTIAL_SECRET=", "AUTHORIZATION="]:
    if forbidden in agent_config.upper():
        failures.append(f"packaged Agent configuration contains secret field: {forbidden}")

for forbidden_phase in ["StreamingGateway", "OsdBridge", "TimerOrchestrator"]:
    if forbidden_phase in agent_sources:
        failures.append(f"later-phase implementation leaked into Phase 63: {forbidden_phase}")

if failures:
    print("Backend Agent foundation architecture check failed:")
    for failure in failures:
        print(f"- {failure}")
    sys.exit(1)

print("Backend Agent foundation architecture check passed")
