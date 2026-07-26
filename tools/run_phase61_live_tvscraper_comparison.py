#!/usr/bin/env python3

import json
import subprocess
import sys
import tempfile
import time
from pathlib import Path


MAXIMUM_ATTEMPTS = 5
RETRY_DELAY_SECONDS = 2


class ResumeFailure(RuntimeError):
    pass


def argument_value(arguments, name):
    for index, value in enumerate(arguments):
        if value == name:
            if index + 1 >= len(arguments):
                raise ResumeFailure(f"{name} benötigt einen Wert")
            return arguments[index + 1]
        prefix = name + "="
        if value.startswith(prefix):
            return value[len(prefix) :]
    return ""


def load_report(path):
    if not path or not Path(path).is_file():
        return {}
    try:
        value = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ResumeFailure(f"Vergleichs-JSON ist ungültig: {error}") from error
    if not isinstance(value, dict):
        raise ResumeFailure("Vergleichs-JSON enthält kein Objekt")
    return value


def coverage_complete(report):
    summary = report.get("summary", {})
    return isinstance(summary, dict) and summary.get("coverageComplete") is True


def ensure_resume(arguments):
    return list(arguments) if "--resume" in arguments else [*arguments, "--resume"]


def run_resumable(compare_script, arguments, runner=subprocess.run, sleeper=time.sleep):
    json_output = argument_value(arguments, "--json-output")
    if not json_output:
        raise ResumeFailure(
            "--json-output ist für eine sichere automatische Fortsetzung erforderlich"
        )

    command = [sys.executable, str(compare_script), *ensure_resume(arguments)]
    last_return_code = 2

    for attempt in range(1, MAXIMUM_ATTEMPTS + 1):
        completed = runner(command, check=False)
        last_return_code = int(completed.returncode)
        report = load_report(json_output)

        if coverage_complete(report) or last_return_code == 0:
            return last_return_code

        if attempt < MAXIMUM_ATTEMPTS:
            inspected = report.get("summary", {}).get("inspected", 0)
            print(
                "SVDRP-Vergleich unvollständig; "
                f"Fortsetzung ab gespeichertem Stand ({inspected} Ereignisse), "
                f"Versuch {attempt + 1}/{MAXIMUM_ATTEMPTS}.",
                file=sys.stderr,
                flush=True,
            )
            sleeper(RETRY_DELAY_SECONDS)

    return last_return_code


def run_self_test():
    with tempfile.TemporaryDirectory() as directory:
        report_path = Path(directory) / "report.json"
        calls = []

        class Result:
            def __init__(self, returncode):
                self.returncode = returncode

        def runner(command, check=False):
            assert check is False
            calls.append(command)
            inspected = len(calls)
            report_path.write_text(
                json.dumps(
                    {
                        "summary": {
                            "inspected": inspected,
                            "coverageComplete": inspected == 2,
                        }
                    }
                ),
                encoding="utf-8",
            )
            return Result(2)

        result = run_resumable(
            Path("compare.py"),
            ["--run", "--limit", "0", "--json-output", str(report_path)],
            runner=runner,
            sleeper=lambda _: None,
        )
        assert result == 2
        assert len(calls) == 2
        assert calls[0][-1] == "--resume"

        arguments = ensure_resume(["--resume", "--json-output=x.json"])
        assert arguments.count("--resume") == 1
        assert argument_value(arguments, "--json-output") == "x.json"

    print("phase61 resumable live/tvscraper comparison wrapper self-test ok")
    return 0


def main():
    arguments = sys.argv[1:]
    if arguments == ["--self-test"]:
        return run_self_test()

    compare_script = Path(__file__).with_name("compare_phase61_live_tvscraper.py")
    if not compare_script.is_file():
        print(f"RESULT: FAIL - Vergleichswerkzeug fehlt: {compare_script}", file=sys.stderr)
        return 2

    try:
        return run_resumable(compare_script, arguments)
    except ResumeFailure as error:
        print(f"RESULT: FAIL - {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
