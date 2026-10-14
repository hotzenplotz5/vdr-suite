#!/usr/bin/env python3
"""Contract: experimental package revisions used by isolated P7 upgrade."""
from pathlib import Path
import re
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
CHANGELOG = ROOT / "debian" / "changelog"
WORKFLOW = ROOT / ".github" / "workflows" / "packaging-plugin-experimental.yml"
PRIOR_COMMIT = "bc4623374a4df4c08118c424ea19aaf552a4964c"


class P7VersionedUpgradeContractTests(unittest.TestCase):
    def test_real_debian_version_order_and_app_version(self):
        releases = re.findall(r"^vdr-suite \(([^)]+)\) ", CHANGELOG.read_text(), re.M)
        self.assertGreaterEqual(len(releases), 2)
        self.assertEqual(releases[:2], ["0.14.1-1~exp2", "0.14.1-1~exp1"])
        if shutil.which("dpkg") is None:
            self.skipTest("dpkg unavailable")
        subprocess.run(["dpkg", "--compare-versions", releases[0], "gt", releases[1]],
                       check=True)

    def test_pinned_prior_commit_and_nonproduction_gate(self):
        source = WORKFLOW.read_text()
        self.assertIn("ref: " + PRIOR_COMMIT, source)
        self.assertIn("container: ubuntu:24.04", source)
        self.assertIn("p7-versioned-upgrade-noble:", source)
        self.assertIn("policy-rc.d", source)
        self.assertIn("P7_VERIFIED_BACKUP_AND_BINARY_DOWNGRADE=PASS", source)


if __name__ == "__main__":
    unittest.main()
