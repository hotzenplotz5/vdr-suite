#!/usr/bin/env python3
"""MT.HA.0 regression: first-install provenance depends on official main, not a deleted work branch.

All git operations take place inside TemporaryDirectory fixtures. Never consult
or change the live VDR checkout, installed services, packages, or databases.
"""
from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("install_media_tools.sh")
LEGACY_REF = "refs/heads/work/addon-platform-registry-lifecycle"
MAIN_REF = "refs/heads/main"


def git(*arguments: str, cwd: Path) -> str:
    result = subprocess.run(
        ["git", *arguments], cwd=cwd, check=True, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=20,
    )
    return result.stdout.strip()


@unittest.skipUnless(shutil.which("git"), "git unavailable")
class ManualInstallerProvenanceTests(unittest.TestCase):
    def test_installer_only_fetches_official_main(self) -> None:
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertNotIn(LEGACY_REF, source)
        self.assertEqual(source.count(MAIN_REF), 2)
        self.assertIn('git merge-base --is-ancestor "$SHA" "$REMOTE"', source)
        self.assertIn('git rev-parse FETCH_HEAD', source)

    def test_main_only_remote_is_sufficient_for_ancestry_check(self) -> None:
        with tempfile.TemporaryDirectory(prefix="mt-ha-origin-") as scratch:
            root = Path(scratch)
            remote = root / "remote.git"
            source = root / "source"
            checkout = root / "checkout"
            git("init", "--bare", str(remote), cwd=root)
            source.mkdir()
            git("init", "-b", "main", cwd=source)
            git("config", "user.name", "MT HA Test", cwd=source)
            git("config", "user.email", "mt-ha@example.invalid", cwd=source)
            git("commit", "--allow-empty", "-m", "approved main commit", cwd=source)
            approved = git("rev-parse", "HEAD", cwd=source)
            git("remote", "add", "origin", str(remote), cwd=source)
            git("push", "origin", "main", cwd=source)
            checkout.mkdir()
            git("init", cwd=checkout)
            git("remote", "add", "origin", str(remote), cwd=checkout)
            git("fetch", "--quiet", "--no-tags", "origin", MAIN_REF, cwd=checkout)
            main_head = git("rev-parse", "FETCH_HEAD", cwd=checkout)
            self.assertEqual(approved, main_head)
            git("merge-base", "--is-ancestor", approved, main_head, cwd=checkout)
            self.assertEqual("", git("ls-remote", "--heads", "origin", LEGACY_REF, cwd=checkout))

    def test_unrelated_commit_is_not_accepted_as_main_ancestor(self) -> None:
        with tempfile.TemporaryDirectory(prefix="mt-ha-ancestry-") as scratch:
            root = Path(scratch)
            git("init", "-b", "main", cwd=root)
            git("config", "user.name", "MT HA Test", cwd=root)
            git("config", "user.email", "mt-ha@example.invalid", cwd=root)
            git("commit", "--allow-empty", "-m", "official main", cwd=root)
            official = git("rev-parse", "HEAD", cwd=root)
            git("checkout", "--orphan", "unreviewed", cwd=root)
            git("commit", "--allow-empty", "-m", "unrelated candidate", cwd=root)
            unreviewed = git("rev-parse", "HEAD", cwd=root)
            result = subprocess.run(
                ["git", "merge-base", "--is-ancestor", unreviewed, official],
                cwd=root, check=False, capture_output=True, text=True, timeout=20,
            )
            self.assertEqual(1, result.returncode)


if __name__ == "__main__":
    unittest.main()
