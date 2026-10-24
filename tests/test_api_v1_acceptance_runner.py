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
from cases import authorized_operation


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

    def test_authorized_operation_accepts_legacy_opaque_ids(self):
        """Fixture IDs follow durable storage compatibility, not new ID issuance."""
        class OwnedOperation:
            backend = "default"
            def __init__(self, operation_id):
                self.cfg = {"existingOperationId": operation_id}
                self.requests = []
            def get_json(self, path):
                self.requests.append(path)
                return ({"operationId": self.cfg["existingOperationId"],
                         "backendId": "default", "state": "outcome_unknown"}, {})
            def require(self, valid, message, **_):
                if not valid:
                    raise AssertionError(message)

        for candidate in ("op_" + "a" * 32, "op_invalid",
                          "historical:opaque.1", "h" * 160):
            fixture = OwnedOperation(candidate)
            self.assertEqual(authorized_operation(fixture),
                             "operation-state:outcome_unknown")
            self.assertEqual(fixture.requests,
                             ["/api/v1/operations/" + candidate])

        for invalid in (None, "", ".", "..", "h" * 161,
                        "bad/id", "bad?key=x", "bad%2Fid", "bad\\nvalue"):
            fixture = OwnedOperation(invalid)
            with self.assertRaises(Blocked):
                authorized_operation(fixture)
            self.assertEqual(fixture.requests, [])

    def test_operation_read_openapi_preserves_historical_ids(self):
        spec = json.loads((Path(__file__).resolve().parents[1] /
                           "api/openapi.json").read_text(encoding="utf-8"))
        operation = spec["paths"]["/api/v1/operations/{operationId}"]["get"]
        parameter = next(p for p in operation["parameters"]
                         if p["name"] == "operationId")
        self.assertEqual(parameter["schema"]["minLength"], 1)
        self.assertEqual(parameter["schema"]["maxLength"], 160)
        self.assertNotIn("pattern", parameter["schema"])

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
                 patch("cases.discovery"), patch("cases.missing_auth"), \
                 patch("cases.invalid_auth"), \
                 patch.object(a.client,"get",side_effect=AssertionError("unexpected GET")):
                run_cases(a)
            statuses = {x["id"]:x["status"] for x in a.results}
            self.assertEqual(statuses["security.browser-admin-login"],"BLOCKED")
            self.assertEqual(statuses["backends.authorized"],"BLOCKED")
            self.assertEqual(statuses["recording.collection"],"BLOCKED")
            self.assertEqual(a.persist(),"INCOMPLETE")

    def test_artwork_accepts_server_supported_1mib_jpeg(self):
        from runner import MAX_IMAGE
        self.assertEqual(MAX_IMAGE, 16*1024*1024)
        client = HttpClient("http://127.0.0.1:18080")
        data = bytes((255, 216, 255)) + b"x"*(1024*1024)
        class FakeReply:
            status = 200
            headers = {"Content-Type":"image/jpeg"}
            def __enter__(self): return self
            def __exit__(self,*x): return False
            def read(self,n):
                assert n == MAX_IMAGE+1
                return data[:n]
        class FakeOpener:
            def open(self,request,timeout):
                assert request.get_method() == "GET"
                return FakeReply()
        client.opener = FakeOpener()
        result, _ = client.expect("/api/v1/recordings/rec_"+"a"*32+
                                  "/artwork/"+"b"*32+"?backendId=default",binary=True)
        self.assertEqual(result["bytes_read"],len(data))
        self.assertEqual(result["image_type"],"image/jpeg")

    def test_artwork_larger_than_server_limit_rejected_bounded(self):
        from runner import MAX_IMAGE
        client = HttpClient("http://127.0.0.1:18080")
        class FakeReply:
            status = 200
            headers = {"Content-Type":"image/jpeg","X-Request-ID":"req-test"}
            def __enter__(self): return self
            def __exit__(self,*args): return False
            def read(self,n):
                assert n == MAX_IMAGE+1
                return b"x"*n
        class FakeOpener:
            def open(self,request,timeout): return FakeReply()
        client.opener = FakeOpener()
        with self.assertRaises(Problem) as p:
            client.expect("/api/v1/recordings/rec_"+"a"*32+
                          "/artwork/"+"b"*32+"?backendId=default",binary=True)
        self.assertEqual(p.exception.kind,"response_too_large")
        self.assertEqual(p.exception.actual,200)

    def test_negative_epg_and_recordings_accept_authenticated_admin(self):
        from cases import status_negative
        with tempfile.TemporaryDirectory() as out:
            args = argparse.Namespace(profile="readonly",backend="default",
                origin="http://127.0.0.1:18080",output_dir=Path(out),
                approval_file=None,expected_daemon_commit="a"*40,
                admin_user="vdr-suite")
            a = Acceptance(args,{})
            a.client.browser_cookie = "vdr_suite_session=abcdefgh"
            with patch.object(a.client,"get",return_value=(400,{},{})) as get:
                status_negative(a,"/api/v1/recordings/rec_bad?backendId=default",(400,))
                status_negative(a,"/api/v1/epg/window?bad=1",(400,))
                self.assertEqual(get.call_count,2)

    def test_negative_searchtimer_403_is_blocked_not_invalid_id_pass(self):
        from cases import status_negative
        with tempfile.TemporaryDirectory() as out:
            args = argparse.Namespace(profile="readonly",backend="default",
                origin="http://127.0.0.1:18080",output_dir=Path(out),
                approval_file=None,expected_daemon_commit="a"*40,
                admin_user="vdr-suite")
            a = Acceptance(args,{})
            a.client.browser_cookie = "vdr_suite_session=abcdefgh"
            with patch.object(a.client,"get",return_value=(403,{},{})):
                with self.assertRaises(Blocked) as e:
                    status_negative(a,"/api/v1/search-timers/st_bad?backend=default",(400,))
            self.assertEqual(e.exception.kind,"permission_denied")

    def test_admin_searchtimer_requires_device_without_attempted_get(self):
        from cases import searchtimers, draft_preview, searchtimer_bad_public_id
        with tempfile.TemporaryDirectory() as out:
            args = argparse.Namespace(profile="readonly",backend="default",
                origin="http://127.0.0.1:18080",output_dir=Path(out),
                approval_file=None,expected_daemon_commit="a"*40,
                admin_user="vdr-suite")
            a = Acceptance(args,{})
            a.client.browser_cookie = "vdr_suite_session=abcdefgh"
            with patch.object(a.client,"get",side_effect=AssertionError("forbidden Device GET")):
                for check in (searchtimers,draft_preview,searchtimer_bad_public_id):
                    with self.assertRaises(Blocked) as result:
                        check(a)
                    self.assertIn("Device credential",result.exception.detail)

    def test_domain_pass_precedes_intentionally_skipped_write(self):
        with tempfile.TemporaryDirectory() as out:
            a = self._version_a(out)
            a.record("recording.collection","RECORDINGS","PASS")
            a.record("recording.actions","RECORDINGS","SKIP",required=False)
            self.assertEqual(a.persist(),"PASS")
            summary = (Path(out)/"last-summary.txt").read_text()
            self.assertIn("RECORDINGS        PASS",summary)
            self.assertNotIn("RECORDINGS        SKIP",summary)

    def test_permission_inventory_never_prints_secrets_and_covers_existing_devices(self):
        from inspect_existing_access import inspect
        import io
        from contextlib import redirect_stdout
        seen = []
        class FakeClient:
            def expect(self,path):
                seen.append(path)
                if path.startswith("/api/v1/accounts?"):
                    return {"items":[{"accountId":"accountA","displayName":"Admin",
                                      "active":True}]},{}
                if path == "/api/v1/accounts/accountA/grants":
                    return {"items":[{"permission":"role.admin","backendId":"*",
                                      "credentialSecret":"MUST_NOT_LEAK"}]},{}
                if path.startswith("/api/v1/devices?"):
                    return {"items":[{"deviceId":"devA","displayName":"yaVDR Timer Hardwaretest",
                                      "active":True,"revoked":False},
                                     {"deviceId":"devB","displayName":"Hisense VIDAA TV",
                                      "active":True,"revoked":False},
                                     {"deviceId":"devC","displayName":"Revoked",
                                      "active":False,"revoked":True}]},{}
                if path == "/api/v1/devices/devA/grants":
                    return {"items":[{"permission":"timers.view","backendId":"default"},
                                     {"permission":"searchtimers.view","backendId":"default"},
                                     {"permission":"media.recording.play","backendId":"default"}]},{}
                if path == "/api/v1/devices/devB/grants":
                    return {"items":[]},{}
                raise AssertionError(path)
        capture=io.StringIO()
        with redirect_stdout(capture):
            inspect(FakeClient())
        output=capture.getvalue()
        self.assertNotIn("MUST_NOT_LEAK",output)
        self.assertIn("yaVDR Timer Hardwaretest",output)
        self.assertIn("searchtimers.view@default: grant vorhanden",output)
        self.assertIn("Hisense VIDAA TV",output)
        self.assertNotIn("devA",output)
        self.assertNotIn("accountA",output)
        self.assertEqual(len(seen),5)

    def test_permission_inventory_rejects_credential_shaped_grants(self):
        from inspect_existing_access import grant_summary, safe_resource_id
        self.assertEqual(grant_summary({"items":[{"permission":"timers.view",
                        "backendId":"default","credentialSecret":"invisible"}]}),
                         [("timers.view","default")])
        with self.assertRaises(Problem):
            grant_summary({"items":[{"permission":"bad\\nsecret","backendId":"*"}]})
        with self.assertRaises(Blocked):
            safe_resource_id("../escape")

    def test_existing_device_audit_proves_only_authorized_gate(self):
        from inspect_device_auth_audit import inspect
        import sqlite3
        import io
        from contextlib import redirect_stdout
        import datetime as dt
        connection=sqlite3.connect(":memory:")
        connection.execute("CREATE TABLE security_devices (device_id TEXT, actor_id TEXT, display_name TEXT, active INT, revoked_at TEXT)")
        connection.execute("CREATE TABLE accountability_events (occurred_at TEXT,permission TEXT,backend_id TEXT,event_type TEXT,decision TEXT,authentication_state TEXT,actor_type TEXT,outcome TEXT,device_id TEXT,actor_id TEXT)")
        connection.execute("INSERT INTO security_devices VALUES (?,?,?,?,?)",
                           ("device_secret_ID","actor_secret_ID","Hisense VIDAA TV",1,""))
        now=dt.datetime(2026,10,23,17,0,0,tzinfo=dt.timezone.utc)
        rows=[
            (now.isoformat(),"searchtimers.view","default","authorization.allowed","allowed","authenticated","service","dispatch_authorized","device_secret_ID","actor_secret_ID"),
            (now.isoformat(),"media.recording.play","default","authorization.denied","denied","authenticated","service","dispatch_denied","device_secret_ID","actor_secret_ID"),
            (now.isoformat(),"timers.view","default","authorization.allowed","allowed","authenticated","user","dispatch_authorized","device_secret_ID","actor_secret_ID"),
            (now.isoformat(),"epg.view","other","authorization.allowed","allowed","authenticated","service","dispatch_authorized","device_secret_ID","actor_secret_ID")]
        connection.executemany("INSERT INTO accountability_events VALUES (?,?,?,?,?,?,?,?,?,?)",rows)
        buf=io.StringIO()
        with redirect_stdout(buf):
            self.assertTrue(inspect(connection,"Hisense VIDAA TV",30,current=now))
        output=buf.getvalue()
        self.assertIn("searchtimers.view@default: 1",output)
        self.assertIn("timers.view@default: 0",output)
        self.assertIn("DEVICE_AUTHENTICATED_GATE: PASS",output)
        self.assertIn("HTTP_RESPONSE_AND_VDR_OUTCOME: NOT_VERIFIED",output)
        self.assertNotIn("device_secret_ID",output)
        self.assertNotIn("actor_secret_ID",output)
        connection.close()

    def test_existing_device_audit_missing_activity_is_not_pass(self):
        from inspect_device_auth_audit import inspect, AuditBlocked, parse_iso
        import sqlite3
        import io
        from contextlib import redirect_stdout
        import datetime as dt
        c=sqlite3.connect(":memory:")
        c.execute("CREATE TABLE security_devices (device_id TEXT, actor_id TEXT, display_name TEXT, active INT, revoked_at TEXT)")
        c.execute("CREATE TABLE accountability_events (occurred_at TEXT,permission TEXT,backend_id TEXT,event_type TEXT,decision TEXT,authentication_state TEXT,actor_type TEXT,outcome TEXT,device_id TEXT,actor_id TEXT)")
        c.execute("INSERT INTO security_devices VALUES (?,?,?,?,?)",("x","y","Hisense VIDAA TV",1,""))
        now=dt.datetime(2026,10,23,17,0,tzinfo=dt.timezone.utc)
        with redirect_stdout(io.StringIO()) as output:
            self.assertFalse(inspect(c,"Hisense VIDAA TV",30,current=now))
        self.assertIn("DEVICE_AUTHENTICATED_GATE: NOT_VERIFIED",output.getvalue())
        self.assertIsNone(parse_iso("not-time"))
        with self.assertRaises(AuditBlocked):
            inspect(c,"Unknown TV",30,current=now)
        c.close()

    def test_existing_device_audit_requires_real_readonly_database(self):
        from inspect_device_auth_audit import open_ro, AuditBlocked
        import sqlite3
        with tempfile.TemporaryDirectory() as root:
            database=Path(root)/"read.db"
            conn=sqlite3.connect(database)
            conn.execute("CREATE TABLE security_devices (device_id TEXT)")
            conn.execute("CREATE TABLE accountability_events (event_id TEXT)")
            conn.commit()
            conn.close()
            ro=open_ro(str(database))
            with self.assertRaises(sqlite3.OperationalError):
                ro.execute("INSERT INTO security_devices VALUES ('forbidden')")
            ro.close()
            check=sqlite3.connect(database)
            self.assertEqual(check.execute("SELECT COUNT(*) FROM security_devices").fetchone()[0],0)
            check.close()
            link=Path(root)/"symlink.db"
            link.symlink_to(database)
            with self.assertRaises(AuditBlocked):
                open_ro(str(link))

    def test_device_historical_audit_counts_all_events_beyond_500(self):
        from inspect_device_auth_audit import inspect
        import sqlite3
        import datetime as dt
        import io
        from contextlib import redirect_stdout
        c=sqlite3.connect(":memory:")
        c.execute("CREATE TABLE security_devices (device_id TEXT,actor_id TEXT,display_name TEXT,active INT,revoked_at TEXT)")
        c.execute("CREATE TABLE accountability_events (occurred_at TEXT,permission TEXT,backend_id TEXT,event_type TEXT,decision TEXT,authentication_state TEXT,actor_type TEXT,outcome TEXT,device_id TEXT,actor_id TEXT)")
        c.execute("INSERT INTO security_devices VALUES (?,?,?,?,?)",
                  ("device1","actor1","Hisense VIDAA TV",1,""))
        now=dt.datetime(2026,10,23,18,0,tzinfo=dt.timezone.utc)
        valid=("2026-10-22T20:00:00Z","recordings.view","default",
               "authorization.allowed","allowed","authenticated","service",
               "dispatch_authorized","device1","actor1")
        invalid_actor=("2026-10-22T20:00:00Z","timers.view","default",
                       "authorization.allowed","allowed","authenticated",
                       "user","dispatch_authorized","device1","actor1")
        stale=("2026-10-15T20:00:00+00:00","epg.view","default",
               "authorization.allowed","allowed","authenticated","service",
               "dispatch_authorized","device1","actor1")
        c.executemany("INSERT INTO accountability_events VALUES (?,?,?,?,?,?,?,?,?,?)",
                      [valid]*650+[invalid_actor]*3+[stale]*4)
        output=io.StringIO()
        with redirect_stdout(output):
            self.assertTrue(inspect(c,"Hisense VIDAA TV",10080,current=now))
        printed=output.getvalue()
        self.assertIn("recordings.view@default: 650",printed)
        self.assertIn("timers.view@default: 0",printed)
        self.assertIn("epg.view@default: 0",printed)
        self.assertIn("AUDIT_WINDOW: FULL_QUERY",printed)
        self.assertNotIn("PARTIAL",printed)
        self.assertNotIn("device1",printed)
        c.close()

    def test_device_historical_audit_normalizes_utc_offsets(self):
        from inspect_device_auth_audit import inspect
        import sqlite3
        import datetime as dt
        import io
        from contextlib import redirect_stdout
        c=sqlite3.connect(":memory:")
        c.execute("CREATE TABLE security_devices (device_id TEXT,actor_id TEXT,display_name TEXT,active INT,revoked_at TEXT)")
        c.execute("CREATE TABLE accountability_events (occurred_at TEXT,permission TEXT,backend_id TEXT,event_type TEXT,decision TEXT,authentication_state TEXT,actor_type TEXT,outcome TEXT,device_id TEXT,actor_id TEXT)")
        c.execute("INSERT INTO security_devices VALUES (?,?,?,?,?)",("a","b","TV",1,""))
        c.execute("INSERT INTO accountability_events VALUES (?,?,?,?,?,?,?,?,?,?)",
                  ("2026-10-23T18:00:00+02:00","searchtimers.view","default",
                   "authorization.allowed","allowed","authenticated","service",
                   "dispatch_authorized","a","b"))
        now=dt.datetime(2026,10,23,16,5,tzinfo=dt.timezone.utc)
        result=io.StringIO()
        with redirect_stdout(result):
            self.assertTrue(inspect(c,"TV",30,current=now))
        self.assertIn("searchtimers.view@default: 1",result.getvalue())
        self.assertIn("2026-10-23T16:00:00+00:00",result.getvalue())
        c.close()

    def test_existing_fixture_inventory_bounded_counts_only(self):
        from inspect_existing_fixtures import inventory
        import sqlite3
        import io
        from contextlib import redirect_stdout
        conn=sqlite3.connect(":memory:")
        conn.execute("CREATE TABLE timer_assignments (timer_assignment_id TEXT, backend_id TEXT, state TEXT, secret TEXT)")
        conn.execute("CREATE TABLE mutation_operations (operation_id TEXT, backend_id TEXT, state TEXT, secret TEXT)")
        conn.execute("CREATE TABLE native_timer_bindings (native_timer_binding_id TEXT, backend_id TEXT, ownership TEXT, secret TEXT)")
        conn.executemany("INSERT INTO timer_assignments VALUES (?,?,?,?)", [
            ("assign-private","default","bound","SENSITIVE"),
            ("assign-other","other","selected","SENSITIVE")])
        conn.execute("INSERT INTO mutation_operations VALUES (?,?,?,?)",
                     ("op-private","default","succeeded","SENSITIVE"))
        conn.execute("INSERT INTO native_timer_bindings VALUES (?,?,?,?)",
                     ("native-private","default","managed","SENSITIVE"))
        output=io.StringIO()
        with redirect_stdout(output):
            inventory(conn,"default")
        text=output.getvalue()
        self.assertIn("TIMER_ASSIGNMENTS: 1 existing",text)
        self.assertIn("MUTATION_OPERATIONS: 1 existing",text)
        self.assertIn("NATIVE_TIMER_BINDINGS: 1 existing",text)
        self.assertIn("HTTP_AUTHORIZATION_AND_NATIVE_READBACK: NOT_VERIFIED",text)
        self.assertNotIn("SENSITIVE",text)
        self.assertNotIn("assign-private",text)
        self.assertNotIn("op-private",text)
        self.assertNotIn("assign-other",text)
        conn.close()

    def test_existing_fixture_inventory_absent_schema_is_explicit(self):
        from inspect_existing_fixtures import inventory
        from inspect_device_auth_audit import AuditBlocked
        import sqlite3
        import io
        from contextlib import redirect_stdout
        conn=sqlite3.connect(":memory:")
        output=io.StringIO()
        with redirect_stdout(output):
            inventory(conn)
        self.assertIn("TIMER_ASSIGNMENTS: SCHEMA_NOT_INSTALLED",output.getvalue())
        self.assertIn("MUTATION_OPERATIONS: SCHEMA_NOT_INSTALLED",output.getvalue())
        self.assertIn("NATIVE_TIMER_BINDINGS: SCHEMA_NOT_INSTALLED",output.getvalue())
        with self.assertRaises(AuditBlocked):
            inventory(conn,"../invalid")
        conn.close()

    def test_timer_operation_consistency_links_and_24h_age(self):
        from inspect_timer_operation_consistency import correlate
        import sqlite3
        import io
        from contextlib import redirect_stdout
        db=sqlite3.connect(":memory:")
        db.execute("CREATE TABLE timer_assignments (timer_assignment_id TEXT,native_timer_binding_id TEXT,backend_generation INTEGER,state TEXT,backend_id TEXT,updated_at INTEGER)")
        db.execute("CREATE TABLE native_timer_bindings (native_timer_binding_id TEXT,timer_assignment_id TEXT,backend_generation INTEGER,ownership TEXT,backend_id TEXT,last_verified_operation_id TEXT,missing_since INTEGER,drift_state TEXT)")
        db.execute("CREATE TABLE mutation_operations (operation_id TEXT,backend_id TEXT,state TEXT,updated_at INTEGER,deadline INTEGER)")
        db.executemany("INSERT INTO timer_assignments VALUES (?,?,?,?,?,?)",[
            ("assignment-secret","bind-secret",1,"bound","default",100),
            ("assignment-pending","",1,"provisioning","default",100),
            ("assignment-other","",1,"provisioning","other",100)])
        db.execute("INSERT INTO native_timer_bindings VALUES (?,?,?,?,?,?,?,?)",
                   ("bind-secret","assignment-secret",1,"managed","default","op-done",0,"none"))
        db.executemany("INSERT INTO mutation_operations VALUES (?,?,?,?,?)",[
            ("op-done","default","succeeded",100,200),
            ("op-unknown","default","outcome_unknown",100,200),
            ("op-other","other","outcome_unknown",100,200)])
        output=io.StringIO()
        with redirect_stdout(output):
            values=correlate(db,"default",now=200000)
        self.assertEqual(values["BOUND_ASSIGNMENTS"],1)
        self.assertEqual(values["BOUND_ASSIGNMENTS_WITHOUT_MATCHING_MANAGED_BINDING"],0)
        self.assertEqual(values["MANAGED_BINDINGS_WITHOUT_RECIPROCAL_BOUND_ASSIGNMENT"],0)
        self.assertEqual(values["PROVISIONING_UPDATED_OVER_24H_AGO"],1)
        self.assertEqual(values["OUTCOME_UNKNOWN_OPERATIONS"],1)
        self.assertEqual(values["OUTCOME_UNKNOWN_PAST_DECLARED_DEADLINE"],1)
        self.assertEqual(values["MANAGED_BINDINGS_WITHOUT_SUCCEEDED_VERIFICATION_OPERATION"],0)
        self.assertIn("BOUND_RELATIONSHIP: CONSISTENT_PERSISTED_LINKS",output.getvalue())
        for secret in ("assignment-secret","bind-secret","op-unknown"):
            self.assertNotIn(secret,output.getvalue())
        db.close()

    def test_timer_operation_consistency_detects_broken_reciprocity_without_ids(self):
        from inspect_timer_operation_consistency import correlate
        import sqlite3
        import io
        from contextlib import redirect_stdout
        db=sqlite3.connect(":memory:")
        db.execute("CREATE TABLE timer_assignments (timer_assignment_id TEXT,native_timer_binding_id TEXT,backend_generation INTEGER,state TEXT,backend_id TEXT,updated_at INTEGER)")
        db.execute("CREATE TABLE native_timer_bindings (native_timer_binding_id TEXT,timer_assignment_id TEXT,backend_generation INTEGER,ownership TEXT,backend_id TEXT,last_verified_operation_id TEXT,missing_since INTEGER,drift_state TEXT)")
        db.execute("CREATE TABLE mutation_operations (operation_id TEXT,backend_id TEXT,state TEXT,updated_at INTEGER,deadline INTEGER)")
        db.execute("INSERT INTO timer_assignments VALUES (?,?,?,?,?,?)",
                   ("assignment1","binding_missing",1,"bound","default",100))
        db.execute("INSERT INTO native_timer_bindings VALUES (?,?,?,?,?,?,?,?)",
                   ("binding_another","assignment1",2,"managed","default","operation_absent",0,"none"))
        output=io.StringIO()
        with redirect_stdout(output):
            values=correlate(db,"default",now=200000)
        self.assertEqual(values["BOUND_ASSIGNMENTS_WITHOUT_MATCHING_MANAGED_BINDING"],1)
        self.assertEqual(values["MANAGED_BINDINGS_WITHOUT_RECIPROCAL_BOUND_ASSIGNMENT"],1)
        self.assertEqual(values["MANAGED_BINDINGS_WITHOUT_SUCCEEDED_VERIFICATION_OPERATION"],1)
        self.assertIn("BOUND_RELATIONSHIP: NEEDS_INVESTIGATION",output.getvalue())
        self.assertNotIn("binding_missing",output.getvalue())
        db.close()

    def test_timer_operation_consistency_requires_all_canonical_tables(self):
        from inspect_timer_operation_consistency import correlate
        from inspect_device_auth_audit import AuditBlocked
        import sqlite3
        db=sqlite3.connect(":memory:")
        with self.assertRaises(AuditBlocked):
            correlate(db)
        db.close()

    def test_persisted_timer_anomaly_flags_are_nonadditive_and_no_id_leaks(self):
        from inspect_persisted_timer_anomalies import classify
        import sqlite3
        import io
        from contextlib import redirect_stdout
        db=sqlite3.connect(":memory:")
        db.execute("CREATE TABLE timer_assignments (timer_assignment_id TEXT,backend_id TEXT,backend_generation INT,state TEXT,updated_at INT,native_timer_binding_id TEXT)")
        db.execute("CREATE TABLE native_timer_bindings (native_timer_binding_id TEXT,timer_assignment_id TEXT,backend_id TEXT,backend_generation INT,ownership TEXT,last_verified_operation_id TEXT,missing_since INT,drift_state TEXT)")
        db.execute("CREATE TABLE mutation_operations (backend_id TEXT,state TEXT,updated_at INT,deadline INT,resource_type TEXT,verification_policy TEXT)")
        db.executemany("INSERT INTO timer_assignments VALUES (?,?,?,?,?,?)",[
            ("timer-secret-1","default",2,"bound",100,"binding-secret-1"),
            ("timer-secret-2","default",2,"bound",100,"binding-secret-2"),
            ("timer-secret-3","default",2,"provisioning",100,""),
            ("other","other",2,"provisioning",100,"")])
        db.executemany("INSERT INTO native_timer_bindings VALUES (?,?,?,?,?,?,?,?)",[
            ("binding-secret-1","timer-secret-1","default",2,"managed","op-success",0,"none"),
            ("binding-secret-2","timer-secret-2","default",2,"managed","",20,"external_delete")])
        db.executemany("INSERT INTO mutation_operations VALUES (?,?,?,?,?,?)",[
            ("default","outcome_unknown",100,300,"NativeTimer","readback_required"),
            ("default","outcome_unknown",100,0,"private-other-identifier","reconciliation_required"),
            ("other","outcome_unknown",100,0,"Recording","none")])
        out=io.StringIO()
        with redirect_stdout(out):
            v=classify(db,"default",now=1000000)
        self.assertEqual(v["bound_factors"],(2,1,1,1))
        self.assertEqual(v["provisioning"],(1,1,0,0))
        self.assertEqual(v["unknown"],(2,2,1,1))
        self.assertEqual(v["resources"]["NativeTimer"],1)
        self.assertEqual(v["resources"]["other"],1)
        self.assertEqual(v["drift"]["external_delete"],1)
        text=out.getvalue()
        self.assertIn("BOUND_DRIFT_EXTERNAL_DELETE: 1",text)
        self.assertIn("DISPATCH_DEADLINE_IS_NOT_RECONCILIATION_DEADLINE: TRUE",text)
        for sensitive in ("timer-secret","binding-secret","private-other-identifier","op-success"):
            self.assertNotIn(sensitive,text)
        db.close()

    def test_persisted_timer_anomaly_missing_schema_fail_closed(self):
        from inspect_persisted_timer_anomalies import classify
        from inspect_device_auth_audit import AuditBlocked
        import sqlite3
        db=sqlite3.connect(":memory:")
        with self.assertRaises(AuditBlocked):
            classify(db)
        db.close()

    def test_outcome_unknown_command_evidence_distinct_by_operation(self):
        from inspect_unknown_operation_delivery import inspect
        import sqlite3
        import io
        from contextlib import redirect_stdout
        db=sqlite3.connect(":memory:")
        db.execute("CREATE TABLE mutation_operations (operation_id TEXT,backend_id TEXT,state TEXT,resource_type TEXT,action_family TEXT,result_reference TEXT)")
        db.execute("CREATE TABLE backend_agent_command_reservations (operation_id TEXT,backend_id TEXT,command_id TEXT)")
        db.execute("CREATE TABLE backend_agent_commands (operation_id TEXT,backend_id TEXT,command_id TEXT,state TEXT,command_type TEXT)")
        db.execute("CREATE TABLE backend_agent_command_receipts (command_id TEXT)")
        db.execute("CREATE TABLE backend_agent_command_results (command_id TEXT,result_category TEXT,verification_state TEXT,dispatch_state TEXT,error_category TEXT,result_evidence TEXT)")
        db.executemany("INSERT INTO mutation_operations VALUES (?,?,?,?,?,?)",[
            ("secret_op1","default","outcome_unknown","SearchTimer","searchtimer.native-cas.delete",""),
            ("secret_op2","default","outcome_unknown","TimerAssignment","timer.create",""),
            ("secret_op3","default","succeeded","TimerAssignment","timer.create",""),
            ("secret_op4","other","outcome_unknown","sensitive_other","sensitive_family","")])
        db.execute("INSERT INTO backend_agent_command_reservations VALUES (?,?,?)",
                   ("secret_op1","default","secret_command"))
        db.executemany("INSERT INTO backend_agent_commands VALUES (?,?,?,?,?)",[
            ("secret_op1","default","secret_command","waiting_reconciliation","timer.delete"),
            ("secret_op1","default","secret_command2","completed","timer.delete"),
            ("secret_op4","other","secret_command3","waiting_reconciliation","private-command-type")])
        db.execute("INSERT INTO backend_agent_command_receipts VALUES (?)",("secret_command",))
        db.executemany("INSERT INTO backend_agent_command_results VALUES (?,?,?,?,?,?)",[
            ("secret_command","outcome_unknown","outcome_unknown","accepted_by_executor","executor_unknown",""),
            ("secret_command2","outcome_unknown","outcome_unknown","accepted_by_executor","executor_unknown","")])
        out=io.StringIO()
        with redirect_stdout(out):
            result=inspect(db,"default")
        self.assertEqual(result["OUTCOME_UNKNOWN_OPERATIONS"],2)
        self.assertEqual(result["WITH_COMMAND_RESERVATION"],1)
        self.assertEqual(result["WITH_BACKEND_AGENT_COMMAND"],1)
        self.assertEqual(result["WITH_WAITING_RECONCILIATION_COMMAND"],1)
        self.assertEqual(result["WITH_COMMAND_RECEIPT"],1)
        self.assertEqual(result["WITH_COMMAND_RESULT"],1)
        self.assertEqual(result["WITH_UNKNOWN_COMMAND_RESULT"],1)
        self.assertEqual(result["WITH_ACCEPTED_BY_EXECUTOR_RESULT"],1)
        self.assertIn("MISSING_COMMAND_EVIDENCE_IS_NOT_PROOF_OF_NO_DISPATCH",out.getvalue())
        self.assertIn("CHAIN resource=TimerAssignment command=no_command dispatch=no_result",out.getvalue())
        self.assertIn("CHAIN resource=SearchTimer command=waiting_reconciliation dispatch=accepted_by_executor",out.getvalue())
        self.assertIn("action=searchtimer.native-cas.delete command_type=timer.delete error=executor_unknown",out.getvalue())
        self.assertIn("CHAIN_GROUP_COUNTS_MAY_OVERLAP_IF_MULTIPLE_COMMANDS: TRUE",out.getvalue())
        for secret in ("secret_op","secret_command"):
            self.assertNotIn(secret,out.getvalue())
        db.close()

    def test_unknown_operation_delivery_schema_absence_fails_closed(self):
        from inspect_unknown_operation_delivery import inspect
        from inspect_device_auth_audit import AuditBlocked
        import sqlite3
        db=sqlite3.connect(":memory:")
        with self.assertRaises(AuditBlocked):
            inspect(db)
        db.close()

    def test_unknown_chain_classifier_does_not_print_untrusted_result_labels(self):
        from inspect_unknown_operation_delivery import inspect
        import sqlite3
        import io
        from contextlib import redirect_stdout
        db=sqlite3.connect(":memory:")
        db.execute("CREATE TABLE mutation_operations (operation_id TEXT,backend_id TEXT,state TEXT,resource_type TEXT,action_family TEXT,result_reference TEXT)")
        db.execute("CREATE TABLE backend_agent_command_reservations (operation_id TEXT,backend_id TEXT,command_id TEXT)")
        db.execute("CREATE TABLE backend_agent_commands (operation_id TEXT,backend_id TEXT,command_id TEXT,state TEXT,command_type TEXT)")
        db.execute("CREATE TABLE backend_agent_command_receipts (command_id TEXT)")
        db.execute("CREATE TABLE backend_agent_command_results (command_id TEXT,result_category TEXT,verification_state TEXT,dispatch_state TEXT,error_category TEXT,result_evidence TEXT)")
        db.execute("INSERT INTO mutation_operations VALUES (?,?,?,?,?,?)",
                   ("secret-id","default","outcome_unknown","PRIVATE_RESOURCE_NAME","SECRET_ACTION","SECRET_REFERENCE"))
        db.execute("INSERT INTO backend_agent_commands VALUES (?,?,?,?,?)",
                   ("secret-id","default","private-command-id","SENSITIVE-COMMAND","SECRET_COMMAND_TYPE"))
        db.execute("INSERT INTO backend_agent_command_results VALUES (?,?,?,?,?,?)",
                   ("private-command-id","SENSITIVE-RESULT","SENSITIVE-VERIFY","SENSITIVE-DISPATCH","SECRET_ERROR","SECRET_EVIDENCE"))
        capture=io.StringIO()
        with redirect_stdout(capture):
            values=inspect(db)
        output=capture.getvalue()
        self.assertEqual(values["OUTCOME_UNKNOWN_OPERATIONS"],1)
        self.assertIn("CHAIN resource=other command=other dispatch=other verification=other result=other",output)
        for secret in ("secret-id","private-command-id","PRIVATE_RESOURCE_NAME","SENSITIVE-","SECRET_ACTION","SECRET_COMMAND_TYPE","SECRET_ERROR","SECRET_REFERENCE","SECRET_EVIDENCE"):
            self.assertNotIn(secret,output)
        db.close()

    def test_unknown_toggle_quarantine_candidate_requires_both_evidence_factors(self):
        from inspect_unknown_operation_delivery import inspect
        import sqlite3
        import io
        from contextlib import redirect_stdout
        db=sqlite3.connect(":memory:")
        db.execute("CREATE TABLE mutation_operations (operation_id TEXT,backend_id TEXT,state TEXT,resource_type TEXT,action_family TEXT,result_reference TEXT)")
        db.execute("CREATE TABLE backend_agent_command_reservations (operation_id TEXT,backend_id TEXT,command_id TEXT)")
        db.execute("CREATE TABLE backend_agent_commands (operation_id TEXT,backend_id TEXT,command_id TEXT,state TEXT,command_type TEXT)")
        db.execute("CREATE TABLE backend_agent_command_receipts (command_id TEXT)")
        db.execute("CREATE TABLE backend_agent_command_results (command_id TEXT,result_category TEXT,verification_state TEXT,dispatch_state TEXT,error_category TEXT,result_evidence TEXT)")
        rows=[
          ("id-a","default","outcome_unknown","NativeTimerBinding","timer.toggle","native-timer-modify:missing-result-evidence"),
          ("id-b","default","outcome_unknown","NativeTimerBinding","timer.toggle","other-reason"),
          ("id-c","default","outcome_unknown","NativeTimerBinding","timer.toggle","native-timer-modify:missing-result-evidence")]
        db.executemany("INSERT INTO mutation_operations VALUES (?,?,?,?,?,?)",rows)
        db.executemany("INSERT INTO backend_agent_commands VALUES (?,?,?,?,?)",[
          ("id-a","default","cmd-a","completed","vdr.timer.toggle"),
          ("id-b","default","cmd-b","completed","vdr.timer.toggle"),
          ("id-c","default","cmd-c","completed","vdr.timer.toggle")])
        db.executemany("INSERT INTO backend_agent_command_results VALUES (?,?,?,?,?,?)",[
          ("cmd-a","rejected","verified","not_started","fenced","PRIVATE_EVIDENCE_A"),
          ("cmd-b","rejected","verified","not_started","fenced","PRIVATE_EVIDENCE_B"),
          ("cmd-c","rejected","verified","not_started","fenced","")])
        out=io.StringIO()
        with redirect_stdout(out):
            result=inspect(db)
        self.assertEqual(result["OUTCOME_UNKNOWN_OPERATIONS"],3)
        output=out.getvalue()
        self.assertIn("WITH_EXACT_EVIDENCE_FREE_QUARANTINE_MARKER: 2",output)
        self.assertIn("WITH_NONEMPTY_TYPED_AGENT_NO_EFFECT_RESULT: 2",output)
        self.assertIn("WITH_BOTH_SAME_OPERATION: 1",output)
        self.assertIn("EXACT_MARKER_UNKNOWN_TOGGLE_OPERATIONS: 2",output)
        self.assertIn("WITH_GENERIC_VERIFIED_REJECTION_BUT_EMPTY_OR_NULL_EVIDENCE: 1",output)
        self.assertIn("WITH_ANY_NONEMPTY_AGENT_RESULT_EVIDENCE: 1",output)
        self.assertIn("TYPED_REJECTION_RECOVERY_GATE: CANDIDATE_ONLY",output)
        self.assertIn("RECOVERY_OR_STATE_CHANGE_PERFORMED: FALSE",output)
        self.assertNotIn("PRIVATE_EVIDENCE",output)
        db.close()


    def test_legacy_generic_rejection_without_typed_evidence_is_blocked(self):
        from inspect_unknown_operation_delivery import inspect
        import sqlite3
        import io
        from contextlib import redirect_stdout
        db=sqlite3.connect(":memory:")
        db.execute("CREATE TABLE mutation_operations (operation_id TEXT,backend_id TEXT,state TEXT,resource_type TEXT,action_family TEXT,result_reference TEXT)")
        db.execute("CREATE TABLE backend_agent_command_reservations (operation_id TEXT,backend_id TEXT,command_id TEXT)")
        db.execute("CREATE TABLE backend_agent_commands (operation_id TEXT,backend_id TEXT,command_id TEXT,state TEXT,command_type TEXT)")
        db.execute("CREATE TABLE backend_agent_command_receipts (command_id TEXT)")
        db.execute("CREATE TABLE backend_agent_command_results (command_id TEXT,result_category TEXT,verification_state TEXT,dispatch_state TEXT,error_category TEXT,result_evidence TEXT)")
        db.execute("INSERT INTO mutation_operations VALUES (?,?,?,?,?,?)",(
          "PRIVATE_OPERATION","default","outcome_unknown","NativeTimerBinding",
          "timer.toggle","native-timer-modify:missing-result-evidence"))
        db.execute("INSERT INTO backend_agent_commands VALUES (?,?,?,?,?)",(
          "PRIVATE_OPERATION","default","PRIVATE_COMMAND","completed","vdr.timer.toggle"))
        db.execute("INSERT INTO backend_agent_command_results VALUES (?,?,?,?,?,?)",(
          "PRIVATE_COMMAND","rejected","verified","not_started","fenced",None))
        output=io.StringIO()
        with redirect_stdout(output):
            values=inspect(db)
        self.assertEqual(values["OUTCOME_UNKNOWN_OPERATIONS"],1)
        rendered=output.getvalue()
        self.assertIn("WITH_EXACT_EVIDENCE_FREE_QUARANTINE_MARKER: 1",rendered)
        self.assertIn("WITH_NONEMPTY_TYPED_AGENT_NO_EFFECT_RESULT: 0",rendered)
        self.assertIn("WITH_BOTH_SAME_OPERATION: 0",rendered)
        self.assertIn("EXACT_MARKER_UNKNOWN_TOGGLE_OPERATIONS: 1",rendered)
        self.assertIn("WITH_GENERIC_VERIFIED_REJECTION_BUT_EMPTY_OR_NULL_EVIDENCE: 1",rendered)
        self.assertIn("WITH_ANY_NONEMPTY_AGENT_RESULT_EVIDENCE: 0",rendered)
        self.assertIn("TYPED_REJECTION_RECOVERY_GATE: BLOCKED",rendered)
        self.assertNotIn("PRIVATE_OPERATION",rendered)
        self.assertNotIn("PRIVATE_COMMAND",rendered)
        db.close()


if __name__ == "__main__":
    unittest.main()
