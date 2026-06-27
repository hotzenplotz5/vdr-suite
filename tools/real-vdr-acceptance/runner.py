#!/usr/bin/env python3
import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from urllib.parse import urljoin

MANIFEST_PATH = Path(__file__).with_name("manifest.json")
REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
API_ROUTER_PATH = REPOSITORY_ROOT / "api" / "rest" / "src" / "ApiRouter.cpp"

RISK_ORDER = {
    "safe": 0,
    "dry-run": 1,
    "destructive": 2,
}


def load_manifest(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def route_path(path_with_query: str) -> str:
    return path_with_query.split("?", 1)[0]


def validate_manifest_routes(manifest: dict) -> list[str]:
    errors: list[str] = []

    if not API_ROUTER_PATH.exists():
        return [f"ApiRouter.cpp not found: {API_ROUTER_PATH}"]

    router_source = API_ROUTER_PATH.read_text(encoding="utf-8")

    for probe in manifest.get("probes", []):
        if not isinstance(probe, dict):
            continue

        path = probe.get("path")
        probe_id = probe.get("id", "<unknown>")
        if not isinstance(path, str):
            continue

        route = route_path(path)
        quoted_route = f'"{route}"'

        if quoted_route not in router_source:
            errors.append(
                f"probe {probe_id} references route {route}, "
                "but that route is not present in api/rest/src/ApiRouter.cpp")

    return errors


def validate_manifest(manifest: dict) -> list[str]:
    errors: list[str] = []

    if manifest.get("schemaVersion") != 1:
        errors.append("schemaVersion must be 1")

    defaults = manifest.get("defaults")
    if not isinstance(defaults, dict):
        errors.append("defaults must be an object")
        defaults = {}

    allowed_risks = defaults.get("allowedRiskLevels", [])
    if allowed_risks != ["safe", "dry-run", "destructive"]:
        errors.append("defaults.allowedRiskLevels must be ['safe', 'dry-run', 'destructive']")

    probes = manifest.get("probes")
    if not isinstance(probes, list) or not probes:
        errors.append("probes must be a non-empty array")
        return errors

    seen_ids: set[str] = set()
    for index, probe in enumerate(probes):
        prefix = f"probes[{index}]"
        if not isinstance(probe, dict):
            errors.append(f"{prefix} must be an object")
            continue

        probe_id = probe.get("id")
        if not isinstance(probe_id, str) or not probe_id:
            errors.append(f"{prefix}.id must be a non-empty string")
        elif probe_id in seen_ids:
            errors.append(f"duplicate probe id: {probe_id}")
        else:
            seen_ids.add(probe_id)

        if not isinstance(probe.get("title"), str) or not probe.get("title"):
            errors.append(f"{prefix}.title must be a non-empty string")

        if probe.get("risk") not in RISK_ORDER:
            errors.append(f"{prefix}.risk must be one of safe, dry-run, destructive")

        if probe.get("method") not in ("GET", "POST"):
            errors.append(f"{prefix}.method must be GET or POST")

        path = probe.get("path")
        if not isinstance(path, str) or not path.startswith("/api/"):
            errors.append(f"{prefix}.path must start with /api/")

        expected_status = probe.get("expectedStatus")
        if not isinstance(expected_status, int) or expected_status < 100 or expected_status > 599:
            errors.append(f"{prefix}.expectedStatus must be an HTTP status code")

        required_json_keys = probe.get("requiredJsonKeys")
        if not isinstance(required_json_keys, list):
            errors.append(f"{prefix}.requiredJsonKeys must be an array")
        elif not all(isinstance(item, str) for item in required_json_keys):
            errors.append(f"{prefix}.requiredJsonKeys must contain strings only")

        expected_json_values = probe.get("expectedJsonValues", {})
        if not isinstance(expected_json_values, dict):
            errors.append(f"{prefix}.expectedJsonValues must be an object when provided")
        elif not all(isinstance(key, str) for key in expected_json_values.keys()):
            errors.append(f"{prefix}.expectedJsonValues keys must be strings")

        if probe.get("risk") == "destructive" and not probe.get("destructiveConfirmation"):
            errors.append(f"{prefix}.destructiveConfirmation is required for destructive probes")

    errors.extend(validate_manifest_routes(manifest))

    return errors


def selected_probes(manifest: dict, max_risk: str) -> list[dict]:
    allowed_level = RISK_ORDER[max_risk]
    return [
        probe
        for probe in manifest["probes"]
        if RISK_ORDER[probe["risk"]] <= allowed_level
    ]


def probe_url(base_url: str, path: str) -> str:
    return urljoin(base_url.rstrip("/") + "/", path.lstrip("/"))


def execute_probe(base_url: str, timeout: int, probe: dict) -> dict:
    started = time.monotonic()
    url = probe_url(base_url, probe["path"])
    method = probe["method"]
    data = None
    headers = {"Accept": "application/json"}

    if method == "POST":
        data = json.dumps(probe.get("body", {})).encode("utf-8")
        headers["Content-Type"] = "application/json"

    request = urllib.request.Request(
        url,
        data=data,
        headers=headers,
        method=method)

    status = None
    body = ""
    error_message = ""

    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            status = response.status
            body = response.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as error:
        status = error.code
        body = error.read().decode("utf-8", errors="replace")
    except Exception as error:
        error_message = f"request failed: {error}"

    duration_ms = int((time.monotonic() - started) * 1000)

    result = {
        "id": probe["id"],
        "title": probe["title"],
        "risk": probe["risk"],
        "method": method,
        "path": probe["path"],
        "route": route_path(probe["path"]),
        "expectedStatus": probe["expectedStatus"],
        "status": status,
        "durationMs": duration_ms,
        "passed": False,
        "message": "",
    }

    if error_message:
        result["message"] = error_message
        return result

    if status != probe["expectedStatus"]:
        result["message"] = f"expected HTTP {probe['expectedStatus']}, got {status}"
        return result

    try:
        decoded = json.loads(body)
    except json.JSONDecodeError as error:
        result["message"] = f"response is not valid JSON: {error}"
        return result

    if not isinstance(decoded, dict):
        result["message"] = "response JSON must be an object"
        return result

    for key in probe.get("requiredJsonKeys", []):
        if key not in decoded:
            result["message"] = f"missing JSON key '{key}'"
            return result

    for key, expected_value in probe.get("expectedJsonValues", {}).items():
        if key not in decoded:
            result["message"] = f"missing expected JSON value key '{key}'"
            return result

        actual_value = decoded[key]
        if actual_value != expected_value:
            result["message"] = (
                f"JSON value mismatch for '{key}': "
                f"expected {expected_value!r}, got {actual_value!r}")
            return result

    result["passed"] = True
    result["message"] = "ok"
    return result


def write_report(path: str, report: dict) -> None:
    report_path = Path(path)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n",
        encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run or validate VDR-Suite real VDR acceptance probes.")
    parser.add_argument(
        "--manifest",
        default=str(MANIFEST_PATH),
        help="Path to the acceptance manifest")
    parser.add_argument(
        "--validate-only",
        action="store_true",
        help="Validate manifest structure without contacting a daemon")
    parser.add_argument(
        "--max-risk",
        choices=["safe", "dry-run", "destructive"],
        default="safe",
        help="Maximum probe risk level to execute")
    parser.add_argument(
        "--base-url",
        default="",
        help="Daemon base URL. Defaults to env var from manifest or manifest defaultBaseUrl")
    parser.add_argument(
        "--report-json",
        default="",
        help="Optional path for a machine-readable JSON acceptance report")
    args = parser.parse_args()

    manifest = load_manifest(Path(args.manifest))
    errors = validate_manifest(manifest)
    if errors:
        print("Real VDR acceptance manifest validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("Real VDR acceptance manifest validation passed.")

    if args.validate_only:
        return 0

    defaults = manifest["defaults"]
    base_url = (
        args.base_url
        or os.environ.get(defaults["baseUrlEnvironment"], "")
        or defaults["defaultBaseUrl"])
    timeout = int(defaults.get("timeoutSeconds", 5))
    probes = selected_probes(manifest, args.max_risk)

    started = time.monotonic()
    results = []

    for probe in probes:
        result = execute_probe(base_url, timeout, probe)
        results.append(result)
        print(f"{result['id']}: {result['message']}")

    passed = sum(1 for result in results if result["passed"])
    failed = len(results) - passed
    duration_ms = int((time.monotonic() - started) * 1000)

    report = {
        "schemaVersion": 1,
        "manifestName": manifest.get("name", ""),
        "baseUrl": base_url,
        "maxRisk": args.max_risk,
        "total": len(results),
        "passed": passed,
        "failed": failed,
        "durationMs": duration_ms,
        "results": results,
    }

    if args.report_json:
        write_report(args.report_json, report)
        print(f"Real VDR acceptance JSON report written: {args.report_json}")

    if failed:
        print(f"Real VDR acceptance failed: {failed} probe(s) failed.")
        return 1

    print("Real VDR acceptance passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
