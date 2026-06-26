#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
import time


DEFAULT_REPO = "hotzenplotz5/vdr-suite"
DEFAULT_BRANCH = "main"
DEFAULT_WORKFLOW = "VDR-Suite CI"
DEFAULT_INTERVAL = 60


def run_json(command):
    result = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )

    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "command failed")

    return json.loads(result.stdout)


def latest_commit(repo, branch):
    data = run_json(
        [
            "gh",
            "api",
            f"repos/{repo}/commits/{branch}",
        ]
    )

    sha = data.get("sha", "")
    if not sha:
        raise RuntimeError("commit sha not found")

    return sha


def latest_run(repo, branch, sha, workflow):
    data = run_json(
        [
            "gh",
            "run",
            "list",
            "--repo",
            repo,
            "--branch",
            branch,
            "--commit",
            sha,
            "--event",
            "push",
            "--limit",
            "20",
            "--json",
            "databaseId,name,status,conclusion,url,headSha",
        ]
    )

    for run in data:
        if workflow and run.get("name") != workflow:
            continue
        return run

    return None


def status_message(sha, run):
    short_sha = sha[:8]

    if run is None:
        return f"⏳ CI läuft noch für {short_sha} (noch kein Run gefunden)", 2

    status = run.get("status", "")
    conclusion = run.get("conclusion")

    if status in ("queued", "in_progress", "waiting", "requested", "pending"):
        return f"⏳ CI läuft noch für {short_sha}", 2

    if status == "completed" and conclusion == "success":
        return f"✅ CI grün für {short_sha}", 0

    if status == "completed":
        return f"❌ CI rot für {short_sha} ({conclusion or 'unknown'})", 1

    return f"⏳ CI läuft noch für {short_sha} ({status})", 2


def check_once(args):
    sha = latest_commit(args.repo, args.branch)
    run = latest_run(args.repo, args.branch, sha, args.workflow)

    message, code = status_message(sha, run)
    print(message)

    if args.url and run is not None and run.get("url"):
        print(run["url"])

    return code


def parse_args():
    parser = argparse.ArgumentParser(
        description="Watch GitHub Actions CI status for VDR-Suite."
    )
    parser.add_argument("--repo", default=DEFAULT_REPO)
    parser.add_argument("--branch", default=DEFAULT_BRANCH)
    parser.add_argument("--workflow", default=DEFAULT_WORKFLOW)
    parser.add_argument("--interval", type=int, default=DEFAULT_INTERVAL)
    parser.add_argument("--watch", action="store_true")
    parser.add_argument("--url", action="store_true")
    return parser.parse_args()


def main():
    args = parse_args()

    if args.interval < 5:
        print("interval must be at least 5 seconds", file=sys.stderr)
        return 2

    while True:
        try:
            code = check_once(args)
        except Exception as error:
            print(f"❌ CI-Status konnte nicht gelesen werden: {error}", file=sys.stderr)
            return 2

        if not args.watch or code in (0, 1):
            return code

        time.sleep(args.interval)


if __name__ == "__main__":
    raise SystemExit(main())
