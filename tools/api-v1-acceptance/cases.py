"""Contract-aware and bounded read-only Public-v1 flows.

Every live request uses HttpClient.get only. No legacy /api/ probes, no
validate/dry-run POST, no MediaSession creation and no retry on timeout.
"""
import json
from pathlib import Path
import re
import subprocess
import sys
import time
from urllib.parse import urlencode, urlsplit, parse_qsl

from runner import ROOT, BACKEND, SHA, Problem, Blocked, Skip, validate

V1 = "/api/v1"
RECORDING_ID = re.compile(r"^rec_[0-9a-f]{32}$")
SEARCH_ID = re.compile(r"^st_[0-9a-f]{32}$")
ARTWORK_ID = re.compile(r"^[0-9a-f]{32}$")
REQUIRED_ROUTES = {
    "/api/v1", "/api/v1/capabilities", "/api/v1/backends",
    "/api/v1/channels", "/api/v1/epg/now-next", "/api/v1/epg/window",
    "/api/v1/timer-assignments", "/api/v1/native-timers",
    "/api/v1/operations/{operationId}", "/api/v1/epg/timer-intents",
    "/api/v1/recordings", "/api/v1/recordings/{recordingId}",
    "/api/v1/recordings/{recordingId}/artwork",
    "/api/v1/recordings/{recordingId}/artwork/{artworkId}",
    "/api/v1/search-timers", "/api/v1/search-timers/{searchTimerId}",
    "/api/v1/search-timers/{searchTimerId}/preview",
    "/api/v1/search-timers/{searchTimerId}/version",
    "/api/v1/search-timers/draft-preview",
    "/api/v1/genres", "/api/v1/search/recordings", "/api/v1/search/epg",
    "/api/v1/recording-playback-sessions/capabilities",
    "/api/v1/recording-playback-sessions/continue-watching",
    "/api/v1/recording-playback-sessions/progress",
    "/api/v1/recording-playback-sessions",
    "/api/v1/recording-playback-sessions/{sessionId}",
}
WRITABLE_ROUTES = (
    "/api/v1/recordings/actions/execute",
    "/api/v1/epg/timer-intents",
    "/api/v1/native-timers/{timerId}/modifications",
    "/api/v1/search-timers/{searchTimerId}/mutations",
    "/api/v1/recording-playback-sessions/progress",
)


def endpoint(route, **params):
    return route + ("?" + urlencode(params) if params else "")


def collection(data, path, limit):
    if not isinstance(data.get("items"), list):
        raise Problem("wrong_json_structure", "items must be an array",
                      path=path, shape="items: array")
    if len(data["items"]) > limit:
        raise Problem("pagination_overflow", "response exceeds requested page limit",
                      path=path, shape="max " + str(limit))
    return data["items"]


def no_private_paths(obj):
    """Read-only leak predicate, never persist raw values to the result files."""
    if isinstance(obj, dict):
        return all(no_private_paths(v) for v in obj.values())
    if isinstance(obj, list):
        return all(no_private_paths(v) for v in obj)
    if isinstance(obj, str):
        lower = obj.lower()
        return not (any(x in lower for x in
                        ("/var/lib/vdr", "/video/", "/home/", "credentialsecret",
                         "authorization:", "bearer ", "cookie:"))
                    or re.search(r"/[^ ]+\.rec(?:/|$)", obj) is not None)
    return True


def get(a, route, **params):
    return a.get_json(endpoint(route, **params))[0]


def require_device(a):
    if "device" not in a.cfg:
        raise Blocked("paired Device credential missing; private 0600 config required")


def check_spec(a):
    path = ROOT / "api/openapi.json"
    if not path.is_file():
        raise Blocked("api/openapi.json unavailable at checkout")
    a.spec = json.loads(path.read_text(encoding="utf-8"))
    a.context["spec"] = True
    a.require(a.spec.get("openapi", "").startswith("3."), "OpenAPI 3 contract required")
    paths = a.spec.get("paths", {})
    missing = REQUIRED_ROUTES - paths.keys()
    a.require(not missing, "source lacks Public-v1 route contracts: " +
              ",".join(sorted(missing)))
    for route in REQUIRED_ROUTES:
        a.require("get" in paths[route] or
                  (route in ("/api/v1/epg/timer-intents", "/api/v1/recording-playback-sessions")
                   and "post" in paths[route]),
                  "missing documented expected HTTP method", shape=route)
    a.require("post" in paths["/api/v1/epg/timer-intents"],
              "TimerIntent must remain POST-only")
    a.require("get" not in paths["/api/v1/epg/timer-intents"],
              "no invented TimerIntent collection GET")
    a.require(all(route in paths for route in WRITABLE_ROUTES),
              "controlled write target missing from OpenAPI")
    for route in WRITABLE_ROUTES:
        a.require("post" in paths[route],
                  "mutation must be an explicitly documented POST", shape=route)
    a.require((ROOT / "docs/development/public-v1-final-hardware-acceptance.md").is_file(),
              "central final hardware acceptance checklist missing")


def check_source(a):
    if not SHA.fullmatch(a.repo_sha):
        raise Blocked("source SHA cannot be established with git rev-parse HEAD")
    # Any runner branch or detached checkout is valid. Target SHA is independent.


def offline_unit(a):
    file = ROOT / "tests/test_api_v1_acceptance_runner.py"
    if not file.is_file():
        raise Blocked("offline runner regression tests missing")
    try:
        r = subprocess.run([sys.executable, "-m", "unittest", "discover", "-s",
                            "tests", "-p", "test_api_v1_acceptance_runner.py"],
                           cwd=str(ROOT), stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE, timeout=35, check=False)
    except (OSError, subprocess.SubprocessError):
        raise Problem("selftest_error", "runner regression process failed to start")
    a.require(r.returncode == 0, "offline regression tests returned nonzero exit")


def source_contract_tests(a):
    """Run already-established source HTTP/contract tests only when explicitly configured.

    No legacy acceptance result is imported here, and no C++ rebuild is
    performed by a Python safe-acceptance invocation.
    """
    paths = a.spec.get("paths", {})
    a.require(len(paths) >= 40, "contract source unexpectedly incomplete")
    a.require("/api/v1/search-timers/{searchTimerId}/version" in paths,
              "S1 revision contract missing")
    a.require("/api/v1/recording-playback-sessions/progress" in paths,
              "P1 resume contract missing")
    # Execute the existing source-vs-runtime-route auditor, not a new competing schema.
    audit = ROOT / "tools/check_public_v1_openapi.py"
    if not audit.is_file():
        raise Blocked("existing Public-v1 OpenAPI drift checker missing")
    try:
        r = subprocess.run([sys.executable, str(audit)], cwd=str(ROOT),
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           check=False, timeout=25)
    except (OSError, subprocess.SubprocessError):
        raise Problem("contract_audit_failed", "OpenAPI drift audit could not run")
    a.require(r.returncode == 0, "authoritative OpenAPI drift auditor failed")


def controlled_contracts_offline(a):
    """Verify future controlled scenarios against actual documented DTOs.

    Local JSON fixtures only. This never constructs or sends an HTTP write.
    """
    spec = a.spec
    if spec is None:
        raise Blocked("OpenAPI contract not yet loaded")
    refs = spec["components"]["schemas"]
    plan = [
        ("/api/v1/recordings/actions/execute", "post",
         {"backendId": "fixture", "recordingId": "rec_" + "a" * 32,
          "action": "RENAME", "newName": "ACCEPTANCE-FIXTURE", "dryRun": False},
         ()),
        ("/api/v1/epg/timer-intents", "post",
         {"backendId": "fixture", "channelId": "C-1-1-1",
          "eventId": "12345"},
         ("If-Match", "Idempotency-Key")),
        ("/api/v1/native-timers/{timerId}/modifications", "post",
         {"kind": "toggle", "enabled": False},
         ("If-Match", "Idempotency-Key")),
        ("/api/v1/native-timers/{timerId}", "delete", None,
         ("If-Match", "Idempotency-Key")),
        ("/api/v1/search-timers", "post",
         {"name": "ACCEPTANCE-FIXTURE", "query": "Fixture",
          "active": False, "confirmed": True, "mode": 0,
          "compareTitle": True, "compareSubtitle": False,
          "compareSummary": False}, ("Idempotency-Key",)),
        ("/api/v1/search-timers/{searchTimerId}/mutations", "post",
         {"action": "update", "confirmed": True, "query": "Fixture"},
         ("Idempotency-Key", "If-Match")),
        ("/api/v1/recording-playback-sessions/progress", "post",
         {"backendId": "fixture", "recordingId": "rec_" + "a" * 32,
          "operationId": "fixture-operation", "positionSeconds": 1,
          "expectedRevision": 0, "sessionId": "ms_" + "b" * 32}, ()),
        ("/api/v1/recording-playback-sessions/{sessionId}/stop", "post",
         {"operation": "stop", "backendId": "fixture",
          "sessionId": "ms_" + "b" * 32}, ()),
        ("/api/v1/accounts/{accountId}/grants", "post",
         {"permission": "recordings.view", "backendId": "fixture", "active": False},
         ("If-Match",)),
    ]
    for route, method, body, required_headers in plan:
        op = spec.get("paths", {}).get(route, {}).get(method)
        a.require(isinstance(op, dict),
                  "controlled scenario method absent in documented OpenAPI", shape=route)
        names = {p.get("name") for p in op.get("parameters", [])
                 if isinstance(p, dict) and p.get("in") == "header" and p.get("required")}
        a.require(set(required_headers).issubset(names),
                  "revision/idempotency contract missing for controlled scenario",
                  shape=route + " required headers")
        if body is not None:
            body_schema = op.get("requestBody", {}).get("content", {}).get(
                "application/json", {}).get("schema")
            a.require(isinstance(body_schema, dict) and
                      validate(body, body_schema, refs),
                      "local controlled DTO fixture does not match documented schema",
                      shape=route + " request body")
    search_create = spec["paths"]["/api/v1/search-timers"]["post"]
    shape = search_create["requestBody"]["content"]["application/json"]["schema"]
    bad = dict(plan[4][2], active=True)
    a.require(not validate(bad, shape, refs),
              "unsafe active SearchTimer creation fixture was accepted")
    return "9-controlled-contract-scenarios-validated-offline"


def root(a):
    d = get(a, V1)
    a.require(d.get("apiVersion") == "v1", "incorrect API major",
              path=V1, shape="apiVersion=v1")
    reported = d.get("serverVersion", "unavailable")
    a.runtime_version = (reported if isinstance(reported, str) and
                         re.fullmatch(r"[A-Za-z0-9_.+:-]{1,96}", reported)
                         else "invalid/unverifiable")
    a.context["root"] = d
    if "device" in a.cfg:
        a.require(d.get("authentication", {}).get("authenticated") is True,
                  "paired Device did not authenticate at public root", path=V1)


def runtime_match(a):
    # Never compare the daemon to the test runner's repository HEAD.
    expected = a.expected_daemon_commit
    if not expected:
        a.evidence = "NOT_VERIFIED (daemon target not configured)"
        raise Blocked("expected daemon target commit missing; set --expected-daemon-commit")
    d = a.context.get("root")
    version = d.get("serverVersion") if isinstance(d, dict) else None
    if not isinstance(version, str):
        a.evidence = "NOT_VERIFIED (runtime build identity absent)"
        raise Blocked("runtime did not expose a Git build identity")
    if not re.fullmatch(r"git-[0-9a-f]{7,40}", version):
        a.evidence = "NOT_VERIFIED (non-Git runtime identity)"
        raise Blocked("daemon build identity is not a verifiable Git SHA")
    gitpart = version[4:]
    if not expected.startswith(gitpart):
        a.evidence = "MISMATCH (expected git-" + expected[:12] + ", reported " + version + ")"
        raise Problem("daemon_version_mismatch",
                      "expected git-" + expected[:12] + " but daemon reports " + version,
                      path=V1, shape="daemon build matches explicitly selected target")
    a.evidence = "MATCHED_DAEMON_TARGET_GIT_BUILD_ID (" + gitpart + ")"
    a.context["runtime_verified"] = True


def discovery(a):
    d = get(a, V1 + "/capabilities")
    a.require(d.get("apiVersion") == "v1", "capability API major mismatch",
              path=V1 + "/capabilities")
    a.require(isinstance(d.get("capabilities"), list),
              "capabilities must be array", path=V1 + "/capabilities")


def missing_auth(a):
    status, _, _ = a.client.get(V1 + "/backends", auth=False)
    a.require(status == 401, "missing authentication not rejected (expected 401)",
              path=V1 + "/backends", expected="401", actual=status)


def invalid_auth(a):
    status, _, _ = a.client.get(V1 + "/backends", invalid=True)
    a.require(status == 401, "invalid Device credential not rejected (expected 401)",
              path=V1 + "/backends", expected="401", actual=status)


def backends(a):
    require_device(a)
    d = get(a, V1 + "/backends", limit=10)
    items = collection(d, V1 + "/backends", 10)
    matches = [r for r in items if isinstance(r, dict) and r.get("backendId") == a.backend]
    if len(matches) != 1:
        raise Blocked("configured backend not in bounded authorized backend page")
    a.context["backend"] = a.backend


def isolation(a):
    require_device(a)
    denied = a.cfg.get("deniedBackend")
    if not denied or not BACKEND.fullmatch(str(denied)) or denied == a.backend:
        raise Blocked("separate known unauthorized backend ID not configured")
    path = endpoint(V1 + "/channels", backendId=denied, limit=1)
    status, _, _ = a.client.get(path)
    a.require(status in (403, 404), "backend isolation failed",
              path=path, expected="403/404", actual=status)


def channels(a):
    d = get(a, V1 + "/channels", backendId=a.backend, limit=5)
    items = collection(d, V1 + "/channels", 5)
    a.require(no_private_paths(d), "internal data leaked in channel response")
    for item in items:
        a.require(isinstance(item, dict) and item.get("backendId") == a.backend
                  and isinstance(item.get("channelId"), str),
                  "invalid channel identity")
    if not items:
        raise Blocked("no channels in authorized backend; EPG chain cannot be checked")
    a.context["channel"] = items[0]["channelId"]


def now_next(a):
    t = str(int(time.time()))
    d = get(a, V1 + "/epg/now-next", backendId=a.backend,
            channelId=a.context["channel"], fromTime=t, limit=2)
    items = collection(d, V1 + "/epg/now-next", 2)
    a.require(d.get("channelId") == a.context["channel"], "EPG channel mismatch")
    if items:
        a.context["event"] = items[0]


def epg_window(a):
    t = int(time.time())
    path = endpoint(V1 + "/epg/window", backendId=a.backend,
                    channelId=a.context["channel"], fromTime=t,
                    untilTime=t + 7200, limit=3)
    d = a.get_json(path)[0]
    items = collection(d, path, 3)
    a.require(no_private_paths(d), "private path in EPG response", path=path)
    first_ids = {str(x["eventId"]) for x in items if isinstance(x, dict)
                 and x.get("eventId")}
    a.require(len(first_ids) == sum(bool(x.get("eventId")) for x in items),
              "duplicate authoritative EPG event IDs", path=path)
    page = d.get("page", {})
    cursor = page.get("nextCursor") if isinstance(page, dict) else None
    if cursor:
        p2 = path + "&" + urlencode({"cursor": cursor})
        nxt = a.get_json(p2)[0]
        nxt_items = collection(nxt, p2, 3)
        next_ids = {str(x["eventId"]) for x in nxt_items if isinstance(x, dict)
                    and x.get("eventId")}
        a.require(not first_ids.intersection(next_ids),
                  "EPG cursor repeated an authoritative event ID", path=p2)
    if not items:
        raise Blocked("EPG is empty in bounded selected window")


def timer_assignments(a):
    d = get(a, V1 + "/timer-assignments", backend=a.backend, limit=5)
    items = collection(d, V1 + "/timer-assignments", 5)
    if items:
        r = items[0]
        if not isinstance(r, dict) or not r.get("timerAssignmentId"):
            raise Problem("wrong_json_structure", "TimerAssignment public identity missing")
        a.context["assignment"] = r["timerAssignmentId"]


def timer_detail(a):
    if not a.context.get("assignment"):
        raise Blocked("no existing TimerAssignment for read-only detail verification")
    path = endpoint(V1 + "/timer-assignments/" + a.context["assignment"],
                    backend=a.backend)
    data, headers = a.get_json(path)
    a.require(data.get("backendId") == a.backend, "TimerAssignment backend mismatch")
    etag = headers.get("etag")
    a.require(etag and not etag.startswith("W/"), "strong TimerAssignment ETag missing",
              path=path, shape="strong ETag")


def native_timers(a):
    d = get(a, V1 + "/native-timers", backendId=a.backend, limit=5)
    a.require(isinstance(d, dict), "native timer response must be object")
    items = d.get("items")
    a.require(isinstance(items, list) and len(items) <= 5,
              "native timer items collection missing or unbounded",
              path=V1 + "/native-timers", shape="items: array (max 5)")
    a.require(no_private_paths(d), "native timer leaked private filesystem path")
    if items:
        first = items[0]
        if not isinstance(first, dict) or not re.fullmatch(r"[A-Za-z0-9._:-]{1,160}",
                                                        str(first.get("timerId", ""))):
            raise Problem("wrong_json_structure", "invalid public native timer identity")
        a.context["native_timer"] = first["timerId"]


def native_timer_detail(a):
    path = endpoint(V1 + "/native-timers/" + a.context["native_timer"],
                    backendId=a.backend)
    data, headers = a.get_json(path)
    a.require(data.get("backendId") == a.backend and
              data.get("timerId") == a.context["native_timer"],
              "native timer list/detail mismatch", path=path)
    a.require(no_private_paths(data), "native timer detail leaked a filesystem path")
    if data.get("bindingState") == "verified":
        etag = headers.get("etag")
        a.require(etag and etag.startswith('"') and etag.endswith('"'),
                  "bound native timer must expose authoritative strong ETag",
                  path=path, shape="strong ETag")
    return "native-timer-detail-authoritative"


def authorized_operation(a):
    op_id = a.cfg.get("existingOperationId")
    if not isinstance(op_id, str) or not re.fullmatch(r"op_[0-9a-f]{32}", op_id):
        raise Blocked("existing owned Public-v1 Operation ID not configured")
    path = V1 + "/operations/" + op_id
    data, headers = a.get_json(path)
    a.require(data.get("operationId") == op_id,
              "durable Operation identity mismatch", path=path)
    if data.get("backendId") != a.backend:
        raise Blocked("existing Operation belongs to another backend or actor", path=path)
    a.require(no_private_paths(data), "durable Operation leaked filesystem path")
    return "operation-state:" + str(data.get("state", "missing"))


def searchtimers(a):
    d = get(a, V1 + "/search-timers", backend=a.backend, limit=5)
    items = collection(d, V1 + "/search-timers", 5)
    a.require(no_private_paths(d), "private SearchTimer data leaked")
    if items:
        first = items[0]
        a.require(isinstance(first, dict) and SEARCH_ID.fullmatch(
            str(first.get("searchTimerId", ""))) is not None,
            "invalid opaque SearchTimer ID")
        a.context["searchtimer"] = first["searchTimerId"]


def search_detail(a):
    path = endpoint(V1 + "/search-timers/" + a.context["searchtimer"],
                    backend=a.backend)
    data, _ = a.get_json(path)
    a.require(data.get("searchTimerId") == a.context["searchtimer"],
              "SearchTimer detail public ID mismatch", path=path)


def search_version(a):
    path = endpoint(V1 + "/search-timers/" + a.context["searchtimer"] + "/version",
                    backend=a.backend)
    data, hdr = a.get_json(path)
    a.require(isinstance(data.get("revision"), str) and
              re.fullmatch(r"sr_[0-9a-f]{32}", data["revision"]),
              "SearchTimer revision contract invalid", path=path)
    a.require(hdr.get("etag") and not hdr["etag"].startswith("W/"),
              "strong SearchTimer revision ETag missing", path=path)


def search_preview(a):
    path = endpoint(V1 + "/search-timers/" + a.context["searchtimer"] + "/preview",
                    backend=a.backend, limit=3)
    data, _ = a.get_json(path)
    collection(data, path, 3)
    a.require(data.get("searchTimerId") == a.context["searchtimer"],
              "preview identity mismatch", path=path)


def draft_preview(a):
    # Explicit GET, no rule is created; query string never appears in logs.
    path = endpoint(V1 + "/search-timers/draft-preview", backend=a.backend,
                    query="VDRSuiteAcceptanceNoMatch", mode=0, compareTitle=1,
                    compareSubtitle=0, compareSummary=0, limit=2)
    data, _ = a.get_json(path)
    collection(data, path, 2)
    a.require(data.get("backendId") == a.backend, "draft preview backend mismatch")


def recordings(a):
    path = endpoint(V1 + "/recordings", backendId=a.backend, limit=5)
    data, _ = a.get_json(path)
    items = collection(data, path, 5)
    a.require(no_private_paths(data), "private recording filesystem path leaked")
    if not items:
        raise Blocked("no recordings available for a detail-to-artwork-to-resume chain")
    r = items[0]
    ident = r.get("recordingId") if isinstance(r, dict) else None
    a.require(isinstance(ident, str) and RECORDING_ID.fullmatch(ident),
              "recording public ID invalid")
    a.context["recording"] = ident


def folders(a):
    path = endpoint(V1 + "/recordings", backendId=a.backend, view="folders",
                    limit=5, offset=0)
    data, _ = a.get_json(path)
    collection(data, path, 5)
    a.require(no_private_paths(data), "private filesystem path in folder response")


def recording_detail(a):
    path = endpoint(V1 + "/recordings/" + a.context["recording"],
                    backendId=a.backend)
    data, _ = a.get_json(path)
    a.require(data.get("recordingId") == a.context["recording"],
              "recording list/detail identity mismatch", path=path)
    a.require(no_private_paths(data), "filesystem location leaked", path=path)
    a.context["recording_detail"] = data


def metadata(a):
    d = a.context["recording_detail"]
    a.require("metadata" in d and isinstance(d["metadata"], dict),
              "public Recording metadata object absent")
    m = d["metadata"]
    a.require(no_private_paths(m), "metadata contains private path")
    a.require(m.get("state") in ("absent", "partial", "available"),
              "metadata state must be explicit")
    a.require(m.get("contentKind") in ("unknown", "movie", "series", "series-episode"),
              "invalid contentKind")
    return "metadata-state:" + m["state"]


def rich_metadata(a):
    m = a.context["recording_detail"]["metadata"]
    if m.get("state") != "available":
        raise Blocked("selected recording lacks available enriched metadata")
    if m.get("contentKind") not in ("series", "series-episode"):
        raise Blocked("no approved series/episode metadata fixture selected")
    a.require(isinstance(m.get("seriesTitle"), str) and bool(m["seriesTitle"]),
              "series metadata must include seriesTitle")
    if m["contentKind"] == "series-episode":
        a.require(isinstance(m.get("episodeTitle"), str) and bool(m["episodeTitle"]),
                  "episode metadata must include episodeTitle")


def genre(a):
    path = endpoint(V1 + "/genres", backendId=a.backend, limit=5)
    data, _ = a.get_json(path)
    collection(data, path, 5)


def artwork_list(a):
    path = endpoint(V1 + "/recordings/" + a.context["recording"] + "/artwork",
                    backendId=a.backend)
    data, _ = a.get_json(path)
    a.require(data.get("recordingId") == a.context["recording"],
              "artwork owner mismatch", path=path)
    items = data.get("items")
    a.require(isinstance(items, list) and len(items) <= 64,
              "artwork items shape invalid", path=path)
    if not items:
        raise Blocked("selected recording has no artwork; binary delivery not verified")
    item = items[0]
    artid = item.get("artworkId") if isinstance(item, dict) else None
    uri = item.get("url") if isinstance(item, dict) else None
    a.require(isinstance(artid, str) and ARTWORK_ID.fullmatch(artid),
              "invalid artwork reference", path=path)
    expected = V1 + "/recordings/" + a.context["recording"] + "/artwork/" + artid
    a.require(isinstance(uri, str) and urlsplit(uri).path == expected and
              not urlsplit(uri).scheme and not urlsplit(uri).netloc and
              not any(k.lower() in ("token", "key", "secret", "credential", "auth")
                      for k in dict(parse_qsl(urlsplit(uri).query))),
              "artwork URL must be relative without credentials", path=path)
    a.context["artwork_url"] = endpoint(expected, backendId=a.backend)


def artwork_bytes(a):
    image, _ = a.client.expect(a.context["artwork_url"], binary=True)
    return image["image_type"] + " bytes:" + str(image["bytes_read"])


def playback_capabilities(a):
    require_device(a)
    profile = a.cfg.get("mediaProfile")
    if not isinstance(profile, dict) or any(k not in profile for k in
                                            ("appVersion", "firmwareVersion", "playerVersion", "engine")):
        raise Blocked("Device mediaProfile values not configured; no synthetic player versions")
    d = get(a, V1 + "/recording-playback-sessions/capabilities", **profile)
    a.require(d.get("profileState") in ("verified", "unverified"),
              "invalid media profile state")


def continue_watching(a):
    require_device(a)
    d = get(a, V1 + "/recording-playback-sessions/continue-watching",
            backendId=a.backend)
    items = d.get("items")
    a.require(isinstance(items, list) and len(items) <= 50,
              "Continue Watching bounded collection invalid")
    for x in items:
        a.require(RECORDING_ID.fullmatch(str(x.get("recordingId", ""))) is not None,
                  "Continue Watching exposes invalid Recording ID")


def resume_progress(a):
    require_device(a)
    path = endpoint(V1 + "/recording-playback-sessions/progress", backendId=a.backend,
                    recordingId=a.context["recording"])
    data, _ = a.get_json(path)
    a.require(data.get("recordingId") == a.context["recording"] and
              isinstance(data.get("hasProgress"), bool) and
              type(data.get("revision")) is int and data["revision"] >= 0,
              "P1 persistent progress shape or identity mismatch", path=path)


def resume_saved_position(a):
    require_device(a)
    path = endpoint(V1 + "/recording-playback-sessions/progress", backendId=a.backend,
                    recordingId=a.context["recording"])
    data, _ = a.get_json(path)
    if not data.get("hasProgress"):
        raise Blocked("selected Recording has no persisted Device playback position")
    a.require(type(data.get("positionSeconds")) is int and
              data["positionSeconds"] > 0,
              "stored position must be a positive integer", path=path)
    return "persisted-position-verified"


def search_recordings(a):
    path = endpoint(V1 + "/search/recordings", backendId=a.backend,
                    query="VDRSuiteNoMatch", limit=3, offset=0)
    data, _ = a.get_json(path)
    collection(data, path, 3)


def status_negative(a, path, allowed):
    require_device(a)
    status, _, _ = a.client.get(path)
    if status in (401, 403, 503):
        kind = ("authentication_failed" if status == 401 else
                "permission_denied" if status == 403 else "provider_unavailable")
        raise Blocked("negative read prerequisite not available", kind=kind,
                      path=path, actual=status)
    a.require(status in allowed, "negative request returned wrong HTTP status",
              path=path, expected="/".join(map(str, allowed)), actual=status)


def epg_invalid_range(a):
    now_s = int(time.time())
    path = endpoint(V1 + "/epg/window", backendId=a.backend,
                    channelId=a.context["channel"], fromTime=now_s + 3600,
                    untilTime=now_s, limit=1)
    status_negative(a, path, (400,))


def epg_cursor_scope(a):
    # Invalid fabricated cursor is GET only; backend/view grant remains enforced.
    now_s = int(time.time())
    path = endpoint(V1 + "/epg/window", backendId=a.backend,
                    channelId=a.context["channel"], fromTime=now_s,
                    untilTime=now_s + 60, limit=1, cursor="invalid-v1-cursor")
    status_negative(a, path, (400,))


def recording_bad_public_id(a):
    path = endpoint(V1 + "/recordings/rec_bad", backendId=a.backend)
    status_negative(a, path, (400,))


def artwork_unknown(a):
    # Syntactically valid but absent; never needs an actual image fixture.
    ident = "f" * 32
    path = endpoint(V1 + "/recordings/" + a.context["recording"] +
                    "/artwork/" + ident, backendId=a.backend)
    status_negative(a, path, (404,))


def searchtimer_bad_public_id(a):
    path = endpoint(V1 + "/search-timers/st_bad", backend=a.backend)
    status_negative(a, path, (400,))


def account_admin_read(a):
    # Admin/Human GET cannot be authenticated with a Device grant alone.
    # The current configured identity is a paired Device, not a fabricated
    # Browser Session. Explicitly hold the matrix item open.
    raise Blocked("admin Browser Session/Human Account context not configured; "
                  "no guessed cookies, CSRF or privilege elevation")


def foreign_device_ownership(a):
    raise Blocked("separately authorized foreign Device and owned test session "
                  "are not provided; cannot assert ownership isolation")

def controlled_gate(a):
    approval = a.args.approval_file
    if not approval:
        raise Blocked("controlled hardware mutation requires a separate operator approval")
    if not approval.is_file() or approval.is_symlink():
        raise Blocked("operator approval file missing or unsafe")
    if approval.stat().st_mode & 0o077:
        raise Blocked("operator approval file must be private mode 0600")
    try:
        payload = json.loads(approval.read_text(encoding="utf-8"))
    except (ValueError, OSError, UnicodeError):
        raise Blocked("operator approval manifest unavailable or invalid")
    if not isinstance(payload, dict):
        raise Blocked("operator approval manifest must be an object")
    required = ("sourceCommit", "backend", "targetHost", "approvedFixtureIds",
                "backupEvidence", "rollbackEvidence")
    if any(not payload.get(k) for k in required):
        raise Blocked("controlled approval manifest missing provenance/fixture/backup/rollback")
    if (not a.expected_daemon_commit
            or payload["sourceCommit"] != a.expected_daemon_commit
            or payload["backend"] != a.backend):
        raise Blocked("controlled approval does not bind the selected daemon target/backend")
    if payload["targetHost"] != a.client.origin:
        raise Blocked("controlled approval does not bind the selected daemon origin")
    fixtures = payload["approvedFixtureIds"]
    if not isinstance(fixtures, list) or not fixtures or not all(
            isinstance(item, str) and item for item in fixtures):
        raise Blocked("controlled approvedFixtureIds must contain explicitly selected fixtures")
    raise Blocked("mutation execution intentionally locked: no explicit operator "
                  "authorization has been granted for a real hardware write run")


def run_cases(a):
    # The canonical checklist is referenced, never duplicated or modified.
    a.check("source.commit", "CONTRACT", lambda: check_source(a))
    a.check("source.openapi", "CONTRACT", lambda: check_spec(a))
    a.check("source.v1-contract", "CONTRACT", lambda: source_contract_tests(a),
            requires=("spec",))
    a.check("source.runner-regressions", "CONTRACT", lambda: offline_unit(a))
    a.check("source.controlled-write-contracts", "CONTRACT",
            lambda: controlled_contracts_offline(a), requires=("spec",))
    def hardware(ident, area, task, *, requires=(), required=True):
        if a.args.profile == "offline":
            return a.record(ident, area, "SKIP",
                            "offline profile intentionally does not contact any daemon",
                            required=False)
        if ident not in ("discovery.root", "preflight.source-runtime"):
            requires = ("runtime_verified",) + tuple(requires)
        return a.check(ident, area, task, requires=requires, required=required)

    hardware("discovery.root", "DISCOVERY", lambda: root(a))
    hardware("preflight.source-runtime", "INTEGRATION", lambda: runtime_match(a),
            requires=("root",))
    hardware("discovery.capabilities", "DISCOVERY", lambda: discovery(a),
            requires=("root",))
    hardware("security.unauthenticated", "SECURITY", lambda: missing_auth(a),
            requires=("root",))
    hardware("security.invalid-credential", "SECURITY", lambda: invalid_auth(a),
            requires=("root",))
    hardware("backends.authorized", "BACKENDS", lambda: backends(a),
            requires=("root",))
    hardware("security.backend-isolation", "SECURITY", lambda: isolation(a),
            requires=("backend",))
    hardware("security.account-admin-read", "SECURITY", lambda: account_admin_read(a))
    hardware("security.foreign-device-ownership", "SECURITY", lambda: foreign_device_ownership(a))
    # All reads depend on an authoritative selected backend.
    hardware("epg.channels", "EPG", lambda: channels(a), requires=("backend",))
    hardware("epg.now-next", "EPG", lambda: now_next(a), requires=("channel",))
    hardware("epg.bounded-cursor", "EPG", lambda: epg_window(a), requires=("channel",))
    hardware("epg.reject-invalid-window", "EPG", lambda: epg_invalid_range(a),
            requires=("channel",))
    hardware("epg.reject-invalid-cursor", "EPG", lambda: epg_cursor_scope(a),
            requires=("channel",))
    hardware("timer.assignments", "TIMER READ", lambda: timer_assignments(a),
            requires=("backend",))
    hardware("timer.assignment-detail", "TIMER READ", lambda: timer_detail(a),
            requires=("backend",))
    hardware("timer.native", "TIMER READ", lambda: native_timers(a),
            requires=("backend",))
    hardware("timer.native-detail", "TIMER READ", lambda: native_timer_detail(a),
            requires=("native_timer",))
    hardware("timer.owned-operation", "TIMER READ", lambda: authorized_operation(a),
            requires=("backend",))
    hardware("searchtimer.list", "SEARCHTIMER", lambda: searchtimers(a),
            requires=("backend",))
    hardware("searchtimer.detail", "SEARCHTIMER", lambda: search_detail(a),
            requires=("searchtimer",))
    hardware("searchtimer.version", "SEARCHTIMER", lambda: search_version(a),
            requires=("searchtimer",))
    hardware("searchtimer.preview", "SEARCHTIMER", lambda: search_preview(a),
            requires=("searchtimer",))
    hardware("searchtimer.draft-preview", "SEARCHTIMER", lambda: draft_preview(a),
            requires=("backend",))
    hardware("searchtimer.reject-bad-id", "SEARCHTIMER", lambda: searchtimer_bad_public_id(a),
            requires=("backend",))
    hardware("recording.collection", "RECORDINGS", lambda: recordings(a),
            requires=("backend",))
    hardware("recording.folders", "RECORDINGS", lambda: folders(a),
            requires=("backend",))
    hardware("recording.detail", "RECORDINGS", lambda: recording_detail(a),
            requires=("recording",))
    hardware("recording.reject-bad-id", "RECORDINGS", lambda: recording_bad_public_id(a),
            requires=("backend",))
    hardware("recording.metadata", "METADATA", lambda: metadata(a),
            requires=("recording_detail",))
    hardware("recording.series-episode-metadata", "METADATA", lambda: rich_metadata(a),
            requires=("recording_detail",))
    hardware("recording.genres", "METADATA", lambda: genre(a),
            requires=("backend",))
    hardware("recording.artwork-list", "ARTWORK", lambda: artwork_list(a),
            requires=("recording",))
    hardware("recording.artwork-binary", "ARTWORK", lambda: artwork_bytes(a),
            requires=("artwork_url",))
    hardware("recording.artwork-not-found", "ARTWORK", lambda: artwork_unknown(a),
            requires=("recording",))
    hardware("search.recordings", "INTEGRATION", lambda: search_recordings(a),
            requires=("backend",))
    hardware("playback.profile", "PLAYBACK", lambda: playback_capabilities(a),
            requires=("backend",))
    hardware("resume.continue-watching", "RESUME", lambda: continue_watching(a),
            requires=("backend",))
    hardware("resume.progress", "RESUME", lambda: resume_progress(a),
            requires=("recording",))
    hardware("resume.saved-position", "RESUME", lambda: resume_saved_position(a),
            requires=("recording",))
    for ident, area in (
        ("playback.session-write", "PLAYBACK"), ("resume.progress-write", "RESUME"),
        ("timer.intent-create", "TIMER WRITE"), ("timer.native-mutations", "TIMER WRITE"),
        ("searchtimer.mutations", "SEARCHTIMER"), ("recording.actions", "RECORDINGS"),
        ("security.credential-and-grant-mutations", "SECURITY")):
        locked = a.args.profile == "controlled"
        a.record(ident, area, "BLOCKED" if locked else "SKIP",
                 "productive writes not authorized or executed; mutation dispatcher locked",
                 required=locked)
    if a.args.profile == "controlled":
        hardware("controlled.approval", "INTEGRATION", lambda: controlled_gate(a))
        for name in ("recording.actions", "timer.intent", "native-timer.mutations",
                     "searchtimer.cas-mutations", "playback.session-and-progress",
                     "security.grant-changes"):
            a.record("controlled." + name, "TIMER WRITE" if "timer" in name else "INTEGRATION",
                     "BLOCKED", "real hardware mutation not authorized or executed")
