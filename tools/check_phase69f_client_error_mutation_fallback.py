#!/usr/bin/env python3
"""Guard the first bounded Phase 69.F client-hardening slice."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def function_body(source: str, function_name: str, next_name: str | None) -> str:
    start = source.find(f"function {function_name}(options)")
    require(start >= 0, f"missing client function: {function_name}")
    if next_name is None:
        end = source.find("window.VdrSuiteClientApi", start)
    else:
        end = source.find(f"function {next_name}(options)", start)
    require(end > start, f"cannot bound client function: {function_name}")
    return source[start:end]


client_api = read("web/frontend/api/client-api.js")
for token in (
    "function createClientError(path, status, payload)",
    "failure.name = 'VdrSuiteClientError'",
    "failure.status = Number(status) || 0",
    "failure.code = errorField(payload, 'code')",
    "failure.requestId = errorField(payload, 'requestId')",
    "failure.correlationId = errorField(payload, 'correlationId')",
    "failure.payload = payload && typeof payload === 'object' ? payload : null",
    "function isClientError(error)",
    "isClientError: isClientError",
):
    require(token in client_api, "structured client-error contract drifted: " + token)

request_start = client_api.find("function requestJson(path, options)")
request_end = client_api.find("function requestBinary(path, options)", request_start)
request_body = client_api[request_start:request_end]
require(
    "throw createClientError(path, response.status, payload);" in request_body,
    "requestJson() must surface structured client errors",
)

binary_start = client_api.find("function requestBinary(path, options)")
binary_end = client_api.find("function requestJsonWithFallback(", binary_start)
binary_body = client_api[binary_start:binary_end]
require(
    "throw createClientError(path, response.status, payload);" in binary_body,
    "requestBinary() must surface the same structured client error",
)

mutation_contracts = (
    (
        "fetchClientSearchTimerExecute",
        "fetchClientSearchTimerRealTest",
        "/api/vdr/searchtimers/execute",
        "/api/searchtimers/execute",
    ),
    (
        "fetchClientSearchTimerCreateAction",
        "fetchClientSearchTimerUpdateAction",
        "/api/vdr/searchtimers",
        "/api/searchtimers",
    ),
    (
        "fetchClientSearchTimerUpdateAction",
        "fetchClientSearchTimerDeleteAction",
        "/api/vdr/searchtimers/update",
        "/api/searchtimers/update",
    ),
    (
        "fetchClientSearchTimerDeleteAction",
        None,
        "/api/vdr/searchtimers/delete",
        "/api/searchtimers/delete",
    ),
)
for function_name, next_name, primary_route, alternate_route in mutation_contracts:
    body = function_body(client_api, function_name, next_name)
    require("jsonPostOptions(options)" in body, f"{function_name} must preserve JSON POST options")
    require("requestJson(" in body and primary_route in body, f"{function_name} primary route drifted")
    require("requestJsonWithFallback" not in body, f"{function_name} restored mutation fallback")
    require(alternate_route not in body, f"{function_name} restored alternate mutation dispatch")

# The remaining read/query compatibility helpers are deliberately still present.
# 69.F must not pretend that a bare 404 proves an unsupported route.
for token in (
    "function requestJsonWithFallback(path, fallbackPath, options)",
    "function requestJsonWithFallbacks(paths, options)",
    "/api/vdr/searchtimers/live",
):
    require(token in client_api, "remaining transition fallback inventory drifted: " + token)

router = read("api/rest/src/ApiRouter.cpp")
for first, second in (
    ("/api/searchtimers/execute", "/api/vdr/searchtimers/execute"),
    ("/api/searchtimers", "/api/vdr/searchtimers"),
    ("/api/searchtimers/update", "/api/vdr/searchtimers/update"),
    ("/api/searchtimers/delete", "/api/vdr/searchtimers/delete"),
):
    pair = f'path == "{first}" ||\n        path == "{second}"'
    require(pair in router, f"server same-handler alias proof drifted: {first} <-> {second}")

test = read("web/frontend/tests/test_phase69f_client_error_mutation_fallback.js")
for token in (
    "revision_conflict",
    "permission_denied",
    "service_unavailable",
    "backend_unavailable",
    "assert.strictEqual(emitted.length, 1)",
    "fetchClientSearchTimerExecute",
    "fetchClientSearchTimerCreateAction",
    "fetchClientSearchTimerUpdateAction",
    "fetchClientSearchTimerDeleteAction",
):
    require(token in test, "focused 69.F regression coverage drifted: " + token)

doc = read("docs/development/phase-69f-client-error-mutation-fallback.md")
for token in (
    "main=5e6d7f7ce2b30bacf9b4bb3d57de8dbc677eb3e1",
    "VdrSuiteClientError",
    "404 not_found",
    "11 wrapper fallback call sites remain",
    "Home/LiveTV",
    "no /api/v1/search-timers resource is invented",
):
    require(token in doc, "69.F slice documentation drifted: " + token)

adr = read("docs/adr/ADR-0048-public-api-versioning-error-compatibility-contract.md")
for token in (
    "fallback logic retries after any error rather than only after a proven unsupported route",
    "The preferred v1 behavior is no mutation path fallback at all.",
    "| `404` | Resource or route not found",
):
    require(token in adr, "ADR-0048 authority drifted: " + token)

current = read("docs/CURRENT.md")
require(
    "[Phase 69.F Client Error and Mutation-Fallback Safety](development/phase-69f-client-error-mutation-fallback.md)" in current,
    "CURRENT must link the first 69.F slice",
)
kickoff = read("docs/development/phase-69-public-api-kickoff.md")
require(
    "[Phase 69.F Client Error and Mutation-Fallback Safety](phase-69f-client-error-mutation-fallback.md)" in kickoff,
    "Phase-69 kickoff must link the first 69.F slice",
)

phase_make = read("mk/phase69-public-api-tests.mk")
for token in (
    "test-phase69f-client-error-mutation-fallback:",
    "node web/frontend/tests/test_phase69f_client_error_mutation_fallback.js",
    "python3 tools/check_phase69f_client_error_mutation_fallback.py",
    "test-frontend-contracts: test-phase69f-client-error-mutation-fallback",
):
    require(token in phase_make, "69.F Make wiring drifted: " + token)

maintenance = read("mk/maintenance-tests.mk")
require(
    "python3 tools/check_phase69f_client_error_mutation_fallback.py" in maintenance,
    "architecture group must run the 69.F guard",
)
require(
    "test-phase69f-client-error-mutation-fallback" in maintenance,
    "phase group must include the 69.F slice",
)

test_groups = read("mk/test-groups.mk")
require(
    "test-phase69f-client-error-mutation-fallback" in test_groups,
    "fast CI must include the 69.F slice",
)

print("Phase 69.F client error and mutation-fallback guard passed.")
print("Client error fields: status / code / requestId / correlationId / payload.")
print("SearchTimer mutations: execute / create / update / delete are one-shot.")
print("Remaining transition fallbacks stay explicit debt pending route-specific proof.")
