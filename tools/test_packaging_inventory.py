#!/usr/bin/env python3
"""No-root tests for the packaging stage inventory."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

TOOL = Path(__file__).with_name("packaging_inventory.py")
spec = importlib.util.spec_from_file_location("packaging_inventory", TOOL)
inventory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inventory)

class InventoryTests(unittest.TestCase):
    def test_stage_inventory(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            runtime = base / "runtime"
            plugin = base / "plugin"
            daemon = runtime / "usr/sbin/vdr-suite-daemon"
            bridge = plugin / "usr/lib/vdr/plugins/libvdr-suitebridge.so.2.6.0"
            agent = runtime / "usr/sbin/vdr-suite-backend-agent"
            for file in (daemon, bridge, agent):
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_bytes(b"not-elf")
            records = inventory.inspect(runtime, "runtime") + inventory.inspect(plugin, "plugin")
            owners = {r["path"]: r["package"] for r in records}
            self.assertEqual(owners["/usr/sbin/vdr-suite-daemon"], "vdr-suite-daemon")
            self.assertEqual(owners["/usr/sbin/vdr-suite-backend-agent"], "vdr-suite-backend-agent")
            self.assertEqual(owners["/usr/lib/vdr/plugins/libvdr-suitebridge.so.2.6.0"], "vdr-plugin-suitebridge")
            self.assertEqual(len(records), 3)
            self.assertEqual(records[0]["mode"], "0644")

    def test_fail_closed_unknown_plugin_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            (stage / "unexpected").write_text("no", encoding="utf-8")
            with self.assertRaises(ValueError):
                inventory.inspect(stage, "plugin")

    def test_symlinks_are_not_followed(self):
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            link = stage / "usr/sbin/vdr-suite-daemon"
            link.parent.mkdir(parents=True)
            link.symlink_to("/nonexistent")
            records = inventory.inspect(stage, "runtime")
            with self.assertRaises(ValueError):
                inventory.inspect(stage, "runtime")

    def test_agent_unit_and_config(self):
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            files = ["lib/systemd/system/vdr-suite-backend-agent.service",
                     "etc/vdr-suite/backend-agent.conf"]
            for name in files:
                target = stage / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text("example", encoding="utf-8")
            entries = inventory.inspect(stage, "runtime")
            self.assertTrue(all(x["package"] == "vdr-suite-backend-agent" for x in entries))
            self.assertTrue(any(x.get("config_candidate") for x in entries))

    def test_reject_secret_payload_and_world_write(self):
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            secret = stage / "var/lib/vdr-suite/secrets/token"
            secret.parent.mkdir(parents=True)
            secret.write_text("secret")
            with self.assertRaises(ValueError):
                inventory.inspect(stage, "runtime")
            secret.unlink()
            file = stage / "usr/sbin/vdr-suite-daemon"
            file.parent.mkdir(parents=True)
            file.write_bytes(b"fake")
            file.chmod(0o666)
            with self.assertRaises(ValueError):
                inventory.inspect(stage, "runtime")

if __name__ == "__main__":
    unittest.main()
