#!/usr/bin/env python3
"""Offline contract tests for the opt-in MLD RESTfulAPI measurement script."""

from pathlib import Path
import os
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("mld66-readonly-http-probe.sh")


class MldReadOnlyHttpProbeTest(unittest.TestCase):
    def run_probe(self, option, exit_code=0):
        with tempfile.TemporaryDirectory() as tmp:
            bin_dir = Path(tmp)
            fake_curl = bin_dir / "curl"
            calls = bin_dir / "calls"
            fake_curl.write_text(
                "#!/bin/sh\n"
                ': "${MLD_TEST_CALLS:?}"\n'
                'printf "%s\\n" "$@" >> "$MLD_TEST_CALLS"\n'
                'printf "http=200 seconds=0.123 bytes=18"\n'
                f"exit {exit_code}\n",
                encoding="utf-8",
            )
            fake_curl.chmod(0o755)
            env = {**os.environ, "PATH": f"{bin_dir}:{os.environ.get('PATH', '')}",
                   "MLD_TEST_CALLS": str(calls)}
            p = subprocess.run(["sh", str(SCRIPT), option], env=env,
                               text=True, capture_output=True, check=False)
            args = calls.read_text(encoding="utf-8").splitlines() if calls.exists() else []
            return p, args

    def test_metadata_stays_local_and_does_not_emit_body(self):
        p, args = self.run_probe("--metadata")
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertEqual(p.stdout.count("CURL_EXIT="), 2)
        self.assertIn("REQUEST=info", p.stdout)
        self.assertIn("REQUEST=change-state", p.stdout)
        self.assertIn("-q", args)
        self.assertIn("--noproxy", args)
        self.assertIn("--output", args)
        self.assertIn("/dev/null", args)
        self.assertIn("--max-time", args)
        self.assertIn("http://127.0.0.1:8002/info.json", args)
        self.assertIn("http://127.0.0.1:8002/change-state.json", args)
        self.assertNotIn("--location", args)
        self.assertNotIn("--data", args)
        self.assertNotIn("recordings.json", " ".join(args))

    def test_recordings_sample_one_bounded_request_no_retry(self):
        p, args = self.run_probe("--recordings-sample", exit_code=28)
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertIn("REQUEST=recordings-sample CURL_EXIT=28", p.stdout)
        self.assertEqual(args.count("http://127.0.0.1:8002/recordings.json?start=0&limit=1"), 1)
        self.assertIn("30", args)
        self.assertNotIn("http://127.0.0.1:8002/recordings.json", args)

    def test_invalid_invocation_does_not_issue_request(self):
        p, args = self.run_probe("--unrecognized")
        self.assertEqual(p.returncode, 2)
        self.assertEqual(args, [])


if __name__ == "__main__":
    unittest.main()
