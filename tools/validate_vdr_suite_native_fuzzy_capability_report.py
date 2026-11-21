#!/usr/bin/env python3
import argparse
import json
import sys
import urllib.error
import urllib.request


def parse_args():
    parser = argparse.ArgumentParser(
        description="Validate the VDR-Suite native fuzzy capability report."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=18080)
    parser.add_argument("--base-path", default="")
    parser.add_argument("--path", default="/api/vdr/capabilities")
    parser.add_argument(
        "--capability",
        default="epg.search.fuzzy.native",
    )
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Send the capability report request.",
    )
    parser.add_argument(
        "--expect-unavailable",
        action="store_true",
        help="Expect the capability to be unavailable instead of available.",
    )
    return parser.parse_args()


def make_url(args):
    base_path = args.base_path.strip("/")
    path = args.path if args.path.startswith("/") else "/" + args.path

    if base_path:
        path = "/" + base_path + path

    return f"http://{args.host}:{args.port}{path}"


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


def find_capability(report, capability_name):
    for capability in report.get("capabilities", []):
        if capability.get("capability") == capability_name:
            return capability

    return None


def bool_text(value):
    return "true" if value else "false"


def main():
    args = parse_args()
    url = make_url(args)

    print("VDR-Suite native fuzzy capability report validation")
    print(f"target={args.host}:{args.port}")
    print(f"url={url}")
    print(f"capability={args.capability}")
    print(f"expectAvailable={bool_text(not args.expect_unavailable)}")
    print()

    if not args.execute:
        print("DRY RUN: no request sent.")
        print("Run again with --execute to read the VDR-Suite capability report.")
        return 0

    status, body = http_get_json(url, args.timeout)

    print(f"httpStatus={status}")
    print(f"responseBody={body}")

    if status != 200:
        print("FAIL: capability report did not return HTTP 200")
        return 1

    try:
        report = json.loads(body)
    except json.JSONDecodeError as error:
        print(f"FAIL: response is not valid JSON: {error}")
        return 1

    capability = find_capability(report, args.capability)

    if capability is None:
        print(f"FAIL: capability not found: {args.capability}")
        return 1

    supported = bool(capability.get("supported"))
    available_now = bool(capability.get("availableNow"))
    availability = capability.get("availability")
    reason = capability.get("reason", "")

    print()
    print(f"backendId={report.get('backendId', '')}")
    print(f"supported={bool_text(supported)}")
    print(f"availability={availability}")
    print(f"availableNow={bool_text(available_now)}")
    print(f"reason={reason}")

    expect_available = not args.expect_unavailable

    if expect_available:
        if not supported:
            print("FAIL: expected supported=true")
            return 1

        if not available_now:
            print("FAIL: expected availableNow=true")
            return 1

        if availability != "available":
            print("FAIL: expected availability=available")
            return 1

        print(f"PASS: {args.capability}=true")
        return 0

    if supported or available_now or availability == "available":
        print("FAIL: expected capability to be unavailable")
        return 1

    print(f"PASS: {args.capability}=unavailable")
    return 0


if __name__ == "__main__":
    sys.exit(main())
