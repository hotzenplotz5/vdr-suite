#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
COMPLETED = "Phase 49.26 - EPGSearch native fuzzy operator refresh API validation"
NEXT = "Phase 49.27 - EPGSearch native fuzzy operator refresh safety and permission contract"


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, text):
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(text, old, new, label):
    if old not in text:
        raise SystemExit("Anchor not found: " + label)
    return text.replace(old, new, 1)


def make_lines(items):
    return "\n".join(items) + "\n"


validator = make_lines([
    "#!/usr/bin/env python3",
    "import argparse",
    "import base64",
    "import json",
    "import sys",
    "import time",
    "import urllib.error",
    "import urllib.request",
    "",
    "DEFAULT_ENDPOINTS = [",
    "    \"/api/vdr/epgsearch/native-fuzzy/refresh\",",
    "    \"/api/epgsearch/native-fuzzy/refresh\",",
    "]",
    "REQUIRED_FIELDS = [",
    "    \"backendId\",",
    "    \"backendNativeId\",",
    "    \"probeQuery\",",
    "    \"tolerance\",",
    "    \"backendKnown\",",
    "    \"createAttempted\",",
    "    \"createAccepted\",",
    "    \"readbackAvailable\",",
    "    \"modePreserved\",",
    "    \"tolerancePreserved\",",
    "    \"cleanupAttempted\",",
    "    \"cleanupSucceeded\",",
    "    \"persisted\",",
    "    \"backendCapabilitiesUpdated\",",
    "    \"nativeFuzzyAvailable\",",
    "    \"status\",",
    "    \"warnings\",",
    "    \"errors\",",
    "]",
    "",
    "def normalize_base_path(value):",
    "    if not value:",
    "        return \"\"",
    "    value = value.strip()",
    "    if not value:",
    "        return \"\"",
    "    if not value.startswith(\"/\"):",
    "        value = \"/\" + value",
    "    return value.rstrip(\"/\")",
    "",
    "def build_url(host, port, base_path, endpoint):",
    "    return \"http://{}:{}{}{}\".format(host, port, normalize_base_path(base_path), endpoint)",
    "",
    "def auth_header(user, password):",
    "    if not user:",
    "        return None",
    "    token = (user + \":\" + (password or \"\")).encode(\"utf-8\")",
    "    return \"Basic \" + base64.b64encode(token).decode(\"ascii\")",
    "",
    "def request_http(method, url, payload=None, user=\"\", password=\"\", timeout=10):",
    "    data = None",
    "    headers = {\"Accept\": \"application/json\"}",
    "    if payload is not None:",
    "        data = json.dumps(payload, separators=(\",\", \":\")).encode(\"utf-8\")",
    "        headers[\"Content-Type\"] = \"application/json\"",
    "    auth = auth_header(user, password)",
    "    if auth:",
    "        headers[\"Authorization\"] = auth",
    "    request = urllib.request.Request(url, data=data, headers=headers, method=method)",
    "    try:",
    "        with urllib.request.urlopen(request, timeout=timeout) as response:",
    "            body = response.read().decode(\"utf-8\", errors=\"replace\")",
    "            return response.getcode(), body",
    "    except urllib.error.HTTPError as error:",
    "        body = error.read().decode(\"utf-8\", errors=\"replace\")",
    "        return error.code, body",
    "    except urllib.error.URLError as error:",
    "        return 0, str(error)",
    "",
    "def parse_json_object(body):",
    "    try:",
    "        parsed = json.loads(body)",
    "    except json.JSONDecodeError as error:",
    "        return None, \"response is not valid JSON: {}\".format(error)",
    "    if not isinstance(parsed, dict):",
    "        return None, \"response JSON is not an object\"",
    "    return parsed, \"\"",
    "",
    "def validate_refresh_response(parsed, expected_backend, expected_query, expected_tolerance, expect_native):",
    "    errors = []",
    "    for field in REQUIRED_FIELDS:",
    "        if field not in parsed:",
    "            errors.append(\"missing response field: \" + field)",
    "    if errors:",
    "        return errors",
    "    if not isinstance(parsed.get(\"warnings\"), list):",
    "        errors.append(\"warnings is not a JSON array\")",
    "    if not isinstance(parsed.get(\"errors\"), list):",
    "        errors.append(\"errors is not a JSON array\")",
    "    if parsed.get(\"backendId\") != expected_backend:",
    "        errors.append(\"backendId mismatch: expected {!r}, got {!r}\".format(expected_backend, parsed.get(\"backendId\")))",
    "    if parsed.get(\"probeQuery\") != expected_query:",
    "        errors.append(\"probeQuery mismatch: expected {!r}, got {!r}\".format(expected_query, parsed.get(\"probeQuery\")))",
    "    if parsed.get(\"tolerance\") != expected_tolerance:",
    "        errors.append(\"tolerance mismatch: expected {!r}, got {!r}\".format(expected_tolerance, parsed.get(\"tolerance\")))",
    "    if parsed.get(\"backendKnown\") is not True:",
    "        errors.append(\"backendKnown is not true\")",
    "    if parsed.get(\"createAttempted\") is not True:",
    "        errors.append(\"createAttempted is not true\")",
    "    if parsed.get(\"createAccepted\") is not True:",
    "        errors.append(\"createAccepted is not true\")",
    "    if parsed.get(\"readbackAvailable\") is not True:",
    "        errors.append(\"readbackAvailable is not true\")",
    "    if parsed.get(\"cleanupAttempted\") is not True:",
    "        errors.append(\"cleanupAttempted is not true\")",
    "    if parsed.get(\"cleanupSucceeded\") is not True:",
    "        errors.append(\"cleanupSucceeded is not true\")",
    "    if parsed.get(\"persisted\") is not True:",
    "        errors.append(\"persisted is not true\")",
    "    if parsed.get(\"backendCapabilitiesUpdated\") is not True:",
    "        errors.append(\"backendCapabilitiesUpdated is not true\")",
    "    if expect_native == \"available\" and parsed.get(\"nativeFuzzyAvailable\") is not True:",
    "        errors.append(\"nativeFuzzyAvailable is not true\")",
    "    if expect_native == \"unavailable\" and parsed.get(\"nativeFuzzyAvailable\") is not False:",
    "        errors.append(\"nativeFuzzyAvailable is not false\")",
    "    if not str(parsed.get(\"status\", \"\")):",
    "        errors.append(\"status is empty\")",
    "    return errors",
    "",
    "def parse_endpoints(value):",
    "    if value == \"both\":",
    "        return DEFAULT_ENDPOINTS",
    "    if value == \"vdr\":",
    "        return [DEFAULT_ENDPOINTS[0]]",
    "    if value == \"legacy\":",
    "        return [DEFAULT_ENDPOINTS[1]]",
    "    return [value]",
    "",
    "def run(args):",
    "    base_query = args.query or \"VDR-Suite Native Fuzzy Operator Refresh Validation\"",
    "    endpoints = parse_endpoints(args.endpoint)",
    "    print(\"VDR-Suite native fuzzy operator refresh API validation\")",
    "    print(\"target={}:{}{}\".format(args.host, args.port, normalize_base_path(args.base_path)))",
    "    print(\"backendId={}\".format(args.backend))",
    "    print(\"tolerance={}\".format(args.tolerance))",
    "    print(\"expectedNative={}\".format(args.expect_native))",
    "    if not args.execute:",
    "        print(\"DRY RUN: no HTTP request sent. Use --execute for real operator refresh calls.\")",
    "    ok = True",
    "    for index, endpoint in enumerate(endpoints):",
    "        query = base_query if len(endpoints) == 1 else base_query + \" alias \" + str(index + 1) + \" \" + str(int(time.time()))",
    "        payload = {",
    "            \"backendId\": args.backend,",
    "            \"probeQuery\": query,",
    "            \"tolerance\": args.tolerance,",
    "            \"keepProbeSearchTimer\": False,",
    "            \"updateBackendCapabilities\": True,",
    "        }",
    "        url = build_url(args.host, args.port, args.base_path, endpoint)",
    "        print(\"\")",
    "        print(\"endpoint={}\".format(endpoint))",
    "        print(\"url={}\".format(url))",
    "        print(\"payload=\" + json.dumps(payload, sort_keys=True))",
    "        if not args.execute:",
    "            continue",
    "        status, body = request_http(\"POST\", url, payload, args.user, args.password, args.timeout)",
    "        print(\"status={}\".format(status))",
    "        print(\"body={}\".format(body))",
    "        if status != 200:",
    "            print(\"FAIL: expected HTTP 200\")",
    "            ok = False",
    "            continue",
    "        parsed, error = parse_json_object(body)",
    "        if error:",
    "            print(\"FAIL: \" + error)",
    "            ok = False",
    "            continue",
    "        errors = validate_refresh_response(parsed, args.backend, query, args.tolerance, args.expect_native)",
    "        if errors:",
    "            for item in errors:",
    "                print(\"FAIL: \" + item)",
    "            ok = False",
    "        else:",
    "            print(\"PASS: response contract validated\")",
    "    if not ok:",
    "        return 1",
    "    if args.execute:",
    "        print(\"\")",
    "        print(\"PASS: native fuzzy operator refresh API validation completed\")",
    "    return 0",
    "",
    "def main():",
    "    parser = argparse.ArgumentParser(description=\"Validate VDR-Suite native fuzzy operator refresh API.\")",
    "    parser.add_argument(\"--host\", default=\"127.0.0.1\")",
    "    parser.add_argument(\"--port\", type=int, default=18080)",
    "    parser.add_argument(\"--base-path\", default=\"\")",
    "    parser.add_argument(\"--backend\", default=\"default\")",
    "    parser.add_argument(\"--query\", default=\"\")",
    "    parser.add_argument(\"--tolerance\", type=int, default=2)",
    "    parser.add_argument(\"--endpoint\", default=\"both\", help=\"both, vdr, legacy, or a custom endpoint path\")",
    "    parser.add_argument(\"--expect-native\", choices=[\"available\", \"unavailable\", \"any\"], default=\"any\")",
    "    parser.add_argument(\"--user\", default=\"\")",
    "    parser.add_argument(\"--password\", default=\"\")",
    "    parser.add_argument(\"--timeout\", type=int, default=10)",
    "    parser.add_argument(\"--execute\", action=\"store_true\")",
    "    args = parser.parse_args()",
    "    return run(args)",
    "",
    "if __name__ == \"__main__\":",
    "    sys.exit(main())",
])

write("tools/validate_native_fuzzy_operator_refresh_api.py", validator)

# Documentation additions
path = "docs/development/epgsearch-native-fuzzy-real-backend-validation.md"
text = read(path)
if "## Operator refresh API validation" not in text:
    text += make_lines([
        "",
        "## Operator refresh API validation",
        "",
        "Phase 49.26 adds `tools/validate_native_fuzzy_operator_refresh_api.py`.",
        "",
        "The validator is dry-run by default. It validates the operator refresh API contract only when called with `--execute`.",
        "",
        "Default daemon target:",
        "",
        "    http://127.0.0.1:18080",
        "",
        "Default endpoints:",
        "",
        "    /api/vdr/epgsearch/native-fuzzy/refresh",
        "    /api/epgsearch/native-fuzzy/refresh",
        "",
        "Safety expectations:",
        "",
        "- explicit operator call only",
        "- temporary SearchTimer cleanup must succeed",
        "- response contract must contain all refresh summary fields",
        "- persisted probe and backend capability update must be reported",
        "",
    ])
write(path, text)

path = "docs/development/epgsearch-test-coverage-audit.md"
text = read(path)
if "## Phase 49.26 Operator Refresh API Validation" not in text:
    text += make_lines([
        "",
        "## Phase 49.26 Operator Refresh API Validation",
        "",
        "Phase 49.26 adds a dry-run-by-default validation tool for the native fuzzy operator refresh API.",
        "",
        "Covered behavior:",
        "",
        "- validates both POST endpoint aliases",
        "- verifies required JSON response fields",
        "- verifies cleanup and persistence flags",
        "- supports expected native fuzzy availability checks",
        "",
    ])
write(path, text)

path = "docs/development/epgsearch-capability-matrix.md"
text = read(path)
if "## Phase 49.26 Operator Refresh API Validation" not in text:
    text += make_lines([
        "",
        "## Phase 49.26 Operator Refresh API Validation",
        "",
        "| Capability | Current State | Notes |",
        "| --- | --- | --- |",
        "| Operator refresh endpoint validation | Implemented | Dry-run by default, execute only with explicit operator flag. |",
        "| Alias coverage | Implemented | Covers `/api/vdr/epgsearch/native-fuzzy/refresh` and `/api/epgsearch/native-fuzzy/refresh`. |",
        "| Response contract validation | Implemented | Checks summary fields, cleanup, persistence and capability update flags. |",
    ])
write(path, text)

# Phase marker updates
phase_re = re.compile(r"Phase\s+\d+(?:\.\d+)?(?:\s*-\s*[^\n`|]*)?", re.I)
phase_files = {
    "README.md": {
        "completed": ["Latest completed implementation phase", "Latest Completed Implementation Phase"],
        "next": ["Next major implementation milestone", "Next Major Implementation Milestone", "Current Implementation Focus"],
    },
    "docs/development/current-status.md": {
        "completed": ["Latest completed implementation phase"],
        "next": ["Next Technical Focus", "Next major implementation milestone"],
    },
    "docs/project-status-dashboard.md": {
        "completed": ["Current Major Phase", "Latest completed implementation phase", "Latest Completed Milestone", "Latest Completed Milestones"],
        "next": ["Next Major Implementation Milestone", "Current Focus"],
    },
    "docs/planning/roadmap.md": {
        "completed": ["Completed implementation state"],
        "next": ["Next major implementation milestone", "Next implementation step"],
    },
    "docs/development/index.md": {
        "completed": ["Current completed phase"],
        "next": ["Next implementation focus"],
    },
}


def replace_first_phase_after_markers(text, markers, target, file_name, label):
    all_lines = text.splitlines(keepends=True)
    for index, line in enumerate(all_lines):
        if not any(marker.lower() in line.lower() for marker in markers):
            continue
        for offset in range(0, 45):
            pos = index + offset
            if pos >= len(all_lines):
                break
            match = phase_re.search(all_lines[pos])
            if match:
                all_lines[pos] = all_lines[pos][:match.start()] + target + all_lines[pos][match.end():]
                return "".join(all_lines)
        raise SystemExit(f"{file_name}: marker found but no phase found for {label}")
    raise SystemExit(f"{file_name}: marker not found for {label}")

for file_name, marker_sets in phase_files.items():
    text = read(file_name)
    text = replace_first_phase_after_markers(text, marker_sets["completed"], COMPLETED, file_name, "completed")
    text = replace_first_phase_after_markers(text, marker_sets["next"], NEXT, file_name, "next")
    write(file_name, text)

# Keep all next-focus phase mentions in current-status aligned inside the next focus area.
path = "docs/development/current-status.md"
text = read(path)
all_lines = text.splitlines(keepends=True)
for marker in ["Next Technical Focus", "Next major implementation milestone"]:
    for index, line in enumerate(all_lines):
        if marker.lower() not in line.lower():
            continue
        for offset in range(0, 35):
            pos = index + offset
            if pos >= len(all_lines):
                break
            if offset > 0 and all_lines[pos].startswith("## "):
                break
            if "Phase " in all_lines[pos]:
                all_lines[pos] = phase_re.sub(NEXT, all_lines[pos])
        break
write(path, "".join(all_lines))

path = "docs/development/completed-phases.md"
text = read(path)
entry = make_lines([
    "## Phase 49.26 - EPGSearch native fuzzy operator refresh API validation",
    "",
    "Status: Completed.",
    "",
    "Summary:",
    "- Added `tools/validate_native_fuzzy_operator_refresh_api.py`.",
    "- Validator is dry-run by default and requires `--execute` for real operator refresh API calls.",
    "- Validator checks both operator refresh endpoint aliases.",
    "- Validator checks response contract fields, cleanup, persistence and backend capability update flags.",
    "- Safety and permission contract is deferred to Phase 49.27.",
    "",
    "---",
    "",
])
if "## Phase 49.26 - EPGSearch native fuzzy operator refresh API validation" not in text:
    text = replace_once(
        text,
        "## Phase 49.25 - EPGSearch native fuzzy operator refresh API",
        entry + "## Phase 49.25 - EPGSearch native fuzzy operator refresh API",
        "insert completed phase 49.26",
    )
write(path, text)

print("Phase 49.26 API validation applied.")
