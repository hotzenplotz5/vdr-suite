"""Registry discovery and fail-closed activation tests (no live filesystem)."""
import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

from test_addon_contract import addon

spec = importlib.util.spec_from_file_location(
    "installed_registry", Path(__file__).with_name("installed_registry.py")
)
registry = importlib.util.module_from_spec(spec)
spec.loader.exec_module(registry)


class InstalledAddonRegistryTests(unittest.TestCase):
    def _stage(self, root, name):
        return addon.stage(addon.ROOT, name, str(root))

    def test_missing_catalog_keeps_every_module_disabled(self):
        with tempfile.TemporaryDirectory(prefix="addon-registry-test-") as tmp:
            result = registry.inspect(Path(tmp))
            self.assertEqual(result["registrySchemaVersion"], 1)
            self.assertEqual(len(result["modules"]), 4)
            self.assertTrue(all(not m["installed"] and not m["active"] for m in result["modules"]))
            self.assertEqual(registry.plan_enable(Path(tmp), "rectools")["reason"], "missing-package")

    def test_staged_package_discovered_without_authority(self):
        with tempfile.TemporaryDirectory(prefix="addon-registry-test-") as tmp:
            root = Path(tmp)
            for name in registry.SUPPORTED_PACKAGES:
                self._stage(root, name)
            data = registry.inspect(root)
            self.assertEqual(set(m["id"] for m in data["modules"]), set(registry.SUPPORTED_PACKAGES))
            for item in data["modules"]:
                self.assertTrue(item["installed"])
                self.assertEqual(item["state"], "installed-disabled")
                self.assertEqual(item["capabilities"], [])
                self.assertFalse(item["active"])
                self.assertEqual(registry.plan_enable(root, item["id"])["decision"], "denied")
                self.assertEqual(registry.plan_enable(root, item["id"])["reason"],
                                 "no-authorized-runtime-handler")

    def test_unregistered_package_does_not_self_register(self):
        with tempfile.TemporaryDirectory(prefix="addon-registry-test-") as tmp:
            root = Path(tmp)
            catalog = root / "usr/share/vdr-suite/addons"
            catalog.mkdir(parents=True)
            (catalog / "malicious").mkdir()
            result = registry.inspect(root)
            rejected = next(m for m in result["modules"] if m["id"] == "malicious")
            self.assertEqual(rejected["reason"], "unregistered-package")
            self.assertFalse(rejected["active"])
            self.assertEqual(registry.plan_enable(root, "malicious")["reason"], "unregistered-package")

    def test_tampered_manifest_rejected(self):
        with tempfile.TemporaryDirectory(prefix="addon-registry-test-") as tmp:
            root = Path(tmp)
            location = self._stage(root, "image")
            manifest = json.loads((location / "addon.json").read_text())
            manifest["package"] = "vdr-suite-addon-music"
            (location / "addon.json").write_text(json.dumps(manifest))
            item = next(m for m in registry.inspect(root)["modules"] if m["id"] == "image")
            self.assertEqual(item["reason"], "invalid-package-manifest")
            self.assertFalse(item["installed"])

    def test_manifest_cannot_self_activate_or_invent_permissions(self):
        with tempfile.TemporaryDirectory(prefix="addon-registry-test-") as tmp:
            root = Path(tmp)
            location = self._stage(root, "music")
            manifest = json.loads((location / "addon.json").read_text())
            manifest["capabilities"] = ["admin"]
            manifest["permissions"] = ["accounts.manage"]
            (location / "addon.json").write_text(json.dumps(manifest))
            item = next(m for m in registry.inspect(root)["modules"] if m["id"] == "music")
            self.assertEqual(item["reason"], "invalid-package-manifest")
            self.assertEqual(item["capabilities"], [])
            self.assertFalse(item["active"])

    def test_symlinked_module_and_files_fail_closed(self):
        with tempfile.TemporaryDirectory(prefix="addon-registry-test-") as tmp:
            root = Path(tmp)
            self._stage(root, "rectools")
            catalog = root / "usr/share/vdr-suite/addons"
            folder = catalog / "rectools"
            (folder / "AGENTS.md").unlink()
            (folder / "AGENTS.md").symlink_to("README.md")
            result = registry.inspect(root)
            target = next(m for m in result["modules"] if m["id"] == "rectools")
            self.assertEqual(target["reason"], "invalid-package-manifest")
            folder.rename(catalog / "old-folder")
            folder.symlink_to(catalog / "old-folder", target_is_directory=True)
            result = registry.inspect(root)
            target = next(m for m in result["modules"] if m["id"] == "rectools")
            self.assertEqual(target["reason"], "invalid-package-directory")
            self.assertFalse(target["active"])

    def test_symlinked_catalog_root_and_relative_root_rejected(self):
        with tempfile.TemporaryDirectory(prefix="addon-registry-test-") as tmp:
            root = Path(tmp)
            (root / "usr").symlink_to(root / "other", target_is_directory=True)
            with self.assertRaises(addon.ContractError):
                registry.inspect(root)
            with self.assertRaises(addon.ContractError):
                registry.inspect(Path("relative"))

    def test_deterministic_cli_read_only_output(self):
        with tempfile.TemporaryDirectory(prefix="addon-registry-test-") as tmp:
            root = Path(tmp)
            self._stage(root, "tvscraper")
            script = Path(__file__).with_name("installed_registry.py")
            result = subprocess.run(
                ["python3", str(script), "inventory", "--root", str(root)],
                text=True, capture_output=True, check=True)
            payload = json.loads(result.stdout)
            self.assertEqual(len(payload["modules"]), 4)
            self.assertFalse(any(m["active"] for m in payload["modules"]))
            before = sorted(str(p) for p in root.rglob("*"))
            decision = subprocess.run(
                ["python3", str(script), "plan-enable", "--root", str(root),
                 "--module", "tvscraper"], text=True, capture_output=True, check=True)
            self.assertEqual(json.loads(decision.stdout)["decision"], "denied")
            self.assertEqual(before, sorted(str(p) for p in root.rglob("*")))


if __name__ == "__main__":
    unittest.main()
