#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys
import tempfile

TOOL = Path(__file__).with_name("install_runtime_stage.py")


def write(path: Path, content: str, mode: int = 0o644) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    os.chmod(path, mode)


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

        sealed = run("seal", "--stage-root", str(stage))
        assert "INSTALL_RUNTIME_STAGE_SEALED=YES" in sealed.stdout

        write(live / "usr/sbin/vdr-suite-daemon", "daemon-v2\n", 0o755)
        write(live / "etc/vdr-suite/backend-agent.conf", "SITE_LOCAL=keep\n")
        write(live / "usr/share/vdr-suite/web/frontend/index.html", "index-v2\n")
        write(live / "usr/share/vdr-suite/web/frontend/app.js", "app-v1-stale\n")
        write(live / "usr/share/vdr-suite/web/frontend/stale-only.js", "stale\n")

        drift = run("check", "--stage-root", str(stage), "--live-root", str(live), expect=1)
        assert "RUNTIME_DEPLOYMENT_DRIFT=content:usr/share/vdr-suite/web/frontend/app.js" in drift.stdout
        assert "RUNTIME_DEPLOYMENT_DRIFT=missing:usr/share/vdr-suite/web/frontend/composed.js" in drift.stdout
        assert "RUNTIME_DEPLOYMENT_DRIFT=extra:usr/share/vdr-suite/web/frontend/stale-only.js" in drift.stdout

        deployed = run("deploy", "--stage-root", str(stage), "--live-root", str(live))
        assert "INSTALL_RUNTIME_DEPLOYED=YES" in deployed.stdout
        assert (live / "etc/vdr-suite/backend-agent.conf").read_text(encoding="utf-8") == "SITE_LOCAL=keep\n"
        assert (live / "usr/share/vdr-suite/web/frontend/composed.js").read_text(encoding="utf-8") == "part-a\npart-b\n"
        assert not (live / "usr/share/vdr-suite/web/frontend/stale-only.js").exists()

        matched = run("check", "--stage-root", str(stage), "--live-root", str(live))
        assert "RUNTIME_DEPLOYMENT_MATCH=YES" in matched.stdout

        write(stage / "usr/share/vdr-suite/web/frontend/app.js", "tampered-after-seal\n")
        tampered = run("check", "--stage-root", str(stage), "--live-root", str(live), expect=1)
        assert "sealed stage content changed" in tampered.stdout

    print("install-runtime deployment guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
