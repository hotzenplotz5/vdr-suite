"""Offline regression tests for the Public-v1 acceptance runner.

No VDR connection is opened. Mock responses never establish real-VDR evidence.
"""
import argparse
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools/api-v1-acceptance"))
from runner import Acceptance, Blocked, ConfigError, HttpClient, Problem, load_config
from runner import origin_ok, safe_path, validate
from cases import collection, no_private_paths, REQUIRED_ROUTES


class RunnerTests(unittest.TestCase):
    def test_origin_fail_closed(self):
        for value in ("http://example.org", "file:///tmp", "http://127.0.0.1/x",
                      "https://user:secret@example.org", "http://127.0.0.1/?token=x"):
            with self.assertRaises(ConfigError):
                origin_ok(value)
        self.assertEqual(origin_ok("http://127.0.0.1:18080"), "http://127.0.0.1:18080")

    def test_cred_permissions(self):
        with tempfile.TemporaryDirectory() as p:
            config = Path(p) / "private.json"
            config.write_text('{"device":{"credentialId":"someid","credentialSecret":"somesecret"}}')
            config.chmod(0o644)
            with self.assertRaises(ConfigError):
                load_config(config)
            config.chmod(0o600)
            self.assertIn("device", load_config(config))

    def test_no_credential_in_paths(self):
        path = "/api/v1/recordings/rec_" + "a" * 32 + "?token=secret&backendId=private"
        sanitized = safe_path(path)
        self.assertIn("<public-id>", sanitized)
        self.assertNotIn("secret", sanitized)
        self.assertNotIn("private", sanitized)
        self.assertNotIn("rec_" + "a" * 32, sanitized)

    def test_schema_reference_required(self):
        spec = {"$ref": "#/components/schemas/Example"}
        definitions = {"Example": {"type": "object", "required": ["apiVersion"],
                                    "properties": {"apiVersion": {"type": "string",
                                                                   "const": "v1"}}}}
        self.assertTrue(validate({"apiVersion": "v1"}, spec, definitions))
        self.assertFalse(validate({}, spec, definitions))
        self.assertFalse(validate({"apiVersion": "v2"}, spec, definitions))

    def test_http_never_mutates(self):
        client = HttpClient("http://127.0.0.1:18080")
        class FakeReply:
            status = 200
            headers = {"Content-Type": "application/json"}
            def __enter__(self): return self
            def __exit__(self, *_): return False
            def read(self, size): return b'{"apiVersion":"v1"}'
        class FakeOpener:
            def open(self, request, timeout):
                self.outer.assertEqual(request.get_method(), "GET")
                self.outer.assertNotIn("Authorization", dict(request.header_items()))
                return FakeReply()
        opener = FakeOpener()
        opener.outer = self
        client.opener = opener
        reply, _ = client.expect("/api/v1")
        self.assertEqual(reply["apiVersion"], "v1")
        with self.assertRaises(Problem):
            client.get("https://attacker.example/api/v1")

    def test_missing_auth_and_invalid_auth_differ(self):
        client = HttpClient("http://127.0.0.1:18080",
                            {"device": {"credentialId": "abc", "credentialSecret": "xyz"}})
        seen = []
        class FakeReply:
            status = 401
            headers = {"Content-Type": "application/json"}
            def __enter__(self): return self
            def __exit__(self, *_): return False
            def read(self, size): return b'{"error":{"code":"unauthorized"}}'
        class FakeOpener:
            def open(self, req, timeout):
                seen.append(req.get_header("Authorization"))
                return FakeReply()
        client.opener = FakeOpener()
        client.get("/api/v1/backends", auth=False)
        client.get("/api/v1/backends", invalid=True)
        client.get("/api/v1/backends")
        self.assertEqual(seen, [None, "VDR-Suite-Device invalid.invalid",
                                "VDR-Suite-Device abc.xyz"])

    def test_image_real_content_required(self):
        client = HttpClient("http://127.0.0.1:18080")
        class FakeReply:
            status = 200
            headers = {"Content-Type": "image/jpeg"}
            def __enter__(self): return self
            def __exit__(self, *_): return False
            def read(self, size): return b"this-is-not-a-jpeg"
        class FakeOpener:
            def open(self, req, timeout): return FakeReply()
        client.opener = FakeOpener()
        with self.assertRaises(Problem) as ctx:
            client.expect("/api/v1/recordings/rec_" + "1" * 32 +
                          "/artwork/" + "2" * 32, binary=True)
        self.assertEqual(ctx.exception.kind, "invalid_image")

    def test_pagination_bound(self):
        with self.assertRaises(Problem):
            collection({"items": [1, 2, 3]}, "/api/v1/recordings", 2)

    def test_no_path_leak(self):
        self.assertTrue(no_private_paths({"title": "safe", "metadata": {"series": None}}))
        self.assertFalse(no_private_paths({"fileName": "/video/films/test.rec"}))

    def test_partial_and_failed_reporting(self):
        with tempfile.TemporaryDirectory() as p:
            args = argparse.Namespace(profile="offline", backend=None,
                                      origin=None, output_dir=Path(p),
                                      approval_file=None)
            a = Acceptance(args, {})
            a.repo_sha = "a" * 40
            a.record("a.pass", "CONTRACT", "PASS")
            a.record("b.blocked", "RESUME", "BLOCKED", "no fixture",
                     path="/api/v1/recordings?token=secret")
            self.assertEqual(a.persist(), "INCOMPLETE")
            self.assertIn("OVERALL: INCOMPLETE",
                          (Path(p) / "last-summary.txt").read_text())
            self.assertNotIn("secret", (Path(p) / "last-errors.log").read_text())
            self.assertEqual((Path(p) / "last-results.json").stat().st_mode & 0o077, 0)
            self.assertEqual((Path(p) / "last-errors.log").stat().st_mode & 0o077, 0)
            a.record("c.fail", "SECURITY", "FAIL", "permission check violated")
            self.assertEqual(a.persist(), "FAIL")

    def test_coverage_anchored_to_real_contract(self):
        path = Path(__file__).resolve().parents[1] / "api/openapi.json"
        spec = json.loads(path.read_text())
        self.assertTrue(REQUIRED_ROUTES.issubset(spec["paths"]))
        self.assertIn("/api/v1/recording-playback-sessions/progress", spec["paths"])


if __name__ == "__main__":
    unittest.main()
