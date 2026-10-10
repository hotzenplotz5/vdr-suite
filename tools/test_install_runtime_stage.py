#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

TOOL = Path(__file__).with_name("install_runtime_stage.py")


def write(path: Path, content: str, mode: int = 0o644) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    os.chmod(path, mode)


def mkdir(path: Path, mode: int) -> None:
    path.mkdir(parents=True, exist_ok=True)
    os.chmod(path, mode)


def mode(path: Path) -> int:
    return stat.S_IMODE(path.stat().st_mode)


def run(*args: str, expect: int = 0) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        [sys.executable, str(TOOL), *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if completed.returncode != expect:
        raise AssertionError(
            f"unexpected rc={completed.returncode}, expected={expect}: {' '.join(args)}\n{completed.stdout}"
        )
    return completed


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="vdr-suite-runtime-deploy-test-") as tmp:
        root = Path(tmp)
        stage = root / "stage"
        live = root / "live"

        write(stage / "usr/sbin/vdr-suite-daemon", "daemon-v2\n", 0o755)
        write(stage / "etc/vdr-suite/backend-agent.conf", "DEFAULT=1\n")
        write(stage / "usr/share/vdr-suite/web/frontend/index.html", "index-v2\n")
        write(stage / "usr/share/vdr-suite/web/frontend/app.js", "app-v2\n")
        write(stage / "usr/share/vdr-suite/web/frontend/composed.js", "part-a\npart-b\n")
        mkdir(stage / "var/lib/vdr-suite/backend-agent", 0o700)
        mkdir(stage / "var/lib/vdr-suite/secrets/series-artwork", 0o700)
        # Reproduce a staging tree built with umask 077. Those
        # ancestor modes must NEVER alter existing system folders.
        for rel in ("usr", "usr/bin", "usr/sbin", "usr/share",
                    "var", "var/lib", "var/cache", "var/lib/vdr-suite"):
            mkdir(stage / rel, 0o700)

        sealed = run("seal", "--stage-root", str(stage))
        assert "INSTALL_RUNTIME_STAGE_SEALED=YES" in sealed.stdout
        assert "DIRS=" in sealed.stdout

        write(live / "usr/sbin/vdr-suite-daemon", "daemon-v2\n", 0o755)
        write(live / "etc/vdr-suite/backend-agent.conf", "SITE_LOCAL=keep\n")
        write(live / "usr/share/vdr-suite/web/frontend/index.html", "index-v2\n")
        write(live / "usr/share/vdr-suite/web/frontend/app.js", "app-v1-stale\n")
        write(live / "usr/share/vdr-suite/web/frontend/stale-only.js", "stale\n")
        mkdir(live / "usr/share/vdr-suite/web/frontend/stale-empty-dir", 0o755)
        for rel in ("usr", "usr/bin", "usr/sbin", "usr/share",
                    "var", "var/lib", "var/cache", "var/lib/vdr-suite"):
            mkdir(live / rel, 0o755)

        drift = run("check", "--stage-root", str(stage), "--live-root", str(live), expect=1)
        assert "RUNTIME_DEPLOYMENT_DRIFT=content:usr/share/vdr-suite/web/frontend/app.js" in drift.stdout
        assert "RUNTIME_DEPLOYMENT_DRIFT=missing:usr/share/vdr-suite/web/frontend/composed.js" in drift.stdout
        assert "RUNTIME_DEPLOYMENT_DRIFT=extra:usr/share/vdr-suite/web/frontend/stale-only.js" in drift.stdout
        assert "RUNTIME_DEPLOYMENT_DRIFT=extra-dir:usr/share/vdr-suite/web/frontend/stale-empty-dir" in drift.stdout
        assert "RUNTIME_DEPLOYMENT_DRIFT=missing-dir:var/lib/vdr-suite/backend-agent" in drift.stdout
        assert "RUNTIME_DEPLOYMENT_DRIFT=missing-dir:var/lib/vdr-suite/secrets/series-artwork" in drift.stdout

        deployed = run("deploy", "--stage-root", str(stage), "--live-root", str(live))
        assert "INSTALL_RUNTIME_DEPLOYED=YES" in deployed.stdout
        assert (live / "etc/vdr-suite/backend-agent.conf").read_text(encoding="utf-8") == "SITE_LOCAL=keep\n"
        assert (live / "usr/share/vdr-suite/web/frontend/composed.js").read_text(encoding="utf-8") == "part-a\npart-b\n"
        assert not (live / "usr/share/vdr-suite/web/frontend/stale-only.js").exists()
        assert not (live / "usr/share/vdr-suite/web/frontend/stale-empty-dir").exists()
        assert (live / "var/lib/vdr-suite/backend-agent").is_dir()
        assert mode(live / "var/lib/vdr-suite/backend-agent") == 0o700
        assert (live / "var/lib/vdr-suite/secrets/series-artwork").is_dir()
        assert mode(live / "var/lib/vdr-suite/secrets/series-artwork") == 0o700
        for rel in ("usr", "usr/bin", "usr/sbin", "usr/share",
                    "var", "var/lib", "var/cache", "var/lib/vdr-suite"):
            assert mode(live / rel) == 0o755, f"installer changed system permissions: {rel}"

        matched = run("check", "--stage-root", str(stage), "--live-root", str(live))
        assert "RUNTIME_DEPLOYMENT_MATCH=YES" in matched.stdout

        # A pre-existing private directory with unsafe permissions must
        # fail closed without repairing or replacing live files.
        private_dir = live / "var/lib/vdr-suite/secrets"
        os.chmod(private_dir, 0o755)
        rejected = run("deploy", "--stage-root", str(stage),
                       "--live-root", str(live), expect=1)
        assert "private runtime directory needs explicit repair" in rejected.stdout
        assert mode(private_dir) == 0o755
        os.chmod(private_dir, 0o700)
        assert "RUNTIME_DEPLOYMENT_MATCH=YES" in run(
            "check", "--stage-root", str(stage), "--live-root", str(live)
        ).stdout

        write(stage / "usr/share/vdr-suite/web/frontend/app.js", "tampered-after-seal\n")
        tampered = run("check", "--stage-root", str(stage), "--live-root", str(live), expect=1)
        assert "sealed stage content changed" in tampered.stdout

    print("install-runtime deployment guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
