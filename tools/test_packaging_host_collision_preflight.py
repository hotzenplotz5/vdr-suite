#!/usr/bin/env python3
"""No-root P6/P8 collision checks. Never inspect or change the real host."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

TOOL = Path(__file__).with_name("packaging_host_collision_preflight.py")
spec = importlib.util.spec_from_file_location("p6_host_preflight", TOOL)
host = importlib.util.module_from_spec(spec)
spec.loader.exec_module(host)


class HostCollisionTests(unittest.TestCase):
    def fixture(self):
        return tempfile.TemporaryDirectory()

    @staticmethod
    def put(root, rel, body="fixture"):
        file = root / rel
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text(body, encoding="utf-8")
        return file

    def test_source_owned_targets_are_manifest_derived(self):
        targets = host.package_targets()
        self.assertEqual(targets["etc/nginx/snippets/vdr-suite.conf"],
                         "vdr-suite-daemon")
        self.assertEqual(targets["etc/vdr-suite/backend-agent.conf"],
                         "vdr-suite-backend-agent")
        self.assertIn("usr/sbin/vdr-suite-daemon", targets)
        self.assertNotIn("usr/share/vdr-suite/", targets)

    def test_empty_private_host_is_never_approved(self):
        with self.fixture() as tmp:
            result = host.inspect(Path(tmp))
            self.assertEqual(result["verdict"], "NO_KNOWN_CONFLICT")
            self.assertFalse(result["install_approved"])
            self.assertEqual(result["checks"], [])

    def test_operator_nginx_snippet_is_hard_block(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            file = self.put(root, "etc/nginx/snippets/vdr-suite.conf",
                            "# operator managed\n")
            before = file.read_bytes()
            result = host.inspect(root)
            self.assertEqual(result["verdict"], "BLOCKED")
            self.assertTrue(any(x["code"] == "UNOWNED_PACKAGE_TARGET" and
                                x["path"] == "/etc/nginx/snippets/vdr-suite.conf"
                                for x in result["checks"]))
            self.assertEqual(file.read_bytes(), before)
            self.assertFalse(result["install_approved"])

    def test_other_package_ownership_blocks_and_same_package_reviews(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            file = self.put(root, "etc/default/vdr-suite-daemon")
            other = host.inspect(root, ownership=lambda _: "some-other-package")
            self.assertIn("OTHER_PACKAGE_OWNS_TARGET",
                          [x["code"] for x in other["checks"]])
            self.assertEqual(other["verdict"], "BLOCKED")
            same = host.inspect(root, ownership=lambda _: "vdr-suite-daemon")
            self.assertEqual(same["verdict"], "REVIEW_REQUIRED")
            self.assertFalse(same["install_approved"])
            self.assertEqual(file.read_text(), "fixture")

    def test_manual_binary_and_persistent_database_trigger_review(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            self.put(root, "usr/local/sbin/vdr-suite-daemon")
            (root / "var/lib/vdr-suite").mkdir(parents=True)
            result = host.inspect(root)
            self.assertEqual(result["verdict"], "REVIEW_REQUIRED")
            self.assertTrue(any(x["path"] == "/var/lib/vdr-suite" for x in
                                result["checks"]))

    def test_systemd_dropin_directories_require_review(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            for unit in ("vdr-suite-daemon", "vdr-suite-backend-agent"):
                self.put(root, "etc/systemd/system/" + unit +
                         ".service.d/operator.conf", "# operator override")
            result = host.inspect(root)
            self.assertEqual(result["verdict"], "REVIEW_REQUIRED")
            for unit in ("vdr-suite-daemon", "vdr-suite-backend-agent"):
                self.assertIn(
                    {"code": "EXISTING_MANUAL_OR_STATE_PATH",
                     "path": "/etc/systemd/system/" + unit + ".service.d",
                     "severity": "REVIEW"}, result["checks"])

    def test_active_nginx_route_is_detected_without_disclosing_config(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            content = ("server { location ^~ /vdr-suite/ { "
                       "proxy_pass http://127.0.0.1:18080/; } }\n"
                       "# confidential-test-password-value")
            file = self.put(root, "etc/nginx/sites-enabled/live.conf", content)
            result = host.inspect(root)
            self.assertTrue(any(x["code"] == "NGINX_EXISTING_VDR_SUITE_ROUTE"
                                for x in result["checks"]))
            self.assertNotIn("confidential-test-password-value",
                             json.dumps(result))
            self.assertEqual(file.read_text(), content)

    def test_nginx_multiline_route_and_other_prefix(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            file = self.put(root, "etc/nginx/sites-enabled/routes.conf",
                            "server {\n  location = /vdr-suite { return 308 /vdr-suite/; }\n"
                            "  location /different-app/ { return 200; }\n}\n")
            result = host.inspect(root)
            self.assertTrue(any(x["code"] == "NGINX_EXISTING_VDR_SUITE_ROUTE"
                                for x in result["checks"]))
            self.assertIn("location /different-app", file.read_text())

    def test_nginx_relative_symlink_is_reviewed_within_fixture(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            target = self.put(root, "etc/nginx/sites-available/current.conf",
                              "location = /vdr-suite { return 308 /vdr-suite/; }")
            link = root / "etc/nginx/sites-enabled/current.conf"
            link.parent.mkdir(parents=True)
            link.symlink_to("../sites-available/current.conf")
            result = host.inspect(root)
            self.assertTrue(any(x["code"] == "NGINX_EXISTING_VDR_SUITE_ROUTE"
                                for x in result["checks"]))
            self.assertTrue(target.exists())

    def test_symlinked_package_target_blocks(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            target = self.put(root, "other.conf")
            dest = root / "etc/nginx/snippets/vdr-suite.conf"
            dest.parent.mkdir(parents=True)
            dest.symlink_to(target)
            result = host.inspect(root)
            self.assertEqual(result["verdict"], "BLOCKED")
            self.assertIn("TARGET_NOT_REGULAR_FILE",
                          [x["code"] for x in result["checks"]])
            self.assertTrue(dest.is_symlink())

    def test_ownership_error_fails_closed(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            self.put(root, "usr/sbin/vdr-suite-daemon")
            with mock.patch.object(host, "dpkg_owner") as patched:
                patched.side_effect = ValueError("unknown")
                outcome = host.inspect(root, ownership=host.dpkg_owner)
            self.assertEqual(outcome["verdict"], "BLOCKED")
            self.assertIn("OWNERSHIP_UNVERIFIED",
                          [x["code"] for x in outcome["checks"]])

    def test_missing_known_manifest_target_is_rejected(self):
        with self.fixture() as tmp:
            root = Path(tmp)
            manifest_root = root / "repo"
            self.put(manifest_root, "debian/vdr-suite-daemon.install",
                     "usr/sbin/vdr-suite-daemon\n")
            self.put(manifest_root, "debian/vdr-suite-backend-agent.install",
                     "etc/vdr-suite/backend-agent.conf\n")
            self.put(manifest_root, "debian/vdr-plugin-suitebridge.install",
                     "usr/lib/tmpfiles.d/vdr-suite-live.conf\n")
            with self.assertRaisesRegex(ValueError, "critical Debian"):
                host.package_targets(manifest_root)


if __name__ == "__main__":
    unittest.main()
