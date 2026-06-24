#!/usr/bin/env python3

import argparse
import json
import os
import sys
import urllib.error
import urllib.request


DEFAULT_ENDPOINT = "/api/searchtimers/real-test"


def make_payload(args):
    payload = {
        "operation": args.operation,
        "backendId": args.backend,
        "name": args.name,
        "query": args.query,
        "active": args.active,
        "executionMode": "execute",
        "explicitOperatorConfirmation": True,
        "executorOptIn": True,
    }

    if args.backend_native_id:
        payload["backendNativeId"] = args.backend_native_id

    return payload


def post_json(base_url, endpoint, payload, timeout_seconds):
    url = base_url.rstrip("/") + endpoint
    body = json.dumps(payload).encode("utf-8")

    request = urllib.request.Request(
        url,
        data=body,
        method="POST",
        headers={
            "Content-Type": "application/json",
            "Accept": "application/json",
        },
    )

    try:
        with urllib.request.urlopen(request, timeout=timeout_seconds) as response:
            response_body = response.read().decode("utf-8", errors="replace")
            return response.status, response_body
    except urllib.error.HTTPError as error:
        response_body = error.read().decode("utf-8", errors="replace")
        return error.code, response_body


def check(condition, name, details=""):
    return {
        "name": name,
        "passed": bool(condition),
        "details": details,
    }


def validate_response(status_code, response_text):
    checks = []

    checks.append(check(status_code == 200, "HTTP 200", f"status={status_code}"))

    try:
        data = json.loads(response_text)
        checks.append(check(True, "JSON parse"))
    except json.JSONDecodeError as error:
        checks.append(check(False, "JSON parse", str(error)))
        return checks, None

    checks.append(check(data.get("success") is False, "success=false"))
    checks.append(check(data.get("executed") is False, "executed=false"))
    checks.append(check(data.get("blocked") is True, "blocked=true"))
    checks.append(check(data.get("dryRunOnly") is True, "dryRunOnly=true"))
    checks.append(check(data.get("executionMode") == "execute", "executionMode=execute"))
    checks.append(check(data.get("executorOptInProvided") is True, "executorOptInProvided=true"))
    checks.append(check(data.get("executorInjected") is True, "executorInjected=true"))
    checks.append(check(data.get("executorInvocationAttempted") is False, "executorInvocationAttempted=false"))

    dispatch_stage = data.get("dispatchStage", "")
    checks.append(
        check(
            dispatch_stage == "production-policy-gate-closed",
            "production policy gate closed",
            f"dispatchStage={dispatch_stage}",
        )
    )

    warnings = data.get("warnings", [])
    warning_text = "\n".join(warnings)

    checks.append(
        check(
            "yaVDR real-test mode: no real backend mutation is performed" in warning_text,
            "operator warning: no mutation",
        )
    )

    audit_trail = data.get("executorInvocationAuditTrail", [])
    audit_text = "\n".join(audit_trail)

    checks.append(
        check(
            "policyStage=production-policy-gate-closed" in audit_text,
            "audit trail: final policy stage",
        )
    )

    return checks, data


def print_report(title, base_url, endpoint, payload, status_code, checks, data, print_json):
    print(title)
    print("=" * len(title))
    print(f"baseUrl={base_url}")
    print(f"endpoint={endpoint}")
    print(f"operation={payload.get('operation')}")
    print(f"backendId={payload.get('backendId')}")
    print(f"httpStatus={status_code}")
    print()

    all_passed = True

    for item in checks:
        all_passed = all_passed and item["passed"]
        prefix = "PASS" if item["passed"] else "FAIL"
        line = f"{prefix} {item['name']}"

        if item["details"]:
            line += f" - {item['details']}"

        print(line)

    if data is not None:
        print()
        print(f"dispatchStage={data.get('dispatchStage', '')}")
        print(f"message={data.get('message', '')}")

        warnings = data.get("warnings", [])
        if warnings:
            print()
            print("warnings:")
            for warning in warnings:
                print(f"- {warning}")

        audit_trail = data.get("executorInvocationAuditTrail", [])
        if audit_trail:
            print()
            print("auditTrail:")
            for entry in audit_trail:
                print(f"- {entry}")

    if print_json and data is not None:
        print()
        print("json:")
        print(json.dumps(data, indent=2, sort_keys=True))

    print()
    print("RESULT: " + ("OK" if all_passed else "FAIL"))

    return 0 if all_passed else 2


def run_self_test():
    payload = {
        "operation": "create",
        "backendId": "home-vdr",
    }

    sample = {
        "success": False,
        "executed": False,
        "blocked": True,
        "dryRunOnly": True,
        "executionMode": "execute",
        "operation": "create",
        "backendId": "home-vdr",
        "executorOptInProvided": True,
        "executorInjected": True,
        "executorInvocationAttempted": False,
        "dispatchStage": "production-policy-gate-closed",
        "message": "production policy gate is closed for real backend mutation",
        "warnings": [
            "yaVDR real-test mode: no real backend mutation is performed",
            "yaVDR real-test mode: result shows the final production gate or blocker",
        ],
        "executorInvocationAuditTrail": [
            "policyStage=production-policy-gate-closed",
            "executorInvocationAttempted=false",
        ],
    }

    checks, data = validate_response(200, json.dumps(sample))

    return print_report(
        "SearchTimer yaVDR real-test smoke self-test",
        "self-test",
        DEFAULT_ENDPOINT,
        payload,
        200,
        checks,
        data,
        False,
    )


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Run a non-mutating SearchTimer yaVDR real-test smoke check against VDR-Suite."
    )

    parser.add_argument(
        "--run",
        action="store_true",
        help="Actually contact the configured VDR-Suite API endpoint.",
    )
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="Validate the local response checks without contacting a server.",
    )
    parser.add_argument(
        "--base-url",
        default=os.environ.get("VDR_SUITE_API_BASE_URL", "http://127.0.0.1:8080"),
        help="VDR-Suite API base URL. Default: env VDR_SUITE_API_BASE_URL or http://127.0.0.1:8080",
    )
    parser.add_argument(
        "--endpoint",
        default=os.environ.get("VDR_SUITE_SEARCHTIMER_REAL_TEST_ENDPOINT", DEFAULT_ENDPOINT),
        help="Real-test endpoint path.",
    )
    parser.add_argument(
        "--backend",
        default=os.environ.get("VDR_SUITE_SEARCHTIMER_TEST_BACKEND", "home-vdr"),
        help="Backend id to put into the test workflow body.",
    )
    parser.add_argument(
        "--operation",
        choices=["create", "update", "delete"],
        default=os.environ.get("VDR_SUITE_SEARCHTIMER_TEST_OPERATION", "create"),
        help="Workflow operation to simulate.",
    )
    parser.add_argument(
        "--backend-native-id",
        default=os.environ.get("VDR_SUITE_SEARCHTIMER_TEST_NATIVE_ID", ""),
        help="Backend native id for update/delete simulations.",
    )
    parser.add_argument(
        "--name",
        default=os.environ.get("VDR_SUITE_SEARCHTIMER_TEST_NAME", "VDR-Suite yaVDR Real-Test"),
        help="SearchTimer name for create/update simulations.",
    )
    parser.add_argument(
        "--query",
        default=os.environ.get("VDR_SUITE_SEARCHTIMER_TEST_QUERY", "Terra X"),
        help="SearchTimer query for create/update simulations.",
    )
    parser.add_argument(
        "--inactive",
        action="store_true",
        help="Send active=false in create/update simulations.",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=int(os.environ.get("VDR_SUITE_SMOKE_TIMEOUT", "10")),
        help="HTTP timeout in seconds.",
    )
    parser.add_argument(
        "--print-json",
        action="store_true",
        help="Print parsed response JSON.",
    )

    args = parser.parse_args(argv)
    args.active = not args.inactive
    return args


def main(argv):
    args = parse_args(argv)

    if args.self_test:
        return run_self_test()

    if not args.run:
        print("Refusing to contact VDR-Suite without --run.")
        print("Use --self-test for local validation or --run to contact the configured API.")
        return 1

    if args.operation in ("update", "delete") and not args.backend_native_id:
        print("--backend-native-id is required for update/delete simulations.", file=sys.stderr)
        return 1

    payload = make_payload(args)
    status_code, response_text = post_json(
        args.base_url,
        args.endpoint,
        payload,
        args.timeout,
    )

    checks, data = validate_response(status_code, response_text)

    return print_report(
        "SearchTimer yaVDR real-test smoke",
        args.base_url,
        args.endpoint,
        payload,
        status_code,
        checks,
        data,
        args.print_json,
    )


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
