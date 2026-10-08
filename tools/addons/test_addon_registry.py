"""Offline installed add-on discovery/activation-denial regression tests."""
import importlib.util
import json
import os
import tempfile
import unittest
from pathlib import Path

from test_addon_contract import addon

spec = importlib.util.spec_from_file_location(
    "addon_registry", Path(__file__).with_name("addon_registry.py")
)
registry = importlib.util.module_from_spec(spec)
spec.loader.exec_module(registry)


class RegistryTests(unittest.TestCase):
    def setUp(self):
        self.sandbox = tempfile.TemporaryDirectory(prefix="suite-registry-")
        self.addCleanup(self.sandbox.cleanup)
        self.root = Path(self.sandbox.name)
        self.installed = self.root / "installed"
        self.policy = self.root / "policy"
        self.installed.mkdir()
        self.policy.mkdir()
        self.uid = os.geteuid()

    def install(self, name="rectools"):
        folder = self.installed / name
        folder.mkdir()
        for filename in addon.FILES:
            (folder / filename).write_bytes(
                (addon.ROOT / "modules" / name / filename).read_bytes()
            )
        return folder

    def policy_file(self, name="rectools", enabled=True):
        path = self.policy / (name + ".json")
        path.write_text(json.dumps({
            "schemaVersion": 1, "module": name, "enabled": enabled
        }))
        return path

    def scan(self):
        return registry.discover(self.installed, self.policy, self.uid)

    def test_absence_yields_no_capabilities(self):
        self.assertEqual(self.scan()["modules"], [])
        self.assertEqual(self.scan()["enabledCapabilities"], [])

    def test_all_metadata_packages_present_but_never_active(self):
        for name in ("rectools", "image", "music", "tvscraper"):
            self.install(name)
            self.policy_file(name)
        result = self.scan()
        self.assertEqual(len(result["modules"]), 4)
        self.assertEqual(result["enabledCapabilities"], [])
        for item in result["modules"]:
            self.assertTrue(item["installed"])
            self.assertTrue(item["requestedEnabled"])
            self.assertFalse(item["effectiveEnabled"])
            self.assertEqual(item["reason"], "scaffold_not_executable")
            self.assertEqual(item["capabilities"], [])

    def test_missing_policy_is_disabled(self):
        self.install()
        item = self.scan()["modules"][0]
        self.assertFalse(item["requestedEnabled"])
        self.assertFalse(item["effectiveEnabled"])

    def test_invalid_policy_or_duplicate_fields_deny(self):
        self.install()
        file = self.policy_file()
        file.write_text('{"schemaVersion":1,"module":"rectools","enabled":true,"enabled":false}')
        item = self.scan()["modules"][0]
        self.assertEqual(item["reason"], "invalid_or_untrusted")
        self.assertFalse(item["effectiveEnabled"])

    def test_symlinked_manifest_and_policy_are_rejected(self):
        folder = self.install()
        other = self.root / "fake.json"
        other.write_text((folder / "addon.json").read_text())
        (folder / "addon.json").unlink()
        (folder / "addon.json").symlink_to(other)
        self.assertEqual(self.scan()["modules"][0]["reason"], "invalid_or_untrusted")
        (folder / "addon.json").unlink()
        (folder / "addon.json").write_text(other.read_text())
        policy = self.policy_file()
        policy.unlink()
        policy.symlink_to(other)
        self.assertEqual(self.scan()["modules"][0]["reason"], "invalid_or_untrusted")

    def test_extra_capabilities_and_non_scaffold_rejected(self):
        folder = self.install()
        data = json.loads((folder / "addon.json").read_text())
        data["capabilities"] = ["recordings.import"]
        (folder / "addon.json").write_text(json.dumps(data))
        self.assertFalse(self.scan()["modules"][0]["installed"])
        data["capabilities"] = []
        data["state"] = "active"
        (folder / "addon.json").write_text(json.dumps(data))
        self.assertFalse(self.scan()["modules"][0]["effectiveEnabled"])

    def test_untrusted_permissions_and_duplicate_package_fail_closed(self):
        first = self.install("rectools")
        self.install("image")
        second = self.installed / "image" / "addon.json"
        data = json.loads(second.read_text())
        data["package"] = "vdr-suite-addon-media-tools"
        second.write_text(json.dumps(data))
        report = self.scan()
        self.assertTrue(all(x["reason"] == "invalid_or_untrusted"
                            for x in report["modules"]))
        self.assertTrue(all(not x["effectiveEnabled"] for x in report["modules"]))
        first.chmod(0o777)
        self.assertEqual(
            next(x for x in self.scan()["modules"] if x["id"] == "rectools")["reason"],
            "invalid_or_untrusted",
        )

    def test_world_writable_metadata_refused(self):
        folder = self.install()
        manifest = folder / "addon.json"
        manifest.chmod(0o666)
        item = self.scan()["modules"][0]
        self.assertFalse(item["effectiveEnabled"])
        self.assertEqual(item["reason"], "invalid_or_untrusted")

    def test_untrusted_policy_directory_refused(self):
        self.install()
        self.policy_file()
        self.policy.chmod(0o777)
        item = self.scan()["modules"][0]
        self.assertFalse(item["requestedEnabled"])
        self.assertEqual(item["reason"], "invalid_or_untrusted")

    def test_installed_root_symlink_refused(self):
        other = self.root / "alias"
        other.symlink_to(self.installed, target_is_directory=True)
        with self.assertRaises(registry.ContractError):
            registry.discover(other, self.policy, self.uid)
