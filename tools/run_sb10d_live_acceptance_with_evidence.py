#!/usr/bin/env python3

import argparse
import base64
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools/run_sb10d_live_acceptance.py"


class EvidenceError(RuntimeError):
    pass


def request_json(
    url: str,
    *,
    username: str = "",
    password: str = "",
    timeout: int = 30,
) -> Any:
    headers = {"Accept": "application/json"}

    if username:
        token = base64.b64encode(
            f"{username}:{password}".encode("utf-8")
        ).decode("ascii")
        headers["Authorization"] = f"Basic {token}"

    request = urllib.request.Request(url, headers=headers, method="GET")

    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            if response.status != 200:
                raise EvidenceError(
                    f"expected HTTP 200 from {url}, got {response.status}"
                )
            payload = response.read().decode("utf-8", errors="strict")
    except urllib.error.HTTPError as error:
        raise EvidenceError(
            f"HTTP request failed for {url}: {error.code}"
        ) from error
    except Exception as error:
        raise EvidenceError(f"request failed for {url}: {error}") from error

    try:
        return json.loads(payload)
    except json.JSONDecodeError as error:
        raise EvidenceError(f"invalid JSON from {url}: {error}") from error


def wait_json(
    url: str,
    *,
    username: str = "",
    password: str = "",
    timeout_seconds: int = 60,
) -> Any:
    deadline = time.monotonic() + timeout_seconds
    last_error = ""

    while time.monotonic() < deadline:
        try:
            return request_json(
                url,
                username=username,
                password=password,
                timeout=10,
            )
        except EvidenceError as error:
            last_error = str(error)
            time.sleep(0.5)

    raise EvidenceError(f"JSON endpoint did not recover: {url}: {last_error}")


def write_json(path: Path, value: Any) -> None:
    path.write_text(
        json.dumps(value, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def capture_payloads(
    base_url: str,
    username: str,
    password: str,
) -> dict[str, Any]:
    endpoints = {
        "channels": "/channels.json",
        "timers": "/timers.json",
        "recordings": "/recordings.json",
    }
    payloads: dict[str, Any] = {}

    for name, endpoint in endpoints.items():
        payloads[name] = wait_json(
            base_url.rstrip("/") + endpoint,
            username=username,
            password=password,
            timeout_seconds=60,
        )

    return payloads


def json_difference(
    before: Any,
    after: Any,
    path: str = "$",
) -> list[dict[str, Any]]:
    differences: list[dict[str, Any]] = []

    if type(before) is not type(after):
        return [
            {
                "path": path,
                "kind": "type-changed",
                "beforeType": type(before).__name__,
                "afterType": type(after).__name__,
                "before": before,
                "after": after,
            }
        ]

    if isinstance(before, dict):
        before_keys = set(before)
        after_keys = set(after)

        for key in sorted(before_keys - after_keys):
            differences.append(
                {
                    "path": f"{path}.{key}",
                    "kind": "removed",
                    "before": before[key],
                }
            )

        for key in sorted(after_keys - before_keys):
            differences.append(
                {
                    "path": f"{path}.{key}",
                    "kind": "added",
                    "after": after[key],
                }
            )

        for key in sorted(before_keys & after_keys):
            differences.extend(
                json_difference(
                    before[key],
                    after[key],
                    f"{path}.{key}",
                )
            )

        return differences

    if isinstance(before, list):
        shared_length = min(len(before), len(after))

        for index in range(shared_length):
            differences.extend(
                json_difference(
                    before[index],
                    after[index],
                    f"{path}[{index}]",
                )
            )

        for index in range(shared_length, len(before)):
            differences.append(
                {
                    "path": f"{path}[{index}]",
                    "kind": "removed",
                    "before": before[index],
                }
            )

        for index in range(shared_length, len(after)):
            differences.append(
                {
                    "path": f"{path}[{index}]",
                    "kind": "added",
                    "after": after[index],
                }
            )

        return differences

    if before != after:
        differences.append(
            {
                "path": path,
                "kind": "changed",
                "before": before,
                "after": after,
            }
        )

    return differences


def persist_evidence(
    evidence: Path,
    before_payloads: dict[str, Any],
    after_payloads: dict[str, Any],
    runner_exit_code: int,
) -> list[str]:
    evidence.mkdir(parents=True, exist_ok=True)
    changed_resources: list[str] = []

    for name in ("channels", "timers", "recordings"):
        before = before_payloads[name]
        after = after_payloads[name]
        differences = json_difference(before, after)

        write_json(evidence / f"{name}-before.json", before)
        write_json(evidence / f"{name}-after.json", after)
        write_json(
            evidence / f"{name}-diff.json",
            {
                "resource": name,
                "changed": bool(differences),
                "differenceCount": len(differences),
                "differences": differences,
            },
        )

        if differences:
            changed_resources.append(name)

    write_json(
        evidence / "vdr-state-evidence-summary.json",
        {
            "runnerExitCode": runner_exit_code,
            "changedResources": changed_resources,
            "vdrStateUnchanged": not changed_resources,
        },
    )

    return changed_resources


def parse_wrapper_arguments() -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--restfulapi-base-url",
        default="http://127.0.0.1:8002",
    )
    parser.add_argument("--restfulapi-user", default="")
    parser.add_argument("--restfulapi-password", default="")
    parser.add_argument("--evidence-dir", default="")
    return parser.parse_known_args()


def main() -> int:
    args, passthrough = parse_wrapper_arguments()

    timestamp = time.strftime("%Y%m%d-%H%M%S")
    evidence = Path(args.evidence_dir) if args.evidence_dir else Path(
        f"/tmp/vdr-suite-sb10d-live-{timestamp}"
    )

    if evidence.exists():
        raise EvidenceError(f"evidence directory already exists: {evidence}")

    before_payloads = capture_payloads(
        args.restfulapi_base_url,
        args.restfulapi_user,
        args.restfulapi_password,
    )

    command = [
        sys.executable,
        str(RUNNER),
        "--restfulapi-base-url",
        args.restfulapi_base_url,
        "--restfulapi-user",
        args.restfulapi_user,
        "--restfulapi-password",
        args.restfulapi_password,
        "--evidence-dir",
        str(evidence),
        *passthrough,
    ]

    completed = subprocess.run(
        command,
        cwd=ROOT,
        env=os.environ.copy(),
        check=False,
    )

    if not evidence.exists():
        evidence.mkdir(parents=True)

    try:
        after_payloads = capture_payloads(
            args.restfulapi_base_url,
            args.restfulapi_user,
            args.restfulapi_password,
        )
    except Exception as error:
        write_json(
            evidence / "vdr-state-evidence-summary.json",
            {
                "runnerExitCode": completed.returncode,
                "evidenceError": str(error),
                "vdrStateUnchanged": None,
            },
        )
        raise

    changed_resources = persist_evidence(
        evidence,
        before_payloads,
        after_payloads,
        completed.returncode,
    )

    print(f"SB.10d extended evidence: {evidence}", flush=True)
    if changed_resources:
        print(
            "VDR state differences: " + ", ".join(changed_resources),
            flush=True,
        )

    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
