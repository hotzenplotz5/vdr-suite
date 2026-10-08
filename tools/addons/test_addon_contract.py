"""Focused module validation and source staging without live VDR effects."""
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "addon_contract", Path(__file__).with_name("addon_contract.py")
)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)


class AddonTests(unittest.TestCase):
    def test_modules_are_inert_and_have_rules(self):
        manifests = addon.validate_all()
        self.assertEqual(set(manifests), {"rectools", "image", "music", "tvscraper"})
        self.assertEqual(manifests["rectools"]["package"], "vdr-suite-addon-media-tools")
        for name, manifest in manifests.items():
            self.assertEqual(manifest["state"], "scaffold")
            self.assertEqual(manifest["entrypoints"], [])
            self.assertEqual(manifest["capabilities"], [])
            self.assertIn("root AGENTS.md", (
                addon.ROOT / "modules" / name / "AGENTS.md"
            ).read_text())

    def test_stage_exact_files_and_reject_overwrite(self):
        with tempfile.TemporaryDirectory(prefix="addon-stage-test-") as directory:
            dest = addon.stage(addon.ROOT, "rectools", directory)
            self.assertEqual(dest, Path(directory) / "usr/share/vdr-suite/addons/rectools")
            self.assertEqual({p.name for p in dest.iterdir()}, set(addon.FILES))
            for name in addon.FILES:
                self.assertEqual((dest / name).read_bytes(),
                                 (addon.ROOT / "modules/rectools" / name).read_bytes())
                self.assertEqual((dest / name).stat().st_mode & 0o777, 0o644)
            with self.assertRaises(addon.ContractError):
                addon.stage(addon.ROOT, "rectools", directory)

    def test_unsafe_roots_and_prefix_are_rejected(self):
        for path in ("", "/", "/usr", "/tmp", "/etc", "relative", "/tmp/../etc"):
            with self.subTest(path=path):
                with self.assertRaises(addon.ContractError):
                    addon.stage(addon.ROOT, "music", path)
        with tempfile.TemporaryDirectory(prefix="addon-stage-test-") as directory:
            with self.assertRaises(addon.ContractError):
                addon.stage(addon.ROOT, "image", directory, "/usr/../etc")
            with self.assertRaises(addon.ContractError):
                addon.stage(addon.ROOT, "unknown", directory)

    def test_no_runtime_fields_allowed_in_scaffold(self):
        with tempfile.TemporaryDirectory(prefix="addon-source-test-") as directory:
            folder = Path(directory) / "modules/example"
            folder.mkdir(parents=True)
            (folder / "README.md").write_text("example\n")
            (folder / "AGENTS.md").write_text("rules\n")
            data = {
                "schemaVersion": 1, "id": "example", "version": "0.1.0",
                "package": "vdr-suite-addon-example", "state": "scaffold",
                "displayName": "Example", "description": "No runtime",
                "entrypoints": [], "capabilities": ["import"], "permissions": [],
                "dependencies": [],
            }
            (folder / "addon.json").write_text(json.dumps(data))
            with self.assertRaises(addon.ContractError):
                addon.validate_module(Path(directory), "example")
            data["capabilities"] = []
            (folder / "addon.json").write_text(json.dumps(data))
            self.assertEqual(addon.validate_module(Path(directory), "example")["id"], "example")
            (folder / "AGENTS.md").unlink()
            (folder / "AGENTS.md").symlink_to("README.md")
            with self.assertRaises(addon.ContractError):
                addon.validate_module(Path(directory), "example")

    def test_duplicate_json_keys_rejected(self):
        with self.assertRaises(addon.ContractError):
            json.loads('{"id":"a","id":"b"}', object_pairs_hook=addon.unique_keys)

    def test_stage_symlink_rejected(self):
        with tempfile.TemporaryDirectory(prefix="addon-stage-test-") as directory:
            root = Path(directory)
            real = root / "real"
            real.mkdir()
            link = root / "link"
            link.symlink_to(real, target_is_directory=True)
            with self.assertRaises(addon.ContractError):
                addon.stage(addon.ROOT, "music", str(link))
