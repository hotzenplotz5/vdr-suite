"""Fail-closed dpkg evidence diagnostics: no system dpkg reads or changes."""
import hashlib
import json
import os
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

from addon_contract import FILES, ROOT, stage
from dpkg_evidence import examine
from installed_registry import inspect, plan_enable


class DpkgEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="addon-dpkg-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.module = "rectools"
        self.package = "vdr-suite-addon-media-tools"
        self.staged = stage(ROOT, self.module, str(self.root))
        self.ledger = self.root / "var/lib/dpkg"
        self.info = self.ledger / "info"
        self.info.mkdir(parents=True)
        self._write_ledger()

    def _write_ledger(self, version="0.1.0~scaffold1",
                      architecture="all", status="install ok installed"):
        (self.ledger / "status").write_text(
            f"Package: {self.package}\n"
            f"Status: {status}\n"
            f"Version: {version}\n"
            f"Architecture: {architecture}\n"
            "Description: Inactive source metadata\n\n",
            encoding="utf-8",
        )
        prefix = f"usr/share/vdr-suite/addons/{self.module}/"
        (self.info / (self.package + ".list")).write_text(
            "/.\n/usr\n/usr/share\n/usr/share/vdr-suite\n"
            + "".join("/" + prefix + f + "\n" for f in FILES),
            encoding="utf-8",
        )
        (self.info / (self.package + ".md5sums")).write_text(
            "".join(
                hashlib.md5((self.staged / f).read_bytes()).hexdigest()
                + "  " + prefix + f + "\n"
                for f in FILES
            ),
            encoding="utf-8",
        )

    def _evidence(self):
        inventory = inspect(self.root, verify_dpkg=True)
        self.assertEqual(len(inventory["modules"]), 3)
        record = next(m for m in inventory["modules"] if m["id"] == "rectools")
        self.assertFalse(record["active"])
        self.assertEqual(record["capabilities"], [])
        self.assertTrue(record["installed"])  # source metadata, NOT package trust
        self.assertNotIn("packageTrusted", record)
        return record["dpkgEvidence"]

    def test_ledger_matches_but_never_activates(self):
        self.assertEqual(
            self._evidence(),
            {"consistent": True, "reason": "dpkg-ledger-consistent"},
        )
        self.assertNotIn("dpkgEvidence", inspect(self.root)["modules"][-1])
        self.assertEqual(plan_enable(self.root, "rectools", verify_dpkg=True)["decision"],
                         "denied")

    def test_explicit_cli_and_no_file_writes(self):
        import subprocess
        script = Path(__file__).with_name("installed_registry.py")
        before = {
            str(p.relative_to(self.root)): p.read_bytes()
            for p in self.root.rglob("*") if p.is_file()
        }
        process = subprocess.run(
            ["python3", str(script), "inventory", "--root", str(self.root),
             "--verify-dpkg"], capture_output=True, text=True, check=True,
        )
        records = json.loads(process.stdout)["modules"]
        self.assertEqual(
            next(m for m in records if m["id"] == "rectools")["dpkgEvidence"]["consistent"],
            True,
        )
        after = {
            str(p.relative_to(self.root)): p.read_bytes()
            for p in self.root.rglob("*") if p.is_file()
        }
        self.assertEqual(before, after)

    def test_default_scan_does_not_probe_dpkg(self):
        # Suite may be installed manually, with no local Debian add-on
        # package. The default registry and plan must remain independent
        # from this optional verifier.
        with patch("installed_registry.examine",
                   side_effect=AssertionError("unexpected dpkg probe")):
            inventory = inspect(self.root)
            self.assertNotIn("dpkgEvidence",
                             next(m for m in inventory["modules"]
                                  if m["id"] == "rectools"))
            decision = plan_enable(self.root, "rectools")
            self.assertEqual(decision["decision"], "denied")

    def test_missing_status_fails_closed(self):
        (self.ledger / "status").unlink()
        self.assertFalse(self._evidence()["consistent"])

    def test_not_installed_status_fails_closed(self):
        self._write_ledger(status="deinstall ok config-files")
        self.assertEqual(self._evidence()["reason"], "package-not-installed")

    def test_wrong_version_and_architecture_fails_closed(self):
        self._write_ledger(version="999.0.0")
        self.assertEqual(self._evidence()["reason"], "package-version-mismatch")
        self._write_ledger(architecture="amd64")
        self.assertEqual(self._evidence()["reason"], "package-architecture-mismatch")

    def test_cpp_preview_version_accepted_only_as_inert_record(self):
        self._write_ledger(version="0.1.0~cpppreview1", architecture="amd64")
        self.assertTrue(self._evidence()["consistent"])
        self.assertEqual(plan_enable(self.root, "rectools", verify_dpkg=True)["decision"],
                         "denied")

    def test_incomplete_file_ownership_fails_closed(self):
        path = self.info / (self.package + ".list")
        text = path.read_text()
        path.write_text(text.replace("/usr/share/vdr-suite/addons/rectools/addon.json\n", ""))
        self.assertEqual(self._evidence()["reason"], "metadata-not-dpkg-owned")

    def test_missing_checksum_record_fails_closed(self):
        path = self.info / (self.package + ".md5sums")
        path.write_text("")
        self.assertFalse(self._evidence()["consistent"])

    def test_modified_file_fails_closed(self):
        (self.staged / "README.md").write_text("altered, still nonempty\n")
        self.assertEqual(self._evidence()["reason"], "metadata-checksum-mismatch")

    def test_changed_local_ledger_is_not_cryptographic_trust(self):
        # A privileged attacker can rewrite BOTH a file and local dpkg ledger.
        # "consistent" is therefore deliberately not "trusted".
        (self.staged / "README.md").write_text("locally replaced\n")
        self._write_ledger()
        self.assertTrue(self._evidence()["consistent"])

    def test_duplicate_status_and_checksum_rejected(self):
        path = self.ledger / "status"
        path.write_text(path.read_text() * 2)
        self.assertEqual(self._evidence()["reason"], "package-absent-or-duplicate")
        self._write_ledger()
        path = self.info / (self.package + ".md5sums")
        path.write_text(path.read_text() * 2)
        self.assertEqual(self._evidence()["reason"], "invalid-dpkg-checksums")

    def test_symlink_ledger_components_rejected(self):
        path = self.ledger / "status"
        path.rename(self.ledger / "status-real")
        path.symlink_to("status-real")
        self.assertEqual(self._evidence()["reason"], "unsafe-ledger-path")
        path.unlink()
        self._write_ledger()
        old_info = self.ledger / "previous-info"
        self.info.rename(old_info)
        self.info.symlink_to(old_info, target_is_directory=True)
        self.assertEqual(self._evidence()["reason"], "unsafe-ledger-path")

    def test_no_tvscraper_extension_even_if_ledger_exists(self):
        self.assertEqual(plan_enable(self.root, "tvscraper", verify_dpkg=True)["reason"],
                         "unregistered-package")
        result = examine(self.root, "tvscraper", "vdr-suite-addon-tvscraper", "0.1.0")
        self.assertEqual(result["reason"], "unregistered-module")

    def test_isolated_root_absence_never_reads_host_dpkg(self):
        with tempfile.TemporaryDirectory(prefix="addon-blank-dpkg-") as tmp:
            root = Path(tmp)
            stage(ROOT, "image", str(root))
            evidence = inspect(root, verify_dpkg=True)
            entry = next(m for m in evidence["modules"] if m["id"] == "image")
            self.assertFalse(entry["dpkgEvidence"]["consistent"])
            self.assertEqual(entry["dpkgEvidence"]["reason"],
                             "missing-or-unreadable-ledger")


if __name__ == "__main__":
    unittest.main()
