"""Fail-closed Public API v1 acceptance engine. Python stdlib only.

No productive request other than GET can ever be emitted by HttpClient.
The final hardware write matrix is in docs/development/public-v1-final-hardware-acceptance.md.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_RESULTS = Path.home() / "vdr-suite-api-v1-results"
DEFAULT_CONFIG = Path.home() / ".config/vdr-suite/api-v1-acceptance.json"
DEFAULT_ORIGIN = "http://127.0.0.1:18080"
MAX_JSON = 1024 * 1024
MAX_IMAGE = 256 * 1024
TIMEOUT = 8
SHA = re.compile(r"^[0-9a-f]{40}$")
SAFE_ID = re.compile(r"^(?:rec_|fld1_|st_|ms_|op_)[0-9a-f]{32}$")
BACKEND = re.compile(r"^[A-Za-z0-9._-]{1,128}$")
AREAS = ("DISCOVERY", "SECURITY", "BACKENDS", "EPG", "TIMER READ",
         "TIMER WRITE", "SEARCHTIMER", "RECORDINGS", "METADATA",
         "ARTWORK", "PLAYBACK", "RESUME", "INTEGRATION", "CONTRACT")
STATUSES = ("PASS", "FAIL", "SKIP", "BLOCKED")
EXIT = {"PASS": 0, "FAIL": 1, "INCOMPLETE": 2, "CONFIG_ERROR": 3}


def now():
    return dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")


def shell_git(*args):
    try:
        x = subprocess.run(["git", "-C", str(ROOT), *args], text=True,
                           stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                           timeout=5, check=True)
        return x.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return ""


def safe_path(path):
    """Log only route structure, never query values or opaque object IDs."""
    p = urllib.parse.urlsplit(path).path
    parts = []
    redact_after = {"timer-assignments", "operations", "native-timers", "search-timers",
                    "recordings", "devices", "accounts", "device-pairings",
                    "live-playback-sessions", "recording-playback-sessions"}
    static_suffixes = {"artwork", "version", "preview", "draft-preview", "progress",
                       "capabilities", "continue-watching", "credentials", "sessions",
                       "grants", "lifecycle", "stop", "tracks", "trickplay", "subtitles",
                       "mutations", "modifications", "actions", "validate", "execute",
                       "rotate", "playback-account"}
    previous = ""
    for element in p.split("/"):
        if (previous in redact_after and element not in static_suffixes) or SAFE_ID.fullmatch(element):
            parts.append("<public-id>")
        elif element and not re.fullmatch(r"[A-Za-z0-9._-]{1,100}", element):
            parts.append("<redacted>")
        else:
            parts.append(element)
        if element:
            previous = element
    q = urllib.parse.parse_qsl(urllib.parse.urlsplit(path).query,
                              keep_blank_values=True)
    keys = sorted({k for k, _ in q if re.fullmatch(r"[A-Za-z][A-Za-z0-9-]{0,50}", k)})
    return "/".join(parts) + (("?keys=" + ",".join(keys)) if keys else "")


def safe_code(value):
    if isinstance(value, str) and re.fullmatch(r"[a-zA-Z0-9_.-]{1,72}", value):
        return value
    return None


class Problem(Exception):
    def __init__(self, kind, detail, *, method="GET", path="", expected="200",
                 actual=None, shape="", observed="", request_id=None, operation_id=None):
        super().__init__(detail)
        self.kind = kind
        self.detail = detail
        self.method = method
        self.path = safe_path(path)
        self.expected = expected
        self.actual = actual
        self.shape = shape
        self.observed = observed
        self.request_id = safe_code(request_id)
        self.operation_id = safe_code(operation_id)


class Blocked(Problem):
    def __init__(self, detail, *, kind="prerequisite_missing", **kw):
        super().__init__(kind, detail, **kw)


class Skip(Problem):
    def __init__(self, detail, **kw):
        super().__init__("intentionally_skipped", detail, **kw)


class ConfigError(Exception):
    pass


def origin_ok(origin):
    p = urllib.parse.urlsplit(origin)
    loopback = p.hostname in ("127.0.0.1", "::1", "localhost")
    if (p.scheme not in ("http", "https") or (p.scheme == "http" and not loopback)
            or not p.hostname or p.username or p.password or p.query or p.fragment
            or p.path not in ("", "/")):
        raise ConfigError("origin must be an HTTPS origin or an HTTP loopback origin")
    return origin.rstrip("/")


def load_config(path):
    if not path.exists():
        return {}
    if path.is_symlink():
        raise ConfigError("credential configuration must not be a symlink")
    mode = path.stat().st_mode
    if mode & 0o077:
        raise ConfigError("credential configuration must be private (chmod 600)")
    try:
        cfg = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, ValueError) as e:
        raise ConfigError("credential configuration cannot be read as JSON") from e
    if not isinstance(cfg, dict):
        raise ConfigError("configuration JSON must contain an object")
    # Explicitly selected legacy one-time credential receipts use a flat
    # {credentialId, credentialSecret} object. Accept only complete pairs,
    # normalize in memory, and never rewrite the original on disk.
    flat = "credentialId" in cfg or "credentialSecret" in cfg
    if flat:
        if "device" in cfg:
            raise ConfigError("ambiguous nested and flat device credential configuration")
        if not all(isinstance(cfg.get(k), str) and cfg[k] for k in
                   ("credentialId", "credentialSecret")):
            raise ConfigError("flat device credential receipt requires both fields")
        cfg = {"device": {"credentialId": cfg["credentialId"],
                          "credentialSecret": cfg["credentialSecret"]}}
    if "device" in cfg:
        d = cfg["device"]
        if not isinstance(d, dict) or not all(
                isinstance(d.get(k), str) and d[k] for k in
                ("credentialId", "credentialSecret")):
            raise ConfigError("device credentials require credentialId and credentialSecret")
        if not re.fullmatch(r"[A-Za-z0-9_-]{1,160}", d["credentialId"]):
            raise ConfigError("invalid device credentialId format")
        if not re.fullmatch(r"[A-Za-z0-9_.~-]{1,512}", d["credentialSecret"]):
            raise ConfigError("invalid device credentialSecret format")
    return cfg


def auth_header(cfg):
    d = cfg.get("device")
    if not d:
        return None
    return "VDR-Suite-Device " + d["credentialId"] + "." + d["credentialSecret"]


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


class HttpClient:
    def __init__(self, origin, cfg=None):
        self.origin = origin_ok(origin)
        self.cfg = cfg or {}
        self.opener = urllib.request.build_opener(
            urllib.request.ProxyHandler({}), NoRedirect())

    def get(self, path, *, auth=True, binary=False, invalid=False):
        if not path.startswith("/api/v1") or path.startswith("//"):
            raise Problem("unsafe_route", "only absolute public-v1 routes are permitted", path=path)
        if urllib.parse.urlsplit(path).scheme or urllib.parse.urlsplit(path).netloc:
            raise Problem("unsafe_route", "external request target refused", path=path)
        # Deliberately never supply data or a non-GET method.
        headers = {"Accept": "image/*" if binary else "application/json"}
        token = auth_header(self.cfg) if auth else None
        if invalid:
            token = "VDR-Suite-Device invalid.invalid"
        if token:
            headers["Authorization"] = token
        req = urllib.request.Request(self.origin + path, method="GET", headers=headers)
        status, raw, reply = None, b"", {}
        try:
            with self.opener.open(req, timeout=TIMEOUT) as r:
                status, reply = r.status, dict(r.headers.items())
                raw = r.read((MAX_IMAGE if binary else MAX_JSON) + 1)
        except urllib.error.HTTPError as e:
            status, reply = e.code, dict(e.headers.items())
            raw = e.read(MAX_JSON + 1)
        except (TimeoutError, OSError, urllib.error.URLError) as e:
            raise Blocked("daemon/backend unreachable or timed out (" + type(e).__name__ + ")",
                          path=path)
        header = {k.lower(): v for k, v in reply.items()}
        cid = header.get("x-request-id") or header.get("x-correlation-id")
        if len(raw) > (MAX_IMAGE if binary and status == 200 else MAX_JSON):
            raise Problem("response_too_large", "bounded response size exceeded",
                          path=path, actual=status, request_id=cid)
        data = None
        if binary and status == 200:
            mime = header.get("content-type", "").split(";", 1)[0].strip().lower()
            jpeg = mime == "image/jpeg" and raw.startswith(b"\xff\xd8\xff")
            png = mime == "image/png" and raw.startswith(b"\x89PNG\r\n\x1a\n")
            webp = mime == "image/webp" and raw[:4] == b"RIFF" and raw[8:12] == b"WEBP"
            if not (jpeg or png or webp):
                raise Problem("invalid_image", "artwork MIME or image signature invalid",
                              path=path, actual=status, request_id=cid,
                              shape="real image bytes with matching image MIME")
            data = {"image_type": mime, "bytes_read": len(raw)}
            if not raw:
                raise Problem("invalid_image", "empty artwork response", path=path)
        else:
            try:
                data = json.loads(raw.decode("utf-8")) if raw else {}
            except (UnicodeError, ValueError):
                if status in (401, 403, 503):
                    data = {"error": {"code": "non_json_error"}}
                else:
                    raise Problem("wrong_json_structure", "non-JSON response",
                                  path=path, actual=status, request_id=cid,
                                  shape="valid JSON object")
        return status, data, header

    def expect(self, path, *, statuses=(200,), auth=True, invalid=False, binary=False):
        code, data, hdr = self.get(path, auth=auth, invalid=invalid, binary=binary)
        if code not in statuses:
            kind = ("authentication_failed" if code == 401 and auth and not invalid
                    else "permission_denied" if code == 403 and auth and not invalid
                    else "provider_unavailable" if code == 503
                    else "route_missing" if code == 404
                    else "http_status_mismatch")
            err = data.get("error") if isinstance(data, dict) else None
            key = err.get("code") if isinstance(err, dict) else (
                err if isinstance(err, str) else data.get("code") if isinstance(data, dict) else None)
            detail = "unexpected HTTP status; error_code=" + str(safe_code(key) or "unavailable")
            info = dict(path=path, expected="/".join(map(str, statuses)), actual=code,
                        observed="JSON keys: " + ",".join(sorted(data.keys())[:20])
                        if isinstance(data, dict) else "non-object")
            if kind in ("authentication_failed", "permission_denied", "provider_unavailable"):
                raise Blocked(detail, kind=kind, **info)
            raise Problem(kind, detail, **info)
        if code == 200 and not binary:
            if not isinstance(data, dict):
                raise Problem("wrong_json_structure", "expected JSON object", path=path,
                              actual=code, shape="object")
            typ = hdr.get("content-type", "").lower()
            if "application/json" not in typ:
                raise Problem("wrong_content_type", "expected application/json",
                              path=path, actual=code, shape="application/json")
        return data, hdr


def validate(value, schema, defs, depth=0):
    """Bounded OpenAPI response subset validator (refs, type, required, items)."""
    if depth > 24 or not isinstance(schema, dict):
        raise ValueError("invalid/cyclic schema")
    if "$ref" in schema:
        name = schema["$ref"].removeprefix("#/components/schemas/")
        if name not in defs:
            raise ValueError("unresolved schema ref: " + name)
        return validate(value, defs[name], defs, depth + 1)
    if "allOf" in schema:
        return all(validate(value, sub, defs, depth + 1) for sub in schema["allOf"])
    if "anyOf" in schema:
        return any(validate(value, sub, defs, depth + 1) for sub in schema["anyOf"])
    if "oneOf" in schema:
        return sum(validate(value, sub, defs, depth + 1) for sub in schema["oneOf"]) == 1
    types = schema.get("type")
    if types:
        types = types if isinstance(types, list) else [types]
        mapping = {"object": lambda v: isinstance(v, dict),
                   "array": lambda v: isinstance(v, list),
                   "string": lambda v: isinstance(v, str),
                   "integer": lambda v: type(v) is int,
                   "number": lambda v: isinstance(v, (float, int)) and not isinstance(v, bool),
                   "boolean": lambda v: isinstance(v, bool),
                   "null": lambda v: v is None}
        if not any(mapping[t](value) for t in types if t in mapping):
            return False
    if "const" in schema and value != schema["const"]:
        return False
    if "enum" in schema and value not in schema["enum"]:
        return False
    if isinstance(value, dict):
        if any(key not in value for key in schema.get("required", [])):
            return False
        if schema.get("additionalProperties") is False and any(
                key not in schema.get("properties", {}) for key in value):
            return False
        return all(validate(v, schema["properties"][k], defs, depth + 1)
                   for k, v in value.items() if k in schema.get("properties", {}))
    if isinstance(value, list):
        if "maxItems" in schema and len(value) > schema["maxItems"]:
            return False
        if "minItems" in schema and len(value) < schema["minItems"]:
            return False
        return all(validate(v, schema["items"], defs, depth + 1)
                   for v in value) if "items" in schema else True
    if isinstance(value, str):
        if "pattern" in schema and not re.search(schema["pattern"], value):
            return False
        if "maxLength" in schema and len(value) > schema["maxLength"]:
            return False
        if "minLength" in schema and len(value) < schema["minLength"]:
            return False
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        if "minimum" in schema and value < schema["minimum"]:
            return False
        if "maximum" in schema and value > schema["maximum"]:
            return False
    return True


class Acceptance:
    def __init__(self, args, cfg):
        self.args, self.cfg = args, cfg
        self.repo_sha = shell_git("rev-parse", "HEAD")
        self.branch = shell_git("branch", "--show-current")
        # Runner Git HEAD is not the installed daemon Git identity.
        self.expected_daemon_commit = (getattr(args, "expected_daemon_commit", None)
                                       or cfg.get("expectedDaemonCommit"))
        if self.expected_daemon_commit is not None and not SHA.fullmatch(self.expected_daemon_commit):
            raise ConfigError("expected daemon commit must be an exact 40-character Git SHA")
        self.client = HttpClient(args.origin or cfg.get("origin", DEFAULT_ORIGIN), cfg)
        self.backend = args.backend or cfg.get("backend", "default")
        if not BACKEND.fullmatch(self.backend):
            raise ConfigError("backend identifier must match [A-Za-z0-9._-]{1,128}")
        self.output = args.output_dir.expanduser()
        self.started = now()
        self.results = []
        self.context = {}
        self.spec = None
        self.sources = "docs/development/public-v1-final-hardware-acceptance.md"
        self.run_dir = self.output / "runs" / dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
        self.runtime_version = "not observed"
        self.evidence = "NOT_VERIFIED"
        self._setup()

    def _setup(self):
        old = os.umask(0o077)
        try:
            if self.output.is_symlink():
                raise ConfigError("output directory must not be a symlink")
            self.run_dir.mkdir(mode=0o700, parents=True, exist_ok=False)
            self.output.chmod(0o700)
        finally:
            os.umask(old)

    def _write(self, target, content):
        fd, path = tempfile.mkstemp(prefix=".v1-", dir=str(target.parent))
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as f:
                f.write(content)
            os.chmod(path, 0o600)
            os.replace(path, target)
        finally:
            if os.path.exists(path):
                os.unlink(path)

    def record(self, ident, area, status, detail="", *, method="GET",
               path="", expected=None, actual=None, shape="", observed="",
               request_id=None, operation_id=None, required=True):
        item = {"id": ident, "timestamp": now(), "area": area, "status": status,
                "required": required, "reason": detail, "method": method,
                "path": safe_path(path), "expectedStatus": expected,
                "actualStatus": actual, "expectedStructure": shape,
                "observed": observed, "requestId": safe_code(request_id),
                "operationId": safe_code(operation_id)}
        self.results.append(item)
        self.persist()
        return status

    def check(self, ident, area, task, *, requires=(), required=True):
        if any(self.context.get(key) is None for key in requires):
            return self.record(ident, area, "BLOCKED", "dependent fixture/check missing: " +
                               ",".join(k for k in requires if self.context.get(k) is None),
                               required=required)
        try:
            evidence = task()
            observed = (evidence if isinstance(evidence, str) and
                        re.fullmatch(r"[A-Za-z0-9 _.+:/-]{1,100}", evidence)
                        else "")
            return self.record(ident, area, "PASS", observed=observed,
                               required=required)
        except Skip as e:
            return self.record(ident, area, "SKIP", e.detail, method=e.method,
                               path=e.path, required=required)
        except Blocked as e:
            return self.record(ident, area, "BLOCKED", e.kind + ": " + e.detail, method=e.method,
                               path=e.path, expected=e.expected, actual=e.actual,
                               required=required)
        except Problem as e:
            return self.record(ident, area, "FAIL", e.kind + ": " + e.detail,
                               method=e.method, path=e.path, expected=e.expected,
                               actual=e.actual, shape=e.shape, observed=e.observed,
                               request_id=e.request_id, operation_id=e.operation_id,
                               required=required)
        except Exception as e:
            # Deliberately never format exception strings: they may contain URLs/secrets.
            return self.record(ident, area, "FAIL",
                               "unexpected_exception: " + type(e).__name__,
                               required=required)

    def require(self, ok, detail, **kw):
        if not ok:
            raise Problem("contract_mismatch", detail, **kw)

    def get_json(self, path, *, required_keys=(), auth=True, invalid=False):
        data, hdr = self.client.expect(path, auth=auth, invalid=invalid)
        self.require(all(k in data for k in required_keys),
                     "missing required response keys: " + ",".join(
                         k for k in required_keys if k not in data),
                     path=path, shape=",".join(required_keys))
        if self.spec is not None and not invalid:
            base = urllib.parse.urlsplit(path).path
            match = next((name for name in self.spec["paths"] if
                          re.fullmatch(re.sub(r"\{[^/]+\}", "[^/]+", name), base)), None)
            if match:
                op = self.spec["paths"][match].get("get", {})
                response = op.get("responses", {}).get("200", {})
                schema = response.get("content", {}).get("application/json", {}).get("schema")
                if schema:
                    self.require(validate(data, schema, self.spec["components"]["schemas"]),
                                 "OpenAPI response schema mismatch", path=path,
                                 shape=match + " GET 200")
        return data, hdr

    def persist(self):
        counts = {s: sum(t["status"] == s for t in self.results) for s in STATUSES}
        if counts["FAIL"]:
            overall = "FAIL"
        elif any(t["required"] and t["status"] != "PASS" for t in self.results):
            overall = "INCOMPLETE"
        elif not self.results:
            overall = "INCOMPLETE"
        else:
            overall = "PASS"
        status_by_area = {}
        for area in AREAS:
            values = [t["status"] for t in self.results if t["area"] == area]
            status_by_area[area] = ("FAIL" if "FAIL" in values else
                                    "BLOCKED" if "BLOCKED" in values else
                                    "SKIP" if "SKIP" in values else
                                    "PASS" if values else "SKIP")
        summary = [
            "VDR-SUITE PUBLIC API V1 ACCEPTANCE",
            "==================================",
            "Date: " + self.started,
            "Runner source commit: " + (self.repo_sha or "unknown"),
            "Expected daemon commit: " + (self.expected_daemon_commit or "NOT_CONFIGURED"),
            "Source branch: " + (self.branch or "unknown"),
            "Installed daemon: " + self.runtime_version,
            "Daemon target verification: " + self.evidence,
            "Backend: " + self.backend,
            "Profile: " + self.args.profile, "",
        ]
        summary.extend(f"{area:<17} {s}" for area, s in status_by_area.items())
        summary += ["", "-" * 34]
        summary += [f"{key}: {counts[key]}" for key in STATUSES]
        summary += ["-" * 34, "OVERALL: " + overall,
                    "ERROR_LOG: " + str(self.run_dir / "errors.log"),
                    "RESULT_JSON: " + str(self.run_dir / "results.json"),
                    "NOTE: REAL-VDR final hardware acceptance is NOT established.",
                    ""]
        error_lines = []
        for r in self.results:
            if r["status"] not in ("FAIL", "BLOCKED"):
                continue
            error_lines.extend(["["+r["timestamp"]+"] " + r["id"] +
                                " [" + r["area"] + "] " + r["status"],
                                "cause=" + r["reason"],
                                "request=" + r["method"] + " " + r["path"],
                                "expected_status=" + str(r["expectedStatus"]) +
                                " actual_status=" + str(r["actualStatus"]),
                                "expected_structure=" + r["expectedStructure"],
                                "observed=" + r["observed"],
                                "request_id=" + str(r["requestId"]) +
                                " operation_id=" + str(r["operationId"]), ""])
        errors = "\n".join(error_lines) + ("\n" if error_lines else "")
        bundle = {"schemaVersion": 1, "date": self.started, "sourceCommit": self.repo_sha,
                  "sourceBranch": self.branch,
                  "expectedDaemonCommit": self.expected_daemon_commit,
                  "installedDaemon": self.runtime_version,
                  "runtimeEvidence": self.evidence, "profile": self.args.profile,
                  "backend": self.backend, "acceptanceChecklist": self.sources,
                  "counts": counts, "overall": overall, "tests": self.results,
                  "finalHardwareAcceptance": "NOT_VERIFIED"}
        files = {"summary.txt": "\n".join(summary), "errors.log": errors,
                 "results.json": json.dumps(bundle, indent=2, ensure_ascii=False) + "\n",
                 "full.log": "\n".join(" ".join((x["timestamp"], x["id"],
                                                 x["status"], x["reason"])) for x in self.results) + "\n"}
        for name, content in files.items():
            self._write(self.run_dir / name, content)
            self._write(self.output / ("last-" + name), content)
        return overall


def arguments(argv=None):
    p = argparse.ArgumentParser(description="Public API v1 safe unified acceptance runner")
    p.add_argument("--profile", choices=("offline", "readonly", "controlled"),
                   default="readonly")
    p.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    p.add_argument("--origin", help="override origin; HTTP is loopback-only")
    p.add_argument("--backend", help="authorized Public-v1 backend ID")
    p.add_argument("--expected-daemon-commit",
                   help="exact target daemon Git SHA, never inferred from runner checkout")
    p.add_argument("--output-dir", type=Path, default=DEFAULT_RESULTS)
    p.add_argument("--approval-file", type=Path,
                   help="hardware-only operator approval manifest (never triggers writes here)")
    return p.parse_args(argv)


def main(argv=None):
    os.umask(0o077)
    a = arguments(argv)
    try:
        cfg = load_config(a.config)
        acceptance = Acceptance(a, cfg)
    except (ConfigError, OSError) as e:
        # Never interpolate a config exception that could disclose a path/token.
        print("CONFIG_ERROR: " + (str(e) if isinstance(e, ConfigError) else type(e).__name__),
              file=sys.stderr)
        return EXIT["CONFIG_ERROR"]
    try:
        from cases import run_cases
        run_cases(acceptance)
    except BaseException as e:
        # KeyboardInterrupt and unexpected orchestration exceptions persist.
        acceptance.record("runner.unhandled", "INTEGRATION", "BLOCKED",
                          "execution_aborted: " + type(e).__name__)
    overall = acceptance.persist()
    print((acceptance.output / "last-summary.txt").read_text())
    return EXIT[overall]


if __name__ == "__main__":
    sys.exit(main())
