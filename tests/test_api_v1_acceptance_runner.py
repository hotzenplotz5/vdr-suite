"""Offline regression tests for the Public-v1 acceptance runner.

No VDR connection is opened. Mock responses never establish real-VDR evidence.
"""
import argparse
import threading
import http.server
import urllib.parse
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
from cases import root, backends, recordings, recording_detail, metadata
from cases import artwork_list, artwork_bytes, continue_watching, resume_progress


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

    def test_legacy_flat_credential_receipt_is_private_and_normalized(self):
        with tempfile.TemporaryDirectory() as p:
            path = Path(p) / "receipt.json"
            path.write_text(json.dumps({"credentialId": "device-id",
                                        "credentialSecret": "opaque-secret-value"}))
            path.chmod(0o600)
            cfg = load_config(path)
            self.assertEqual(cfg["device"]["credentialId"], "device-id")
            self.assertEqual(cfg["device"]["credentialSecret"], "opaque-secret-value")
            self.assertNotIn("credentialId", cfg)
            self.assertEqual(json.loads(path.read_text())["credentialId"], "device-id")
            self.assertEqual(path.stat().st_mode & 0o077, 0)

    def test_legacy_flat_credential_receipt_rejects_unsafe_or_partial(self):
        with tempfile.TemporaryDirectory() as p:
            path = Path(p) / "receipt.json"
            path.write_text(json.dumps({"credentialId": "id", "credentialSecret": "secret"}))
            path.chmod(0o644)
            with self.assertRaises(ConfigError):
                load_config(path)
            path.chmod(0o600)
            path.write_text(json.dumps({"credentialId": "id"}))
            with self.assertRaises(ConfigError):
                load_config(path)
            path.write_text(json.dumps({"device":{"credentialId":"id","credentialSecret":"secret"},
                                        "credentialId":"id","credentialSecret":"secret"}))
            with self.assertRaises(ConfigError):
                load_config(path)

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

    def test_real_loopback_http_fixture_recording_to_p1_chain(self):
        """End-to-end real HTTP GET fixture, zero writes, no productive hardware."""
        rid = "rec_" + "1" * 32
        artid = "2" * 32
        arturl = "/api/v1/recordings/" + rid + "/artwork/" + artid
        seen_methods = []

        class Fixture(http.server.BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def do_POST(self):
                seen_methods.append("POST")
                self.send_response(405)
                self.end_headers()

            def do_GET(self):
                seen_methods.append("GET")
                if self.headers.get("Authorization") != "VDR-Suite-Device id.secret":
                    self.send_response(401)
                    self.end_headers()
                    return
                path = urllib.parse.urlsplit(self.path).path
                if path == "/api/v1":
                    data = {"apiVersion": "v1", "serverVersion": "git-source",
                            "authentication": {"authenticated": True}}
                elif path == "/api/v1/backends":
                    data = {"items": [{"backendId": "default"}]}
                elif path == "/api/v1/recordings":
                    data = {"items": [{"recordingId": rid}]}
                elif path == "/api/v1/recordings/" + rid:
                    data = {"recordingId": rid, "metadata": {"state": "absent",
                                                          "source": "none",
                                                          "contentKind": "unknown"}}
                elif path == "/api/v1/recordings/" + rid + "/artwork":
                    data = {"recordingId": rid, "items": [
                        {"artworkId": artid, "url": arturl, "kind": "poster"}]}
                elif path == arturl:
                    data = None
                elif path.endswith("/continue-watching"):
                    data = {"items": [], "backendId": "default", "limit": 50}
                elif path.endswith("/progress"):
                    data = {"recordingId": rid, "backendId": "default",
                            "hasProgress": False, "positionSeconds": 0,
                            "revision": 0}
                else:
                    self.send_response(404)
                    self.end_headers()
                    return
                payload = (b"\xff\xd8\xff\x00picture" if data is None
                           else json.dumps(data).encode("utf-8"))
                self.send_response(200)
                self.send_header("Content-Type", "image/jpeg" if data is None
                                 else "application/json")
                self.send_header("Content-Length", str(len(payload)))
                self.end_headers()
                self.wfile.write(payload)

        server = http.server.HTTPServer(("127.0.0.1", 0), Fixture)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            with tempfile.TemporaryDirectory() as tmp:
                cfg = {"device": {"credentialId": "id", "credentialSecret": "secret"}}
                a = Acceptance(argparse.Namespace(
                    profile="readonly", backend="default",
                    origin="http://127.0.0.1:" + str(server.server_port),
                    output_dir=Path(tmp), approval_file=None), cfg)
                root(a)
                backends(a)
                recordings(a)
                recording_detail(a)
                metadata(a)
                artwork_list(a)
                artwork_bytes(a)
                continue_watching(a)
                resume_progress(a)
                self.assertEqual(a.context["recording"], rid)
                self.assertTrue(seen_methods)
                self.assertEqual(set(seen_methods), {"GET"})
                self.assertNotIn("secret", (Path(tmp) / "last-errors.log").read_text()
                                 if (Path(tmp) / "last-errors.log").exists() else "")
        finally:
            server.shutdown()
            thread.join(timeout=5)
            server.server_close()

    def test_coverage_anchored_to_real_contract(self):
        path = Path(__file__).resolve().parents[1] / "api/openapi.json"
        spec = json.loads(path.read_text())
        self.assertTrue(REQUIRED_ROUTES.issubset(spec["paths"]))
        self.assertIn("/api/v1/recording-playback-sessions/progress", spec["paths"])

    def _version_a(self, out, expected=None, profile="readonly"):
        args = argparse.Namespace(profile=profile, backend="default",
            origin="http://127.0.0.1:18080", output_dir=Path(out),
            approval_file=None, expected_daemon_commit=expected)
        return Acceptance(args, {})

    def test_runner_branch_sha_independent_from_daemon(self):
        from cases import runtime_match
        target = "68f6d05f0ed6dbd6691776910e17b29e0b779554"
        with tempfile.TemporaryDirectory() as out:
            a = self._version_a(out, target)
            a.repo_sha = "b" * 40
            a.context["root"] = {"serverVersion": "git-68f6d05f0ed6"}
            runtime_match(a)
            self.assertTrue(a.context["runtime_verified"])
            a.record("preflight.source-runtime", "INTEGRATION", "PASS")
            result = json.loads((Path(out)/"last-results.json").read_text())
            self.assertEqual(result["sourceCommit"], "b"*40)
            self.assertEqual(result["expectedDaemonCommit"], target)
            self.assertTrue((Path(out)/"last-full.log").exists())

    def test_actual_daemon_mismatch_fails(self):
        from cases import runtime_match
        with tempfile.TemporaryDirectory() as out:
            a = self._version_a(out, "68f6d05f0ed6dbd6691776910e17b29e0b779554")
            a.context["root"] = {"serverVersion": "git-715b781206a2"}
            with self.assertRaises(Problem) as err:
                runtime_match(a)
            self.assertEqual(err.exception.kind, "daemon_version_mismatch")
            self.assertIn("MISMATCH", a.evidence)
            self.assertNotIn("runtime_verified", a.context)

    def test_missing_target_blocks_and_bad_sha_is_config_error(self):
        from cases import runtime_match
        with tempfile.TemporaryDirectory() as out:
            a = self._version_a(out)
            a.context["root"] = {"serverVersion": "git-68f6d05f0ed6"}
            with self.assertRaises(Blocked):
                runtime_match(a)
            with self.assertRaises(ConfigError):
                self._version_a(out, "68f6d05f0ed6")

    def test_non_git_daemon_version_blocks(self):
        from cases import runtime_match
        with tempfile.TemporaryDirectory() as out:
            a = self._version_a(out, "68f6d05f0ed6dbd6691776910e17b29e0b779554")
            a.context["root"] = {"serverVersion": "release-1.2"}
            with self.assertRaises(Blocked):
                runtime_match(a)

    def test_mismatch_suppresses_all_other_live_requests(self):
        from cases import run_cases
        with tempfile.TemporaryDirectory() as out:
            a = self._version_a(out, "68f6d05f0ed6dbd6691776910e17b29e0b779554")
            def fake_root(x):
                x.context["root"] = {"serverVersion":"git-715b781206a2"}
                x.runtime_version = "git-715b781206a2"
            def fake_spec(x):
                x.context["spec"] = True
            with patch("cases.root", side_effect=fake_root), \
                 patch("cases.check_spec", side_effect=fake_spec), \
                 patch("cases.check_source"), patch("cases.source_contract_tests"), \
                 patch("cases.offline_unit"), patch("cases.controlled_contracts_offline"), \
                 patch.object(a.client,"get",side_effect=AssertionError("unexpected backend GET")):
                run_cases(a)
            state = {x["id"]:x["status"] for x in a.results}
            self.assertEqual(state["preflight.source-runtime"], "FAIL")
            self.assertEqual(state["backends.authorized"], "BLOCKED")
            self.assertEqual(state["security.invalid-credential"], "BLOCKED")
            self.assertEqual(state["recording.collection"], "BLOCKED")
            self.assertEqual(a.persist(), "FAIL")

    def test_controlled_writes_stay_blocked(self):
        from cases import run_cases
        with tempfile.TemporaryDirectory() as out:
            a = self._version_a(out, profile="controlled")
            with patch("cases.root", side_effect=Blocked("no daemon")), \
                 patch("cases.check_source"), patch("cases.check_spec"), \
                 patch("cases.source_contract_tests"), patch("cases.offline_unit"), \
                 patch("cases.controlled_contracts_offline"), \
                 patch.object(a.client,"get",side_effect=AssertionError("unexpected HTTP")):
                run_cases(a)
            state = {x["id"]:x["status"] for x in a.results}
            self.assertEqual(state["controlled.approval"], "BLOCKED")
            self.assertEqual(state["timer.intent-create"], "BLOCKED")
            self.assertEqual(state["resume.progress-write"], "BLOCKED")
            self.assertEqual(a.persist(), "INCOMPLETE")

    def test_timeout_error_is_redacted(self):
        import urllib.error
        client = HttpClient("http://127.0.0.1:18080")
        class FakeOpener:
            def open(self, request, timeout):
                raise urllib.error.URLError("credentialSecret=unsafe")
        client.opener = FakeOpener()
        with self.assertRaises(Blocked) as err:
            client.expect("/api/v1")
        self.assertNotIn("unsafe", err.exception.detail)

    def test_schema_rejects_bool_as_integer(self):
        self.assertFalse(validate(True, {"type":"integer"}, {}))

    def test_controlled_manifest_binds_target_not_runner_sha(self):
        from cases import controlled_gate
        target = "68f6d05f0ed6dbd6691776910e17b29e0b779554"
        with tempfile.TemporaryDirectory() as out:
            a = self._version_a(out, target, profile="controlled")
            a.repo_sha = "b" * 40
            approval = Path(out) / "approval.json"
            approval.write_text(json.dumps({
                "sourceCommit":target, "backend":"default",
                "targetHost":"http://127.0.0.1:18080",
                "approvedFixtureIds":["test-fixture"], "backupEvidence":"none",
                "rollbackEvidence":"none"}))
            approval.chmod(0o600)
            a.args.approval_file = approval
            with self.assertRaises(Blocked) as e:
                controlled_gate(a)
            self.assertIn("mutation execution intentionally locked", e.exception.detail)

    def test_admin_browser_login_cookie_only_on_authenticated_reads(self):
        import base64
        seen = []
        client = HttpClient("http://127.0.0.1:18080")
        class Reply:
            def __init__(self, status, body, headers=None):
                self.status = status
                self._body = body
                self.headers = headers or {"Content-Type":"application/json"}
            def __enter__(self): return self
            def __exit__(self,*args): return False
            def read(self,size): return self._body
        class Opener:
            def open(self,req,timeout):
                url = urllib.parse.urlsplit(req.full_url).path
                method = req.get_method()
                cookie = req.get_header("Cookie")
                basic = req.get_header("Authorization")
                seen.append((method,url,cookie,basic))
                if url == "/api/security/browser-sessions":
                    assert method == "POST"
                    assert basic == "Basic " + base64.b64encode(b"vdr-suite:somepassword").decode()
                    assert req.get_header("Origin") == "http://127.0.0.1:18080"
                    return Reply(200,b'{"csrfToken":"valid-csrf"}',
                                 {"Content-Type":"application/json",
                                  "Set-Cookie":"vdr_suite_session=abcde0123456789; HttpOnly; Path=/"})
                if url == "/api/security/browser-sessions/logout":
                    assert method == "POST"
                    assert cookie == "vdr_suite_session=abcde0123456789"
                    return Reply(200,b'{}')
                if url == "/api/v1":
                    assert method == "GET"
                    return Reply(200,b'{"authentication":{"authenticated":true}}')
                if url == "/api/v1/backends":
                    if cookie:
                        return Reply(200,b'{"items":[{"backendId":"default"}]}')
                    return Reply(401,b'{"error":{"code":"unauthorized"}}')
                assert False, url
        client.opener = Opener()
        self.assertEqual(client.open_browser_session("vdr-suite","somepassword"),
                         "administrator-session-created")
        self.assertTrue(client.expect("/api/v1")[0]["authentication"]["authenticated"])
        self.assertEqual(client.expect("/api/v1/backends")[0]["items"][0]["backendId"],"default")
        self.assertEqual(client.get("/api/v1/backends",auth=False)[0],401)
        self.assertEqual(client.get("/api/v1/backends",invalid=True)[0],401)
        client.close_browser_session()
        self.assertIsNone(client.browser_cookie)
        self.assertEqual([x[0] for x in seen],["POST","GET","GET","GET","GET","POST"])
        self.assertIsNone(seen[3][2])
        self.assertIsNone(seen[4][2])
        self.assertEqual(seen[4][3],"VDR-Suite-Device invalid.invalid")
        self.assertEqual(seen[0][2],None)

    def test_admin_login_denied_no_session_or_secret_log(self):
        client = HttpClient("http://127.0.0.1:18080")
        class FakeOpener:
            def open(self,req,timeout):
                raise urllib.error.HTTPError(req.full_url,401,"unauthorized",{},None)
        import urllib.error
        client.opener = FakeOpener()
        with self.assertRaises(Blocked) as error:
            client.open_browser_session("vdr-suite","DONOTLOGPASSWORD")
        self.assertNotIn("DONOTLOGPASSWORD",error.exception.detail)
        self.assertIsNone(client.browser_cookie)
        self.assertEqual(error.exception.actual,401)

    def test_admin_login_never_uses_remote_plaintext_or_device_secret(self):
        with self.assertRaises(ConfigError):
            HttpClient("http://other.example:18080")
        remote = HttpClient("https://other.example")
        with self.assertRaises(ConfigError):
            remote.open_browser_session("vdr-suite","secret")
        with tempfile.TemporaryDirectory() as out:
            args = argparse.Namespace(profile="readonly",backend="default",
                origin="http://127.0.0.1:18080", output_dir=Path(out),
                approval_file=None,expected_daemon_commit="a"*40,
                admin_user="vdr-suite")
            with self.assertRaises(ConfigError):
                Acceptance(args,{"device":{"credentialId":"id","credentialSecret":"secret"}})

    def test_admin_requires_device_for_playback_owned_reads(self):
        from cases import require_device, require_actor
        with tempfile.TemporaryDirectory() as out:
            args = argparse.Namespace(profile="readonly",backend="default",
                origin="http://127.0.0.1:18080", output_dir=Path(out),
                approval_file=None,expected_daemon_commit="a"*40,
                admin_user="vdr-suite")
            a = Acceptance(args,{})
            a.client.browser_cookie = "vdr_suite_session=abcdefgh"
            require_actor(a)
            with self.assertRaises(Blocked):
                require_device(a)

    def test_admin_failed_auth_blocks_dependency_chain(self):
        from cases import run_cases
        with tempfile.TemporaryDirectory() as out:
            args = argparse.Namespace(profile="readonly",backend="default",
                origin="http://127.0.0.1:18080", output_dir=Path(out),
                approval_file=None,expected_daemon_commit="a"*40,
                admin_user="vdr-suite")
            a = Acceptance(args,{})
            def fake_root(x): x.context["root"] = {"serverVersion":"git-aaaaaaaaaaaa"}
            def fake_version(x): x.context["runtime_verified"] = True
            with patch("cases.root",side_effect=fake_root), \
                 patch("cases.runtime_match",side_effect=fake_version), \
                 patch("cases.browser_admin_login",side_effect=Blocked("wrong password")), \
                 patch("cases.check_source"), patch("cases.check_spec"), \
                 patch("cases.source_contract_tests"), patch("cases.offline_unit"), \
                 patch("cases.controlled_contracts_offline"), \
                 patch.object(a.client,"get",side_effect=AssertionError("unexpected GET")):
                run_cases(a)
            statuses = {x["id"]:x["status"] for x in a.results}
            self.assertEqual(statuses["security.browser-admin-login"],"BLOCKED")
            self.assertEqual(statuses["backends.authorized"],"BLOCKED")
            self.assertEqual(statuses["recording.collection"],"BLOCKED")
            self.assertEqual(a.persist(),"INCOMPLETE")


if __name__ == "__main__":
    unittest.main()
