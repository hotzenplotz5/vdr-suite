#!/usr/bin/env python3
import argparse
import json
import sys
import urllib.error
import urllib.request


def parse_args():
    parser = argparse.ArgumentParser(
        description="Validate persisted native fuzzy restore after a VDR-Suite daemon restart."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=18080)
    parser.add_argument("--base-path", default="")
    parser.add_argument("--runtime-path", default="/api/runtime/diagnostics")
    parser.add_argument("--capabilities-path", default="/api/vdr/capabilities")
    parser.add_argument("--capability", default="epg.search.fuzzy.native")
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument(
        "--min-restore-items",
        type=int,
        default=1,
        help="Minimum startup-restore persisted result count expected after restart.",
    )
    parser.add_argument(
        "--skip-runtime-diagnostics",
        action="store_true",
        help="Only validate the capability report.",
    )
    return parser.parse_args()


def make_url(args, path):
    base_path = args.base_path.strip("/")
    resolved_path = path if path.startswith("/") else "/" + path

    if base_path:
        resolved_path = "/" + base_path + resolved_path

    return f"http://{args.host}:{args.port}{resolved_path}"


def http_get_json(url, timeout):
    request = urllib.request.Request(
        url,
        method="GET",
        headers={"Accept": "application/json"},
    )

    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            body = response.read().decode("utf-8", errors="replace")
            return response.status, body
    except urllib.error.HTTPError as error:
        body = error.read().decode("utf-8", errors="replace")
        return error.code, body
    except urllib.error.URLError as error:
        return 0, str(error)


def load_json_response(url, timeout, label):
    status, body = http_get_json(url, timeout)

    print(f"{label}HttpStatus={status}")
    print(f"{label}ResponseBody={body}")

    if status != 200:
        print(f"FAIL: {label} did not return HTTP 200")
        return None

    try:
        return json.loads(body)
    except json.JSONDecodeError as error:
        print(f"FAIL: {label} response is not valid JSON: {error}")
        return None


def find_startup_restore_measurement(diagnostics):
    for measurement in diagnostics.get("measurements", []):
        if (
            measurement.get("component") == "epgsearch-native-fuzzy"
            and measurement.get("operation") == "startup-restore"
        ):
            return measurement

    return None


def find_capability(report, capability_name):
    for capability in report.get("capabilities", []):
        if capability.get("capability") == capability_name:
            return capability

    return None


def bool_text(value):
    return "true" if value else "false"


def validate_runtime_diagnostics(args):
    url = make_url(args, args.runtime_path)
    print(f"runtimeUrl={url}")

    diagnostics = load_json_response(
        url,
        args.timeout,
        "runtime",
    )

    if diagnostics is None:
        return False

    measurement = find_startup_restore_measurement(diagnostics)

    if measurement is None:
        print("FAIL: startup restore runtime measurement not found")
        return False

    status_code = int(measurement.get("statusCode", 0))
    item_count = int(measurement.get("itemCount", 0))

    print()
    print("startupRestoreMeasurement:")
    print(f"component={measurement.get('component', '')}")
    print(f"operation={measurement.get('operation', '')}")
    print(f"statusCode={status_code}")
    print(f"itemCount={item_count}")

    if item_count < args.min_restore_items:
        print(
            "FAIL: startup restore did not report enough persisted results; "
            "restart the daemon after a successful operator refresh"
        )
        return False

    if status_code != 200:
        print("FAIL: startup restore did not report statusCode=200")
        return False

    print("PASS: persisted startup restore measurement found")
    return True


def validate_capability_report(args):
    url = make_url(args, args.capabilities_path)
    print(f"capabilitiesUrl={url}")

    report = load_json_response(
        url,
        args.timeout,
        "capabilities",
    )

    if report is None:
        return False

    capability = find_capability(report, args.capability)

    if capability is None:
        print(f"FAIL: capability not found: {args.capability}")
        return False

    supported = bool(capability.get("supported"))
    available_now = bool(capability.get("availableNow"))
    availability = capability.get("availability")
    reason = capability.get("reason", "")

    print()
    print(f"backendId={report.get('backendId', '')}")
    print(f"capability={args.capability}")
    print(f"supported={bool_text(supported)}")
    print(f"availability={availability}")
    print(f"availableNow={bool_text(available_now)}")
    print(f"reason={reason}")

    if not supported:
        print("FAIL: expected supported=true")
        return False

    if not available_now:
        print("FAIL: expected availableNow=true")
        return False

    if availability != "available":
        print("FAIL: expected availability=available")
        return False

    print(f"PASS: {args.capability}=true")
    return True


def main():
    args = parse_args()

    print("VDR-Suite native fuzzy persisted restore validation")
    print(f"target={args.host}:{args.port}")
    print(f"capability={args.capability}")
    print(f"minRestoreItems={args.min_restore_items}")
    print()

    if not args.execute:
        print("DRY RUN: no request sent.")
        print("Expected workflow:")
        print("1. run operator refresh successfully")
        print("2. stop the daemon")
        print("3. start the daemon again with the same database")
        print("4. run this helper with --execute")
        return 0

    if not args.skip_runtime_diagnostics:
        if not validate_runtime_diagnostics(args):
            return 1
        print()

    if not validate_capability_report(args):
        return 1

    print()
    print("PASS: persisted native fuzzy restore validation completed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
