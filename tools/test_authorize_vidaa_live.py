import contextlib
import importlib.util
import io
import pathlib
import unittest
from unittest import mock


PATH = pathlib.Path(__file__).with_name("authorize_vidaa_live.py")
SPEC = importlib.util.spec_from_file_location("authorize_vidaa_live", PATH)
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


class GrantToolSafetyTests(unittest.TestCase):
    def test_only_https_is_allowed(self):
        with self.assertRaisesRegex(MOD.Stop, "HTTPS_WEB_BASE_REQUIRED"):
            MOD.validated_base("http://127.0.0.1:18080/vdr-suite")
        with self.assertRaisesRegex(MOD.Stop, "HTTPS_WEB_BASE_REQUIRED"):
            MOD.validated_base("https://admin:secret@example.invalid/vdr-suite")
        self.assertEqual(
            MOD.validated_base("https://server.example/vdr-suite/"),
            "https://server.example/vdr-suite")

    def test_dry_run_never_prompts_for_credentials_or_calls_network(self):
        out = io.StringIO()
        with mock.patch.object(MOD.urllib.request, "build_opener",
                               side_effect=AssertionError("network forbidden")), \
             mock.patch("builtins.input",
                        side_effect=AssertionError("prompt forbidden")), \
             contextlib.redirect_stdout(out):
            result = MOD.main([
                "--base-url", "https://server.example/vdr-suite",
                "--device-id", "device_" + "f" * 32,
                "--backend-id", "default"])
        self.assertEqual(result, 0)
        self.assertIn("DRY_RUN=PASS_NO_CHANGES", out.getvalue())
        for permission in MOD.PERMISSIONS:
            self.assertIn(permission, out.getvalue())

    def test_invalid_device_and_backend_fail_closed(self):
        with self.assertRaisesRegex(MOD.Stop, "DEVICE_ID_INVALID"):
            MOD.main(["--base-url", "https://server.example/vdr-suite",
                      "--device-id", "legacy-browser"])
        with self.assertRaisesRegex(MOD.Stop, "BACKEND_ID_INVALID"):
            MOD.main(["--base-url", "https://server.example/vdr-suite",
                      "--device-id", "device_" + "e" * 32,
                      "--backend-id", "../private"])


if __name__ == "__main__":
    unittest.main()
