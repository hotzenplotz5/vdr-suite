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
RECORDING_ROUTER = ROOT / "api/rest/src/ApiRouter.cpp"
DEVICE_RUNTIME = ROOT / "core/daemon/src/RecordingMediaHttpRuntime.cpp"
DEVICE_GATE = ROOT / "core/security/include/SecurityHttpGate.h"

# Each anchor is an actual branch in the corresponding C++ GET/POST dispatcher.
# Method-specific bounds prevent mistaking a POST 405 fallback for a real route.
# This explicit inventory must be reviewed when already-audited routes move.
MEDIA_TRANSPORT_AUDITED = {
    "/api/v1/media/sessions/{sessionId}/hls/{artifact}": {"get": "HlsMarker"},
    "/api/v1/media/sessions/{sessionId}/live/stream.mp4": {"get": "LiveSuffix"},
    "/api/v1/media/sessions/{sessionId}/recording/stream.mp4": {"get": "RecordingStreamSuffix"},
    "/api/v1/media/sessions/{sessionId}/recording/stream.ts": {"get": "RecordingDirectSuffix"},
}

MEDIA_PROCESSING_AUDITED = {
    "/api/v1/backends/{backendId}/capabilities/media-check": {
        "get": ("MediaCheck", "isPublicMediaCheckCapabilityResource", "/capabilities/media-check"),
    },
    "/api/v1/backends/{backendId}/media-checks": {
        "post": ("MediaCheck", "isPublicMediaCheckCreate", "/media-checks"),
    },
    "/api/v1/backends/{backendId}/media-checks/{checkId}": {
        "get": ("MediaCheck", "isPublicMediaCheckStatusResource", "/media-checks/"),
    },
    "/api/v1/backends/{backendId}/capabilities/media-shrink": {
        "get": ("MediaShrinkPlan", "isPublicMediaShrinkCapabilityResource", "/capabilities/media-shrink"),
    },
    "/api/v1/backends/{backendId}/media-shrink-plans": {
        "post": ("MediaShrinkPlan", "isPublicMediaShrinkPlanCreate", "/media-shrink-plans"),
    },
    "/api/v1/backends/{backendId}/media-shrink-stages": {
        "post": ("MediaShrinkPlan", "isPublicMediaShrinkStageCreate", "/media-shrink-stages"),
    },
    "/api/v1/backends/{backendId}/media-shrink-publications": {
        "post": ("MediaShrinkPlan", "isPublicMediaShrinkPublishCreate", "/media-shrink-publications"),
    },
    "/api/v1/backends/{backendId}/media-shrink-reviews/{reviewId}": {
        "get": ("MediaShrinkPlan", "isPublicMediaShrinkReviewResource", "/media-shrink-reviews/"),
    },
    "/api/v1/backends/{backendId}/media-shrink-operations/{operationId}": {
        "get": ("MediaShrinkPlan", "isPublicMediaShrinkOperationResource", "/media-shrink-operations/"),
    },
    "/api/v1/backends/{backendId}/media-shrink-recordings/{recordingId}": {
        "get": ("MediaShrinkPlan", "isPublicMediaShrinkRecordingResource", "/media-shrink-recordings/"),
    },
}

AUDITED = {
    "/api/v1/backends/{backendId}/capabilities/media-import": {
        "get": "if (publicMediaImportCapabilityPath(path, mediaCapabilityBackendId))",
    },
    "/api/v1/backends/{backendId}/media-import-sources": {
        "get": "if (publicMediaImportSourcesPath(path, sourceBackendId))",
    },
    "/api/v1/backends/{backendId}/media-import-sources/{sourceId}/preview": {
        "get": "if (publicMediaImportPreviewPath(path, previewBackendId, previewSourceId))",
    },
    "/api/v1/backends/{backendId}/media-imports": {
        "get": "if (publicMediaImportCollectionPath(path, mediaImportBackend))",
        "post": "if (publicMediaImportCollectionPath(path, mediaImportBackend))",
    },
    "/api/v1": {"get": 'if (path == "/api/v1")'},
    "/api/v1/capabilities": {"get": 'if (path == "/api/v1/capabilities")'},
    "/api/v1/backends": {"get": "if (path == PublicBackendCollectionPath)"},
    # Existing exact TMDb resolver is a read-only POST in PublicApiRuntime.
    # Its OpenAPI route was added without registering this audited anchor.
    "/api/v1/backends/{backendId}/metadata/resolve": {
        "post": "if (publicMetadataResolvePath(path, metadataBackend))",
    },
    "/api/v1/channels": {"get": "if (path == PublicChannelCollectionPath)"},
    "/api/v1/recordings": {"get": "if (path == PublicRecordingCollectionPath)"},
    "/api/v1/recordings/{recordingId}": {"get": "if (isRecordingDetailPath)"},
    "/api/v1/recordings/{recordingId}/artwork": {"get": "if (isRecordingArtworkPath)"},
    "/api/v1/recordings/{recordingId}/artwork/{artworkId}": {"get": "if (isRecordingArtworkPath)"},
    "/api/v1/search/recordings": {"get": "if (path == PublicSearchRecordingPath || path == PublicSearchEpgPath)"},
    "/api/v1/search/epg": {"get": "if (path == PublicSearchRecordingPath || path == PublicSearchEpgPath)"},
    "/api/v1/genres": {"get": "if (path == PublicGenreCollectionPath || path == PublicGenreRecordingPath)"},
    "/api/v1/genres/recordings": {"get": "if (path == PublicGenreCollectionPath || path == PublicGenreRecordingPath)"},
    "/api/v1/epg/now-next": {"get": "if (path == PublicEpgNowNextPath)"},
    "/api/v1/epg/window": {"get": "if (path == PublicEpgWindowPath)"},
    "/api/v1/epg/event": {"get": "if (path == PublicEpgEventDetailPath)"},
    "/api/v1/genres/epg": {"get": "if (path == PublicEpgGenrePath ||"},
    "/api/v1/genres/epg/events": {"get": "if (path == PublicEpgGenrePath ||"},
    "/api/v1/native-timers": {"get": "if (path == PublicNativeTimersPath)"},
    "/api/v1/native-timers/actions/create": {"post": "if (path == PublicNativeTimerDirectCreatePath)"},
    "/api/v1/native-timers/conflicts/live": {"get": "if (path == PublicNativeTimerConflictsPath ||"},
    "/api/v1/native-timers/live/list": {"get": "path == PublicNativeTimerLiveListPath)"},
    "/api/v1/native-timers/{timerId}": {
        "get": "if (publicNativeTimerItemPath(path, nativeTimerId) ||",
        "delete": "if (!publicNativeTimerItemPath(path, timerId)) return false;",
    },
    "/api/v1/native-timers/{timerId}/aux": {
        "get": "publicNativeTimerAuxPath(path, nativeTimerId))",
    },
    "/api/v1/native-timers/{timerId}/direct-update": {
        "post": "if (publicNativeTimerDirectActionPath(path, directTimerId, directAction))",
    },
    "/api/v1/native-timers/{timerId}/direct-delete": {
        "post": "if (publicNativeTimerDirectActionPath(path, directTimerId, directAction))",
    },
    "/api/v1/native-timers/{timerId}/modifications": {
        "post": "if (publicNativeTimerModificationPath(path, modifiedTimerId))",
    },
    "/api/v1/epg/timer-intents": {"post": "if (path == PublicEpgTimerIntentPath)"},
    "/api/v1/manual/timer-intents": {"post": "if (path == PublicManualTimerIntentPath)"},
    "/api/v1/timer-assignments": {"get": "if (path == PublicTimerAssignmentCollectionPath)"},
    "/api/v1/search-timers": {"get": "if (path == PublicSearchTimerCollectionPath)", "post": "if (path == PublicSearchTimerCollectionPath)"},
    "/api/v1/search-timers/{searchTimerId}": {"get": "if (publicSearchTimerDetailPath(path, savedSearchTimerId))"},
    "/api/v1/search-timers/draft-preview": {"get": "if (path == PublicSearchTimerDraftPreviewPath)"},
    "/api/v1/search-timers/{searchTimerId}/preview": {"get": "if (publicSearchTimerPreviewPath(path, previewSearchTimerId))"},
    "/api/v1/search-timers/{searchTimerId}/version": {"get": "if (publicSearchTimerNativeSubresourcePath("},
    "/api/v1/search-timers/{searchTimerId}/mutations": {"post": "if (publicSearchTimerNativeSubresourcePath("},
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
    "/api/v1/devices": {
        "get": 'if (path == "/api/v1/devices")',
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

# M2 EPG routes live in the already authenticated HTTP server;
# audit the real image dispatcher and SecurityHttpGate, not a fake API marker.
EPG_ARTWORK_AUDITED = {
    "/api/v1/epg/artwork": {
        "get": 'pathWithoutQuery(request.path) == "/api/v1/epg/artwork"'},
    "/api/v1/epg/preferred-artwork": {
        "get": 'pathWithoutQuery(request.path) == "/api/v1/epg/preferred-artwork"'},
}
SSE_AUDITED = {
    "/api/v1/events": {
        "get": ('pathWithoutQuery(request.path) == "/api/v1/events"',
                "if (isPublicDeviceEventRead)", "deviceEventHub_->subscribe("),
    },
    "/api/v1/operations/{operationId}/events": {
        "get": ('const std::string eventSuffix = "/events";',
                "if (isPublicOperationRead || isPublicOperationEventRead)",
                "deviceEventHub_->subscribeOperation("),
    },
}
EPG_ARTWORK_HTTP = ROOT / "core/http/src/TestHttpServer.cpp"
DEVICE_PLAYBACK_ACCOUNT_ADMIN_AUDITED = {
    "/api/v1/devices/{deviceId}/playback-account": {
        "get": 'devicePlaybackAccountPath(pathWithoutQuery(request.path)',
        "post": 'devicePlaybackAccountPath(pathWithoutQuery(request.path)',
    },
}

# Versioned Recording playback is registered in the daemon media composition
# root, rather than PublicApiRuntime. Require an actual route branch AND its
# SecurityHttpGate Device-only admission marker for every documented method.
# The GET status and track branches use an empty body to distinguish reads.
# ApiRouter owns these Public-v1 action handlers, not PublicApiRuntime.
RECORDING_ACTION_AUDITED = {
    "/api/v1/recordings/actions/validate": {
        "post": 'if (path == "/api/v1/recordings/actions/validate")',
    },
    "/api/v1/recordings/actions/execute": {
        "post": 'if (path == "/api/v1/recordings/actions/execute")',
    },
}

# Existing manual metadata service, shared by private and public Browser
# callers. Audit its actual router owner and scoped permission dispatch.
RECORDING_METADATA_RUNTIME = ROOT / "api/rest/src/ManualRecordingMetadataApiRuntime.cpp"
RECORDING_METADATA_AUDITED = {
    "/api/v1/backends/{backendId}/recordings/metadata/" + operation: {
        "post": 'route.operation == "' + operation + '"'
    }
    for operation in ("search", "seasons", "episodes", "assign", "withdraw")
}
RECORDING_METADATA_AUDITED["/api/v1/backends/{backendId}/recordings/metadata/trailer"] = {
    "get": 'const auto identity = trailerLookup(route.backendId, native);',
}
RECORDING_METADATA_AUDITED["/api/v1/backends/{backendId}/recordings/metadata/genre"] = {
    "get": 'metadata->getManualRecordingGenre(route.backendId, native, publicId)',
    "post": 'route.operation == "genre"',
}

# F6 Public-v1 operations reuse the existing native RecordingMarks/Cut
# owners. Audit both dispatch reachability and URL/body identity checks.
RECORDING_NATIVE_EDITING_AUDITED = {
    "/api/v1/recordings/{recordingId}/marks": {
        "get": ("RecordingMarksApiRuntime::instance().tryHandleGet(",
                "isPublicRecordingMarks", "RecordingMarksApiRuntime.cpp"),
        "post": ("RecordingMarksApiRuntime::instance().tryHandlePost(",
                 "isRecordingMarksModifyAction", "RecordingMarksApiRuntime.cpp"),
    },
    "/api/v1/recordings/{recordingId}/cut": {
        "get": ("RecordingCutApiRuntime::instance().tryHandleGet(",
                "isPublicRecordingCut", "RecordingCutApiRuntime.cpp"),
        "post": ("RecordingCutApiRuntime::instance().tryHandlePost(",
                 "isRecordingCutAction", "RecordingCutApiRuntime.cpp"),
    },
}

RECORDING_NATIVE_ROUTER = ROOT / "api/rest/include/ApiRouter.h"
RECORDING_NATIVE_MARKS = ROOT / "api/rest/src/RecordingMarksApiRuntime.cpp"
RECORDING_NATIVE_CUT = ROOT / "api/rest/src/RecordingCutApiRuntime.cpp"

DEVICE_AUDITED = {
    "/api/v1/recording-playback-sessions/recently-watched": {
        "get": ('const std::string historyPath = collection + "/recently-watched";',
                'isPublicDeviceRecordingSessionRead'),
    },
    "/api/v1/recording-playback-sessions/continue-watching": {
        "get": ('const std::string continuePath = collection + "/continue-watching";',
                'isPublicDeviceRecordingSessionRead'),
    },
    "/api/v1/recording-playback-sessions/progress": {
        "get": ('const std::string progressPath = collection + "/progress";',
                'isPublicDeviceRecordingSessionRead'),
        "post": ('const std::string progressPath = collection + "/progress";',
                 'isPublicDeviceRecordingProgressUpdate'),
    },
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
    ("post", "/api/v1/search-timers/{searchTimerId}/mutations"),
    ("post", "/api/v1/native-timers/{timerId}/modifications"),
    ("delete", "/api/v1/native-timers/{timerId}"),
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
    ("post", "/api/v1/backends/{backendId}/media-shrink-stages"),
    ("post", "/api/v1/backends/{backendId}/media-shrink-publications"),
    ("post", "/api/v1/backends/{backendId}/media-imports"),
    ("post", "/api/v1/native-timers/{timerId}/modifications"),
    ("delete", "/api/v1/native-timers/{timerId}"),
    ("post", "/api/v1/epg/timer-intents"),
    ("post", "/api/v1/manual/timer-intents"),
    ("post", "/api/v1/search-timers"),
    ("post", "/api/v1/search-timers/{searchTimerId}/mutations"),
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


# These names and this order are the actual private POST domain dispatch chain.
# Do not accept an anchor in an uncalled helper or an unrelated method body.
POST_DOMAIN_HANDLERS = (
    "NativeTimerMutation",
    "DevicePairing",
    "AccountCredentialSession",
    "DeviceAdministration",
    "AccountAdministration",
    "TimerWorkflow",
)


def _handler_sections(source: str) -> dict[str, str]:
    get_start = source.find("bool PublicApiRuntime::tryHandleGet(")
    helper_starts = [
        source.find(f"bool PublicApiRuntime::tryHandle{name}Post(")
        for name in POST_DOMAIN_HANDLERS
    ]
    post_start = source.find("bool PublicApiRuntime::tryHandlePost(")
    delete_start = source.find("bool PublicApiRuntime::tryHandleDelete(")
    unsupported_start = source.find(
        "bool PublicApiRuntime::tryHandleUnsupportedMethod("
    )
    bounds = [get_start, *helper_starts, post_start,
              delete_start, unsupported_start]
    if any(i < 0 for i in bounds) or bounds != sorted(set(bounds)):
        return {}

    # Verify that every helper is reached from the public dispatcher in the
    # exact original first-match order. Merely finding source text does not
    # prove a registered route still exists.
    dispatch = source[post_start:delete_start]
    expected_calls = [
        f"if (tryHandle{name}Post(" for name in POST_DOMAIN_HANDLERS
    ]
    if dispatch.count("if (tryHandle") != len(expected_calls):
        return {}
    positions = [dispatch.find(call) for call in expected_calls]
    if any(i < 0 for i in positions) or positions != sorted(positions):
        return {}
    # Each private handler must return its match result directly to the
    # public dispatcher, never continue to a later mutation owner.
    for offset, position in enumerate(positions):
        following = dispatch[position:]
        if not re.match(
            r"if \(tryHandle\w+Post\(.*?\)\)\s*return true;",
            following, re.DOTALL
        ):
            return {}
        next_position = (positions[offset + 1] if
                         offset + 1 < len(positions) else len(dispatch))
        if "return true;" not in dispatch[position:next_position]:
            return {}

    return {
        "get": " ".join(source[get_start:helper_starts[0]].split()),
        "post": " ".join(source[helper_starts[0]:delete_start].split()),
        "delete": " ".join(source[delete_start:unsupported_start].split()),
    }


def validate(
    spec: Any, source: str,
    device_source: str | None = None,
    security_source: str | None = None,
    recording_router_source: str | None = None,
    epg_artwork_http_source: str | None = None,
    native_router_source: str | None = None,
    native_marks_source: str | None = None,
    native_cut_source: str | None = None,
    recording_metadata_source: str | None = None,
) -> list[str]:
    """Return all detected contract drift without mutating a file or system."""
    errors: list[str] = []
    if device_source is None:
        device_source = DEVICE_RUNTIME.read_text(encoding="utf-8")
    if security_source is None:
        security_source = DEVICE_GATE.read_text(encoding="utf-8")
    if recording_router_source is None:
        recording_router_source = RECORDING_ROUTER.read_text(encoding="utf-8")
    if epg_artwork_http_source is None:
        epg_artwork_http_source = EPG_ARTWORK_HTTP.read_text(encoding="utf-8")
    if native_router_source is None:
        native_router_source = RECORDING_NATIVE_ROUTER.read_text(encoding="utf-8")
    if native_marks_source is None:
        native_marks_source = RECORDING_NATIVE_MARKS.read_text(encoding="utf-8")
    if native_cut_source is None:
        native_cut_source = RECORDING_NATIVE_CUT.read_text(encoding="utf-8")
    if recording_metadata_source is None:
        recording_metadata_source = RECORDING_METADATA_RUNTIME.read_text(encoding="utf-8")
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
    expected_paths = set(AUDITED) | set(DEVICE_AUDITED) | set(RECORDING_ACTION_AUDITED) | set(RECORDING_NATIVE_EDITING_AUDITED) | set(EPG_ARTWORK_AUDITED) | set(SSE_AUDITED) | set(DEVICE_PLAYBACK_ACCOUNT_ADMIN_AUDITED) | set(RECORDING_METADATA_AUDITED) | set(MEDIA_PROCESSING_AUDITED) | set(MEDIA_TRANSPORT_AUDITED)
    if set(paths) != expected_paths:
        missing = sorted(expected_paths - set(paths))
        extra = sorted(set(paths) - expected_paths)
        errors.append(f"audited path inventory changed: missing={missing}, extra={extra}; audit new routes before expanding")

    section = _handler_sections(source)
    if not section:
        errors.append("cannot isolate actual GET and POST C++ handler bodies")
    # ADR-0068: the public dispatcher must *reach* the dedicated read owner,
    # and that owner must still contain the scoped SearchTimer detail branch.
    owner_start = source.find(
        "bool PublicApiRuntime::tryHandleSearchTimerAssignmentReadGet("
    )
    dispatcher_start = source.find("bool PublicApiRuntime::tryHandleGet(")
    if (owner_start < 0 or dispatcher_start <= owner_start or
            'if (publicSearchTimerDetailPath(path, detailSearchTimerId))' not in
            source[owner_start:dispatcher_start]):
        errors.append("SearchTimer detail modular read owner missing")
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
    for path, methods in {
        **AUDITED, **DEVICE_AUDITED, **RECORDING_ACTION_AUDITED,
        **RECORDING_NATIVE_EDITING_AUDITED,
        **EPG_ARTWORK_AUDITED, **SSE_AUDITED, **DEVICE_PLAYBACK_ACCOUNT_ADMIN_AUDITED,
        **RECORDING_METADATA_AUDITED, **MEDIA_PROCESSING_AUDITED, **MEDIA_TRANSPORT_AUDITED
    }.items():
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
            if path in DEVICE_PLAYBACK_ACCOUNT_ADMIN_AUDITED:
                if marker not in epg_artwork_http_source:
                    errors.append(f"{where}: HTTP Device Playback account handler disappeared")
                needed = ("isPublicDevicePlaybackAccountMutation" if method == "post"
                          else "isPublicDevicePlaybackAccountRead")
                if needed not in security_source:
                    errors.append(f"{where}: administrator Browser security gate missing")
            elif path in SSE_AUDITED:
                route_marker, gate_marker, owner_marker = marker
                if (route_marker not in epg_artwork_http_source or
                    owner_marker not in epg_artwork_http_source or
                    gate_marker not in security_source):
                    errors.append(f"{where}: existing SSE route/owner/security anchor missing")
            elif path in EPG_ARTWORK_AUDITED:
                if marker not in epg_artwork_http_source:
                    errors.append(
                        f"{where}: EPG image HTTP route anchor disappeared: {marker}")
                if path not in security_source or '"epg.view"' not in security_source:
                    errors.append(
                        f"{where}: EPG image SecurityHttpGate epg.view admission disappeared")
            elif path in RECORDING_ACTION_AUDITED:
                if marker not in recording_router_source:
                    errors.append(
                        f"{where}: Recording ApiRouter route handler anchor disappeared: {marker}")
                if 'isRecordingExecutionAction' not in security_source or (
                    '/api/v1/recordings/actions/validate' not in security_source):
                    errors.append(
                        f"{where}: Recording SecurityHttpGate admission marker disappeared")
            elif path in MEDIA_TRANSPORT_AUDITED:
                gateway = (ROOT / "core/http/src/MediaGatewayHttpServer.cpp").read_text(encoding="utf-8")
                if (marker not in gateway or
                    '"/api/v1/media/sessions/"' not in gateway or
                    'authenticator_.authenticate(credential, mediaPath.sessionId)' not in gateway or
                    'routeLeaseRepository_.findActive(' not in gateway or
                    '!publicSessionAuthorizer_(' not in gateway):
                    errors.append(f"{where}: session-bound media gateway anchor missing")
                media_op = data.get(method, {})
                if media_op.get("security") != [{"mediaSessionCookie": []}, {"mediaAuthorization": []}]:
                    errors.append(f"{where}: session-bound media authentication drifted")
            elif path in MEDIA_PROCESSING_AUDITED:
                owner, gate_marker, suffix = marker
                verb = "Get" if method == "get" else "Post"
                router_marker = owner + "ApiRuntime::instance().tryHandle" + verb + "("
                public_dispatch = "PublicApiRuntime::instance().tryHandle" + verb + "("
                owner_source = (ROOT / ("api/rest/include/" + owner + "ApiRuntime.h")).read_text(encoding="utf-8")
                if (native_router_source.count(router_marker) != 1 or
                    public_dispatch not in native_router_source or
                    native_router_source.find(router_marker) > native_router_source.find(public_dispatch) or
                    gate_marker not in security_source or suffix not in owner_source):
                    errors.append(f"{where}: Media Tools route/owner/security anchor missing")
            elif path in RECORDING_METADATA_AUDITED:
                router_marker = "ManualRecordingMetadataApiRuntime::instance().tryHandle" + (
                    "Get(" if method == "get" else "Post(")
                if method == "get" and (
                    "isPublicManualMetadataRead" not in security_source or
                    '(manualMetadataOperation == "genre" || manualMetadataOperation == "trailer")' not in security_source or
                    'std::vector<std::string>{manualMetadataBackendId}' not in security_source or
                    'recordingReadRequest.permission = "recordings.view"' not in security_source or
                    native_router_source.find(router_marker) >
                    native_router_source.find("PublicApiRuntime::instance().tryHandleGet(")
                ):
                    errors.append(f"{where}: public metadata read scope or routing missing")
                if method == "get" and (
                    'authorizedBackendIds.front() != route.backendId' not in recording_metadata_source or
                    'const auto native = lookup(route.backendId, publicId);' not in recording_metadata_source or
                    'validPublicRecordingId(publicId)' not in recording_metadata_source
                ):
                    errors.append(f"{where}: public metadata GET identity binding missing")
                if native_router_source.count(router_marker) != 1:
                    errors.append(f"{where}: manual metadata router owner missing")
                if marker not in recording_metadata_source or (
                    'publicPrefix = "/api/v1/backends/"' not in recording_metadata_source or
                    'authorizedBackendId != route.backendId' not in recording_metadata_source or
                    'strings["resourceKey"] = native' not in recording_metadata_source):
                    errors.append(f"{where}: public metadata handler/identity binding missing")
                if ('publicPrefix = "/api/v1/backends/"' not in security_source or
                    'isManualRecordingMetadataAction' not in security_source or
                    'requestToAuthorize.permission = "metadata.recording.assign"' not in security_source):
                    errors.append(f"{where}: existing metadata assignment authorization missing")
            elif path in RECORDING_NATIVE_EDITING_AUDITED:
                router_marker, gate_marker, native_owner = marker
                native_source = (
                    native_marks_source if native_owner == "RecordingMarksApiRuntime.cpp"
                    else native_cut_source
                )
                if native_router_source.count(router_marker) != 1:
                    errors.append(f"{where}: expected exactly one native Editing router owner")
                if gate_marker not in security_source or (
                    "isPublicRecordingEditRead" not in security_source or
                    "isPublicRecordingRead" not in security_source):
                    errors.append(f"{where}: Recording SecurityHttpGate scope/permission admission missing")
                if 'PublicRecordingEditRoute::matchPath(' not in native_source or (
                    'request.request.recordingId != publicRecordingId' not in native_source or
                    'request.request.backendId != backendId' not in native_source):
                    errors.append(f"{where}: public Recording path/body identity check absent")
                public_dispatch = "PublicApiRuntime::instance().tryHandle" + (
                    "Get(" if method == "get" else "Post("
                )
                if public_dispatch not in native_router_source or (
                    native_router_source.index(router_marker) >
                    native_router_source.index(public_dispatch)):
                    errors.append(f"{where}: native Recording owner must precede v1 fallback")
            elif path in DEVICE_AUDITED:
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
        recording_router = RECORDING_ROUTER.read_text(encoding="utf-8")
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        print(f"OPENAPI_DRIFT=FAIL: cannot read source/spec: {exc}", file=sys.stderr)
        return 2
    errors = validate(
        spec, runtime, device_runtime, device_gate, recording_router)
    if errors:
        print("OPENAPI_DRIFT=FAIL")
        for message in errors:
            print(f"- {message}")
        return 1
    paths = {**AUDITED, **DEVICE_AUDITED, **RECORDING_ACTION_AUDITED,
             **RECORDING_NATIVE_EDITING_AUDITED, **EPG_ARTWORK_AUDITED, **SSE_AUDITED,
             **DEVICE_PLAYBACK_ACCOUNT_ADMIN_AUDITED, **RECORDING_METADATA_AUDITED, **MEDIA_PROCESSING_AUDITED, **MEDIA_TRANSPORT_AUDITED}
    print(f"OPENAPI_DRIFT=PASS paths={len(paths)} methods={sum(len(v) for v in paths.values())} scope=audited-partial")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
