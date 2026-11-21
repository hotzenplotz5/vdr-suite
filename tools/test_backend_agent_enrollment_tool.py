#!/usr/bin/env python3
"""Black-box contract test for the controlled Backend Agent enrollment utility."""

from pathlib import Path
import os
import sqlite3
import stat
import subprocess
import sys
import tempfile


def parse_key_values(path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        if not raw or raw.startswith("#"):
            continue
        key, separator, value = raw.partition("=")
        if not separator:
            raise AssertionError("malformed enrollment package")
        result[key] = value
    return result


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test_backend_agent_enrollment_tool.py BINARY", file=sys.stderr)
        return 64
    binary = Path(sys.argv[1]).resolve()
    if not binary.is_file():
        raise AssertionError(f"missing enrollment utility: {binary}")

    with tempfile.TemporaryDirectory(prefix="vdr-suite-agent-enroll-") as directory:
        root = Path(directory)
        database = root / "suite.db"
        package = root / "enrollment"
        result = subprocess.run(
            [
                str(binary),
                "--database", str(database),
                "--backend", "default",
                "--output", str(package),
                "--ttl-seconds", "300",
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=20,
            env={"PATH": os.environ.get("PATH", "/usr/bin:/bin")},
        )
        if result.returncode != 0:
            raise AssertionError(
                f"enrollment utility failed: code={result.returncode} stderr={result.stderr!r}"
            )
        mode = stat.S_IMODE(package.stat().st_mode)
        if mode != 0o600:
            raise AssertionError(f"enrollment package mode is {mode:o}, expected 600")
        values = parse_key_values(package)
        token = values.get("enrollment_token", "")
        enrollment_id = values.get("enrollment_id", "")
        if len(token) < 32 or not enrollment_id:
            raise AssertionError("enrollment package lacks bounded bootstrap material")
        combined_output = result.stdout + result.stderr
        if token in combined_output or enrollment_id in result.stderr:
            raise AssertionError("enrollment bootstrap material leaked to command output")

        connection = sqlite3.connect(database)
        row = connection.execute(
            "SELECT token_hash, status, backend_id FROM backend_agent_enrollments "
            "WHERE enrollment_id = ?",
            (enrollment_id,),
        ).fetchone()
        connection.close()
        if row is None:
            raise AssertionError("enrollment record was not persisted")
        token_hash, status, backend_id = row
        if token_hash == token or token in token_hash:
            raise AssertionError("raw enrollment token was persisted")
        if not (token_hash.startswith("$6$") or token_hash.startswith("$y$")):
            raise AssertionError("enrollment verifier has an unsupported format")
        if status != "pending" or backend_id != "default":
            raise AssertionError("enrollment binding/status mismatch")

        repeated = subprocess.run(
            [str(binary), "--database", str(database), "--output", str(package)],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=20,
        )
        if repeated.returncode == 0:
            raise AssertionError("enrollment utility overwrote an existing package")
        if token in repeated.stdout or token in repeated.stderr:
            raise AssertionError("existing enrollment secret leaked on overwrite refusal")

    print("test_backend_agent_enrollment_tool passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
