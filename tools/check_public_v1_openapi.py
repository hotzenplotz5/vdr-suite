#!/usr/bin/env python3
"""Fail-closed, dependency-free guard for the audited partial Public-v1 OpenAPI.

This deliberately checks ONLY the explicitly audited methods below. It is NOT
full route discovery, an OpenAPI Schema validator or runtime HTTP acceptance.
To extend coverage, review the actual runtime, security gate and tests first.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
OPENAPI = ROOT / "api/openapi.json"
RUNTIME = ROOT / "api/rest/src/PublicApiRuntime.cpp"
DEVICE_RUNTIME = ROOT / "core/daemon/src/RecordingMediaHttpRuntime.cpp"
DEVICE_GATE = ROOT / "core/security/include/SecurityHttpGate.h"

# Each anchor is an actual branch in the corresponding C++ GET/POST dispatcher.
# Method-specific bounds prevent mistaking a POST 405 fallback for a real route.
# This explicit inventory must be reviewed when already-audited routes move.
AUDITED = {
    "/api/v1": {"get": 'if (path == "/api/v1")'},
    "/api/v1/capabilities": {"get": 'if (path == "/api/v1/capabilities")'},
    "/api/v1/backends": {"get": "if (path == PublicBackendCollectionPath)"},
    "/api/v1/channels": {"get": "if (path == PublicChannelCollectionPath)"},
    "/api/v1/recordings": {"get": "if (path == PublicRecordingCollectionPath)"},
    "/api/v1/search/recordings": {"get": "if (path == PublicSearchRecordingPath || path == PublicSearchEpgPath)"},
    "/api/v1/search/epg": {"get": "if (path == PublicSearchRecordingPath || path == PublicSearchEpgPath)"},
    "/api/v1/genres": {"get": "if (path == PublicGenreCollectionPath || path == PublicGenreRecordingPath)"},
    "/api/v1/genres/recordings": {"get": "if (path == PublicGenreCollectionPath || path == PublicGenreRecordingPath)"},
    "/api/v1/epg/now-next": {"get": "if (path == PublicEpgNowNextPath)"},
    "/api/v1/epg/window": {"get": "if (path == PublicEpgWindowPath)"},
    "/api/v1/native-timers": {"get": "if (path == PublicNativeTimersPath)"},
    "/api/v1/native-timers/{timerId}": {"get": "if (publicNativeTimerItemPath(path, nativeTimerId))"},
    "/api/v1/epg/timer-intents": {"post": "if (path == PublicEpgTimerIntentPath)"},
    "/api/v1/timer-assignments": {"get": "if (path == PublicTimerAssignmentCollectionPath)"},
    "/api/v1/search-timers": {"get": "if (path == PublicSearchTimerCollectionPath)", "post": "if (path == PublicSearchTimerCollectionPath)"},
    "/api/v1/search-timers/draft-preview": {"get": "if (path == PublicSearchTimerDraftPreviewPath)"},
    "/api/v1/search-timers/{searchTimerId}/preview": {"get": "if (publicSearchTimerPreviewPath(path, previewSearchTimerId))"},
    "/api/v1/timer-assignments/{timerAssignmentId}": {
        "get": "if (publicTimerAssignmentPath(",
        "post": "if (publicTimerAssignmentPath(",
    },
    "/api/v1/operations/{operationId}": {"get": "if (publicOperationPath("},
    "/api/v1/accounts": {
        "get": "if (path == PublicAccountCollectionPath)",
        "post": "if (path == PublicAccountCollectionPath)",
    },
    "/api/v1/accounts/{accountId}": {
        "get": "if (publicAccountPath(",
        "post": "if (publicAccountPath(",
    },
    "/api/v1/accounts/{accountId}/grants": {
        "get": "if (publicAccountGrantPath(",
        "post": "if (publicAccountGrantPath(",
    },
    "/api/v1/device-pairings": {
        "get": "if (path == PublicDevicePairingCollectionPath)",
        "post": "if (path == PublicDevicePairingCollectionPath)",
    },
    "/api/v1/device-pairings/{pairingRequestId}": {
        "get": "if (publicDevicePairingPath(",
        "post": "if (publicDevicePairingPath(",
    },
    "/api/v1/device-pairings/{pairingRequestId}/credential": {
        "post": 'const std::string issueSuffix = "/credential";',
    },
    "/api/v1/devices/{deviceId}/grants": {
        "get": "if (publicDeviceGrantPath(",
        "post": "if (publicDeviceGrantPath(",
    },
    "/api/v1/devices/{deviceId}/lifecycle": {
        "get": "if (publicDeviceLifecyclePath(",
        "post": "if (publicDeviceLifecyclePath(",
    },
    "/api/v1/devices/{deviceId}/credentials/{credentialId}/lifecycle": {
        "get": "if (publicDeviceLifecyclePath(",
        "post": "if (publicDeviceLifecyclePath(",
    },
    "/api/v1/devices/{deviceId}/credentials/{credentialId}/rotate": {
        "post": "if (publicDeviceCredentialRotationPath(",
    },
    "/api/v1/accounts/{accountId}/credentials": {
        "get": "if (publicAccountCredentialPath(",
    },
    "/api/v1/accounts/{accountId}/sessions": {
        "get": "if (publicAccountSessionPath(",
    },
    "/api/v1/accounts/{accountId}/credentials/{credentialId}": {
        "get": "if (publicAccountCredentialItemPath(",
        "post": "if (publicAccountCredentialItemPath(",
    },
    "/api/v1/accounts/{accountId}/sessions/{sessionId}": {
        "get": "if (publicAccountSessionItemPath(",
        "post": "if (publicAccountSessionItemPath(",
    },
}

# Versioned Recording playback is registered in the daemon media composition
# root, rather than PublicApiRuntime. Require an actual route branch AND its
# SecurityHttpGate Device-only admission marker for every documented method.
# The GET status and track branches use an empty body to distinguish reads.
DEVICE_AUDITED = {
    "/api/v1/recording-playback-sessions/capabilities": {
        "get": ('const std::string capabilityCollection = collection + "/capabilities";',
                'isPublicDeviceRecordingSessionRead'),
    },
    "/api/v1/recording-playback-sessions": {
        "post": ('if (target == collection)', 'isPublicDeviceRecordingSessionCreate'),
    },
    "/api/v1/recording-playback-sessions/{sessionId}": {
        "get": ('target.size() == prefix.size() + 35U &&',
                'isPublicDeviceRecordingSessionRead'),
    },
    "/api/v1/recording-playback-sessions/{sessionId}/trickplay": {
        "get": ('target.find("/trickplay", prefix.size())',
                'isPublicDeviceRecordingSessionRead'),
    },
    "/api/v1/recording-playback-sessions/{sessionId}/tracks": {
        "get": ('const std::string tracksSuffix = "/tracks";',
                'isPublicDeviceRecordingSessionRead'),
    },
    "/api/v1/recording-playback-sessions/{sessionId}/subtitles": {
        "post": ('const std::string subtitleSuffix = "/subtitles";',
                 'isPublicDeviceRecordingSubtitleSelection'),
    },
    "/api/v1/recording-playback-sessions/{sessionId}/stop": {
        "post": ('target.compare(target.size() - 5U, 5U, "/stop")',
                 'isPublicDeviceRecordingSessionStop'),
    },    "/api/v1/live-playback-sessions": {
        "post": ('if (target == collection)', 'isPublicDeviceLiveSessionCreate'),
    },
    "/api/v1/live-playback-sessions/{sessionId}": {
        "get": ('const std::string id = target.substr(prefix.size());',
                'isPublicDeviceLiveSessionRead'),
    },
    "/api/v1/live-playback-sessions/{sessionId}/stop": {
        "post": ('const std::string suffix = "/stop";',
                 'isPublicDeviceLiveSessionStop'),
    },
}

# The three intentionally anonymous bootstrap/discovery cases and dual-mode
# pairing poll/admin GET need explicit human review of the x-vdr-suite-authentication
# extension. The OpenAPI securitySchemes model is not yet complete.
NO_ACTOR = {
    ("get", "/api/v1"),
    ("get", "/api/v1/capabilities"),
    ("post", "/api/v1/device-pairings"),
    ("get", "/api/v1/device-pairings/{pairingRequestId}"),
    ("post", "/api/v1/device-pairings/{pairingRequestId}/credential"),
}

IF_MATCH = {
    ("post", "/api/v1/epg/timer-intents"),
    ("post", "/api/v1/timer-assignments/{timerAssignmentId}"),
    ("post", "/api/v1/accounts/{accountId}"),
    ("post", "/api/v1/accounts/{accountId}/grants"),
    ("post", "/api/v1/device-pairings/{pairingRequestId}"),
    ("post", "/api/v1/devices/{deviceId}/grants"),
    ("post", "/api/v1/devices/{deviceId}/lifecycle"),
    ("post", "/api/v1/devices/{deviceId}/credentials/{credentialId}/lifecycle"),
    ("post", "/api/v1/devices/{deviceId}/credentials/{credentialId}/rotate"),
    ("post", "/api/v1/accounts/{accountId}/credentials/{credentialId}"),
    ("post", "/api/v1/accounts/{accountId}/sessions/{sessionId}"),
}

IDEMPOTENCY = {
    ("post", "/api/v1/epg/timer-intents"),
    ("post", "/api/v1/search-timers"),
    ("post", "/api/v1/accounts"),
    ("post", "/api/v1/timer-assignments/{timerAssignmentId}"),
}
METHODS = {"get", "post", "put", "patch", "delete", "head", "options", "trace"}
PARAM_PATTERN = re.compile(r"\{([^{}]+)\}")


def _walk_refs(value: Any):
    if isinstance(value, dict):
        for key, item in value.items():
            if key == "$ref":
                yield item
            else:
                yield from _walk_refs(item)
    elif isinstance(value, list):
        for item in value:
            yield from _walk_refs(item)


def _handler_sections(source: str) -> dict[str, str]:
    bounds = [
        source.find("bool PublicApiRuntime::tryHandleGet("),
        source.find("bool PublicApiRuntime::tryHandlePost("),
        source.find("bool PublicApiRuntime::tryHandleUnsupportedMethod("),
    ]
    if any(i < 0 for i in bounds) or not bounds[0] < bounds[1] < bounds[2]:
        return {}
    return {
        "get": " ".join(source[bounds[0]:bounds[1]].split()),
        "post": " ".join(source[bounds[1]:bounds[2]].split()),
    }


def validate(
    spec: Any, source: str,
    device_source: str | None = None,
    security_source: str | None = None,
) -> list[str]:
    """Return all detected contract drift without mutating a file or system."""
    errors: list[str] = []
    if device_source is None:
        device_source = DEVICE_RUNTIME.read_text(encoding="utf-8")
    if security_source is None:
        security_source = DEVICE_GATE.read_text(encoding="utf-8")
    if not isinstance(spec, dict):
        return ["OpenAPI root must be a JSON object"]

    if spec.get("openapi") != "3.1.0":
        errors.append("OpenAPI version must remain 3.1.0")
    if spec.get("x-vdr-suite-coverage") != "partial":
        errors.append("OpenAPI must explicitly retain partial coverage")
    if not isinstance(spec.get("info"), dict):
        errors.append("missing OpenAPI info object")
    elif not spec["info"].get("title") or not spec["info"].get("version"):
        errors.append("OpenAPI info.title and info.version are required")

    paths = spec.get("paths")
    if not isinstance(paths, dict):
        return errors + ["OpenAPI paths must be an object"]
    expected_paths = set(AUDITED) | set(DEVICE_AUDITED)
    if set(paths) != expected_paths:
        missing = sorted(expected_paths - set(paths))
        extra = sorted(set(paths) - expected_paths)
        errors.append(f"audited path inventory changed: missing={missing}, extra={extra}; audit new routes before expanding")

    section = _handler_sections(source)
    if not section:
        errors.append("cannot isolate actual GET and POST C++ handler bodies")
    schemas = spec.get("components", {}).get("schemas", {}) if isinstance(spec.get("components"), dict) else {}
    if not isinstance(schemas, dict):
        schemas = {}
        errors.append("components.schemas must be an object")
    for value in _walk_refs(spec):
        if not isinstance(value, str) or not value.startswith("#/components/schemas/"):
            errors.append(f"unsupported/non-local schema reference: {value!r}")
        elif value[len("#/components/schemas/"):] not in schemas:
            errors.append(f"missing referenced schema: {value}")

    tag_names = {x.get("name") for x in spec.get("tags", []) if isinstance(x, dict)}
    operation_ids: set[str] = set()
    # Registration plus cleanup are expected: cleanup alone is NOT a live route.
    if device_source.count('apiRouter.setDeviceRecordingPlaybackHandler(') != 2:
        errors.append("Device Recording playback composition handler disappeared")
    for path, methods in {**AUDITED, **DEVICE_AUDITED}.items():
        data = paths.get(path)
        if not isinstance(data, dict):
            errors.append(f"{path}: path object absent")
            continue
        current = {key for key in data if key.lower() in METHODS}
        expected = set(methods)
        if current != expected:
            errors.append(f"{path}: documented methods changed, expected {sorted(expected)}, got {sorted(current)}")
        for method, marker in methods.items():
            where = f"{method.upper()} {path}"
            if path in DEVICE_AUDITED:
                device_marker, gate_marker = marker
                if device_marker not in device_source:
                    errors.append(f"{where}: Device source route handler anchor disappeared: {device_marker}")
                if gate_marker not in security_source:
                    errors.append(f"{where}: Device SecurityHttpGate admission marker disappeared: {gate_marker}")
            elif section and marker not in section[method]:
                errors.append(f"{where}: source route handler anchor disappeared: {marker}")
            op = data.get(method)
            if not isinstance(op, dict):
                continue
            operation_id = op.get("operationId")
            if not isinstance(operation_id, str) or not operation_id:
                errors.append(f"{where}: operationId missing")
            elif operation_id in operation_ids:
                errors.append(f"{where}: duplicate operationId {operation_id}")
            else:
                operation_ids.add(operation_id)
            if not op.get("summary") or not op.get("description"):
                errors.append(f"{where}: summary/description missing")
            if not op.get("tags") or any(t not in tag_names for t in op.get("tags", [])):
                errors.append(f"{where}: tag missing/undefined")

            params = op.get("parameters", [])
            if not isinstance(params, list):
                errors.append(f"{where}: parameters must be an array")
                params = []
            path_params = [p.get("name") for p in params if isinstance(p, dict) and p.get("in") == "path"]
            placeholders = PARAM_PATTERN.findall(path)
            if sorted(path_params) != sorted(placeholders):
                errors.append(f"{where}: path variable mismatch {path_params} != {placeholders}")
            if any(not p.get("required") for p in params if isinstance(p, dict) and p.get("in") == "path"):
                errors.append(f"{where}: OpenAPI path parameters must be required")
            locations = [(p.get("in"), p.get("name")) for p in params if isinstance(p, dict)]
            if len(locations) != len(set(locations)):
                errors.append(f"{where}: duplicate parameters")
            if len(locations) != len(params):
                errors.append(f"{where}: invalid parameter object")

            ext = op.get("x-vdr-suite-authentication")
            key = (method, path)
            if key not in {("get", "/api/v1"), ("get", "/api/v1/capabilities")}:
                if not isinstance(ext, dict) or not ext.get("authorization"):
                    errors.append(f"{where}: documented authorization boundary missing")
                elif ext.get("actorRequired") is not (key not in NO_ACTOR):
                    errors.append(f"{where}: actorRequired does not match audited actor/token scope")
                if path in DEVICE_AUDITED and (
                    not isinstance(ext, dict) or
                    "Device" not in ext.get("authorization", "") or
                    (path != "/api/v1/recording-playback-sessions/capabilities" and
                     ("media.live.play" if "live-playback" in path else
                     "media.recording.play") not in ext.get("authorization", ""))
                ):
                    errors.append(f"{where}: Device playback ownership/play grant boundary missing")
            required_headers = [p.get("name") for p in params if isinstance(p, dict) and p.get("in") == "header" and p.get("required") is True]
            if ("If-Match" in required_headers) != (key in IF_MATCH):
                errors.append(f"{where}: strong If-Match requirement drifted")
            if ("Idempotency-Key" in required_headers) != (key in IDEMPOTENCY):
                errors.append(f"{where}: Idempotency-Key requirement drifted")

            responses = op.get("responses")
            if not isinstance(responses, dict) or not any(re.fullmatch(r"2\d\d", code) for code in responses):
                errors.append(f"{where}: missing success response")
            elif any(not isinstance(v, dict) or not v.get("description") for v in responses.values()):
                errors.append(f"{where}: response missing description")
            if method == "post":
                should_have_body = path != "/api/v1/device-pairings/{pairingRequestId}/credential"
                body = op.get("requestBody")
                if should_have_body and (not isinstance(body, dict) or body.get("required") is not True):
                    errors.append(f"{where}: required POST body missing")
                if not should_have_body and body is not None:
                    errors.append(f"{where}: one-time issuance must have no request body")
    return errors


def main() -> int:
    try:
        spec = json.loads(OPENAPI.read_text(encoding="utf-8"))
        runtime = RUNTIME.read_text(encoding="utf-8")
        device_runtime = DEVICE_RUNTIME.read_text(encoding="utf-8")
        device_gate = DEVICE_GATE.read_text(encoding="utf-8")
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        print(f"OPENAPI_DRIFT=FAIL: cannot read source/spec: {exc}", file=sys.stderr)
        return 2
    errors = validate(spec, runtime, device_runtime, device_gate)
    if errors:
        print("OPENAPI_DRIFT=FAIL")
        for message in errors:
            print(f"- {message}")
        return 1
    paths = {**AUDITED, **DEVICE_AUDITED}
    print(f"OPENAPI_DRIFT=PASS paths={len(paths)} methods={sum(len(v) for v in paths.values())} scope=audited-partial")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
