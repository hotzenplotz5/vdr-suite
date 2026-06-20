#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys
from pathlib import Path


def run_curl(url):
    result = subprocess.run(
        ["curl", "-fsS", url],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        print(result.stderr.strip(), file=sys.stderr)
        raise SystemExit(result.returncode)
    return result.stdout


def load_json(text):
    try:
        return json.loads(text)
    except json.JSONDecodeError as error:
        print(f"invalid JSON: {error}", file=sys.stderr)
        raise SystemExit(1)


def object_keys(payload):
    timers = payload.get("searchtimers", [])
    keys = set()
    if isinstance(timers, list):
        for item in timers:
            if isinstance(item, dict):
                keys.update(item.keys())
    return sorted(keys)


def main():
    parser = argparse.ArgumentParser(
        description="Capture and summarize RESTfulAPI SearchTimer payloads."
    )
    parser.add_argument(
        "--url",
        default="http://127.0.0.1:8002/searchtimers.json",
        help="RESTfulAPI SearchTimer endpoint URL",
    )
    parser.add_argument(
        "--out",
        default="docs/development/validation/searchtimer-real-payload.json",
        help="Output path for captured JSON",
    )
    args = parser.parse_args()

    raw = run_curl(args.url)
    payload = load_json(raw)

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    timers = payload.get("searchtimers", [])
    timer_count = len(timers) if isinstance(timers, list) else 0

    print(f"captured: {out}")
    print(f"searchtimer_count: {timer_count}")
    print("object_keys:")
    for key in object_keys(payload):
        print(f"- {key}")


if __name__ == "__main__":
    main()
