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
from urllib.parse import urlencode, urlsplit

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
    if a.args.profile != "offline" and a.branch not in (
            "main", "work/public-v1-unified-acceptance-runner-20261023"):
        raise Blocked("unexpected checkout branch for real VDR evidence")


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


def root(a):
    d = get(a, V1)
    a.require(d.get("apiVersion") == "v1", "incorrect API major",
              path=V1, shape="apiVersion=v1")
    a.runtime_version = str(d.get("serverVersion", "unavailable"))[:96]
    a.context["root"] = d


def runtime_match(a):
    version = a.context["root"]["serverVersion"]
    if not isinstance(version, str):
        raise Blocked("runtime did not expose a build identity")
    # Repository builds explicitly default to git-<short SHA>;
    # package-version overrides are not enough for final provenance.
    if not version.startswith("git-") or len(version[4:]) < 7:
        a.evidence = "NOT_VERIFIED (package release identity; source equivalence unknown)"
        raise Blocked("daemon build identity is not a Git SHA; verify installed binary/package")
    gitpart = version[4:]
    if not re.fullmatch("[0-9a-f]{7,40}", gitpart):
        raise Blocked("invalid git build identity")
    if not a.repo_sha.startswith(gitpart):
        a.evidence = "MISMATCH"
        raise Blocked("installed daemon and source checkout have DIFFERENT commits")
    a.evidence = "MATCHED_GIT_BUILD_ID (" + gitpart + ")"


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
    first_ids = [(x.get("eventId"), x.get("startTime")) for x in items]
    a.require(len(first_ids) == len(set(map(str, first_ids))),
              "duplicate first-page EPG identities", path=path)
    page = d.get("page", {})
    cursor = page.get("nextCursor") if isinstance(page, dict) else None
    if cursor:
        p2 = path + "&" + urlencode({"cursor": cursor})
        nxt = a.get_json(p2)[0]
        nxt_items = collection(nxt, p2, 3)
        next_ids = [(x.get("eventId"), x.get("startTime")) for x in nxt_items]
        a.require(not set(map(str, first_ids)).intersection(map(str, next_ids)),
                  "EPG cursor returned duplicate event", path=p2)
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
    items = d.get("items", [])
    a.require(isinstance(items, list) and len(items) <= 5,
              "native timer collection not bounded")


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
    a.require(no_private_paths(d["metadata"]), "metadata contains private path")


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
                      for k in dict(__import__("urllib.parse", fromlist=["parse_qsl"]).parse_qsl(
                          urlsplit(uri).query))),
              "artwork URL must be relative without credentials", path=path)
    a.context["artwork_url"] = endpoint(expected, backendId=a.backend)


def artwork_bytes(a):
    a.client.expect(a.context["artwork_url"], binary=True)


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


def search_recordings(a):
    path = endpoint(V1 + "/search/recordings", backendId=a.backend,
                    query="VDRSuiteNoMatch", limit=3, offset=0)
    data, _ = a.get_json(path)
    collection(data, path, 3)


def controlled_gate(a):
    approval = a.args.approval_file
    if not approval:
        raise Blocked("controlled hardware mutation requires a separate operator approval")
    if approval.is_symlink() or approval.stat().st_mode & 0o077:
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
    if payload["sourceCommit"] != a.repo_sha or payload["backend"] != a.backend:
        raise Blocked("controlled approval does not bind this exact source/backend")
    raise Blocked("mutation execution intentionally locked: no explicit operator "
                  "authorization has been granted for a real hardware write run")


def run_cases(a):
    # The canonical checklist is referenced, never duplicated or modified.
    a.check("source.commit", "CONTRACT", lambda: check_source(a))
    a.check("source.openapi", "CONTRACT", lambda: check_spec(a))
    a.check("source.v1-contract", "CONTRACT", lambda: source_contract_tests(a),
            requires=("spec",))
    a.check("source.runner-regressions", "CONTRACT", lambda: offline_unit(a))
    if a.args.profile == "offline":
        a.record("hardware.reads", "INTEGRATION", "SKIP",
                 "offline profile deliberately does not contact a daemon",
                 required=False)
        a.record("hardware.mutations", "TIMER WRITE", "SKIP",
                 "offline profile forbids all hardware writes", required=False)
        return

    a.check("discovery.root", "DISCOVERY", lambda: root(a))
    a.check("preflight.source-runtime", "INTEGRATION", lambda: runtime_match(a),
            requires=("root",))
    a.check("discovery.capabilities", "DISCOVERY", lambda: discovery(a),
            requires=("root",))
    a.check("security.unauthenticated", "SECURITY", lambda: missing_auth(a),
            requires=("root",))
    a.check("security.invalid-credential", "SECURITY", lambda: invalid_auth(a),
            requires=("root",))
    a.check("security.backend-isolation", "SECURITY", lambda: isolation(a),
            requires=("backend",))
    a.check("backends.authorized", "BACKENDS", lambda: backends(a),
            requires=("root",))
    # All reads depend on an authoritative selected backend.
    a.check("epg.channels", "EPG", lambda: channels(a), requires=("backend",))
    a.check("epg.now-next", "EPG", lambda: now_next(a), requires=("channel",))
    a.check("epg.bounded-cursor", "EPG", lambda: epg_window(a), requires=("channel",))
    a.check("timer.assignments", "TIMER READ", lambda: timer_assignments(a),
            requires=("backend",))
    a.check("timer.assignment-detail", "TIMER READ", lambda: timer_detail(a),
            requires=("backend",))
    a.check("timer.native", "TIMER READ", lambda: native_timers(a),
            requires=("backend",))
    a.check("searchtimer.list", "SEARCHTIMER", lambda: searchtimers(a),
            requires=("backend",))
    a.check("searchtimer.detail", "SEARCHTIMER", lambda: search_detail(a),
            requires=("searchtimer",))
    a.check("searchtimer.version", "SEARCHTIMER", lambda: search_version(a),
            requires=("searchtimer",))
    a.check("searchtimer.preview", "SEARCHTIMER", lambda: search_preview(a),
            requires=("searchtimer",))
    a.check("searchtimer.draft-preview", "SEARCHTIMER", lambda: draft_preview(a),
            requires=("backend",))
    a.check("recording.collection", "RECORDINGS", lambda: recordings(a),
            requires=("backend",))
    a.check("recording.folders", "RECORDINGS", lambda: folders(a),
            requires=("backend",))
    a.check("recording.detail", "RECORDINGS", lambda: recording_detail(a),
            requires=("recording",))
    a.check("recording.metadata", "METADATA", lambda: metadata(a),
            requires=("recording_detail",))
    a.check("recording.genres", "METADATA", lambda: genre(a),
            requires=("backend",))
    a.check("recording.artwork-list", "ARTWORK", lambda: artwork_list(a),
            requires=("recording",))
    a.check("recording.artwork-binary", "ARTWORK", lambda: artwork_bytes(a),
            requires=("artwork_url",))
    a.check("search.recordings", "INTEGRATION", lambda: search_recordings(a),
            requires=("backend",))
    a.check("playback.profile", "PLAYBACK", lambda: playback_capabilities(a),
            requires=("backend",))
    a.check("resume.continue-watching", "RESUME", lambda: continue_watching(a),
            requires=("backend",))
    a.check("resume.progress", "RESUME", lambda: resume_progress(a),
            requires=("recording",))
    a.record("playback.session-write", "PLAYBACK", "SKIP",
             "safe readonly profile forbids MediaSession create/stop",
             required=False)
    a.record("timer.intent-write", "TIMER WRITE", "SKIP",
             "safe readonly profile forbids TimerIntent and native writes",
             required=False)
    if a.args.profile == "controlled":
        a.check("controlled.approval", "INTEGRATION", lambda: controlled_gate(a))
        for name in ("recording.actions", "timer.intent", "native-timer.mutations",
                     "searchtimer.cas-mutations", "playback.session-and-progress",
                     "security.grant-changes"):
            a.record("controlled." + name, "TIMER WRITE" if "timer" in name else "INTEGRATION",
                     "BLOCKED", "real hardware mutation not authorized or executed")
