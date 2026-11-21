#!/usr/bin/env python3
import argparse
import base64
import json
import sys
import time
import urllib.error
import urllib.request


def normalize_base_path(value):
    if not value:
        return ""
    value = value.strip()
    if not value:
        return ""
    if not value.startswith("/"):
        value = "/" + value
    return value.rstrip("/")


def build_url(host, port, base_path, path):
    return f"http://{host}:{port}{normalize_base_path(base_path)}{path}"


def auth_header(user, password):
    if not user:
        return None

    token = f"{user}:{password or ''}".encode("utf-8")
    return "Basic " + base64.b64encode(token).decode("ascii")


def request_http(method, url, payload=None, user=None, password=None, timeout=5):
    data = None
    headers = {
        "Accept": "application/json, text/plain",
    }

    if payload is not None:
        data = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        headers["Content-Type"] = "application/json"

    auth = auth_header(user, password)
    if auth:
        headers["Authorization"] = auth

    request = urllib.request.Request(
        url,
        data=data,
        headers=headers,
        method=method,
    )

    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            body = response.read().decode("utf-8", errors="replace")
            return response.getcode(), body
    except urllib.error.HTTPError as error:
        body = error.read().decode("utf-8", errors="replace")
        return error.code, body
    except urllib.error.URLError as error:
        return 0, str(error)


def parse_json_object(body):
    try:
        parsed = json.loads(body)
    except json.JSONDecodeError:
        return None, "response is not valid JSON"

    if not isinstance(parsed, dict):
        return None, "response JSON is not an object"

    return parsed, ""


def bool_value(data, key):
    return bool(data.get(key, False))


def print_field(data, key):
    value = data.get(key)
    if isinstance(value, bool):
        value = "true" if value else "false"
    print(f"{key}={value}")


def validate_summary(data, expected_backend, expected_tolerance):
    ok = True

    required_true_fields = [
        "backendKnown",
        "createAttempted",
        "createAccepted",
        "readbackAvailable",
        "modePreserved",
        "tolerancePreserved",
        "cleanupAttempted",
        "cleanupSucceeded",
        "persisted",
        "nativeFuzzyAvailable",
    ]

    for field in [
        "status",
        "backendId",
        "backendNativeId",
        "probeQuery",
        "tolerance",
        "backendKnown",
        "createAttempted",
        "createAccepted",
        "readbackAvailable",
        "modePreserved",
        "tolerancePreserved",
        "cleanupAttempted",
        "cleanupSucceeded",
        "persisted",
        "backendCapabilitiesUpdated",
        "nativeFuzzyAvailable",
        "warnings",
        "errors",
    ]:
        print_field(data, field)

    if data.get("backendId") != expected_backend:
        print(
            "FAIL: expected backendId="
            + expected_backend
            + ", got "
            + str(data.get("backendId"))
        )
        ok = False

    if int(data.get("tolerance", -1)) != expected_tolerance:
        print(
            "FAIL: expected tolerance="
            + str(expected_tolerance)
            + ", got "
            + str(data.get("tolerance"))
        )
        ok = False

    for field in required_true_fields:
        if not bool_value(data, field):
            print(f"FAIL: expected {field}=true")
            ok = False
        else:
            print(f"PASS: {field}=true")

    if data.get("status") != "refreshed-native-available":
        print(
            "FAIL: expected status=refreshed-native-available, got "
            + str(data.get("status"))
        )
        ok = False
    else:
        print("PASS: status=refreshed-native-available")

    errors = data.get("errors", [])
    if errors:
        print("FAIL: response contains errors")
        ok = False

    return ok


def run_validation(args):
    url = build_url(
        args.host,
        args.port,
        args.base_path,
        args.path,
    )

    probe_query = args.query
    if args.unique_query:
        probe_query = f"{probe_query} {int(time.time())}"

    payload = {
        "backendId": args.backend,
        "probeQuery": probe_query,
        "tolerance": args.tolerance,
        "keepProbeSearchTimer": args.keep_probe_searchtimer,
        "updateBackendCapabilities": args.update_backend_capabilities,
    }

    print("VDR-Suite native fuzzy operator refresh validation")
    print(f"target={args.host}:{args.port}{normalize_base_path(args.base_path)}")
    print(f"url={url}")
    print(f"backendId={args.backend}")
    print(f"probeQuery={probe_query}")
    print(f"tolerance={args.tolerance}")
    print(f"keepProbeSearchTimer={args.keep_probe_searchtimer}")
    print(f"updateBackendCapabilities={args.update_backend_capabilities}")

    if not args.execute:
        print("")
        print("DRY RUN: no request sent.")
        print("Run again with --execute to call the VDR-Suite operator refresh API.")
        print("")
        print("Payload preview:")
        print(json.dumps(payload, indent=2, sort_keys=True))
        return 0

    status, body = request_http(
        "POST",
        url,
        payload,
        args.user,
        args.password,
        args.timeout,
    )

    print("")
    print(f"httpStatus={status}")
    print(f"responseBody={body}")

    if status != 200:
        print("FAIL: operator refresh request did not return HTTP 200")
        return 1

    data, error = parse_json_object(body)
    if data is None:
        print("FAIL: " + error)
        return 1

    print("")
    ok = validate_summary(data, args.backend, args.tolerance)

    if not ok:
        return 1

    print("")
    print("capability: epg.search.fuzzy.native=true")
    print("PASS: VDR-Suite native fuzzy operator refresh validation completed")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Validate the VDR-Suite native fuzzy operator refresh API endpoint."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=18080)
    parser.add_argument("--base-path", default="")
    parser.add_argument(
        "--path",
        default="/api/epgsearch/native-fuzzy/refresh",
        choices=[
            "/api/epgsearch/native-fuzzy/refresh",
            "/api/vdr/epgsearch/native-fuzzy/refresh",
        ],
    )
    parser.add_argument("--backend", default="default")
    parser.add_argument(
        "--query",
        default="VDR-Suite Operator Native Fuzzy Validation",
    )
    parser.add_argument("--unique-query", action="store_true")
    parser.add_argument("--tolerance", type=int, default=2)
    parser.add_argument("--user", default="")
    parser.add_argument("--password", default="")
    parser.add_argument("--timeout", type=int, default=5)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--keep-probe-searchtimer", action="store_true")
    parser.add_argument(
        "--no-update-backend-capabilities",
        dest="update_backend_capabilities",
        action="store_false",
    )
    parser.set_defaults(update_backend_capabilities=True)

    args = parser.parse_args()
    return run_validation(args)


if __name__ == "__main__":
    raise SystemExit(main())
