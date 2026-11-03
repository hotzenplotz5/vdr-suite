#!/usr/bin/env python3
"""Compatibility entrypoint for MLD package recipes.

The first-run configuration implementation is shared by all distributions.
Keep the established MLD command and output contract for existing recipes.
"""
from pathlib import Path
import stat

from distribution_first_run_stage import (  # noqa: F401
    NotReady,
    stage,
    checked_staging_file,
    validated_origin,
    validated_channels,
    validated_ca_certificate,
    settings,
    substitute,
    update_atomically,
    main as _shared_main,
)


def package_preflight(stage_root):
    """Reject a staged MLD package missing the existing first-run contract.

    Run before modifying either staged config file. The actual MLD recipe
    must invoke this established staging target after installing runtime
    binaries and all shared systemd units.
    """
    root = Path(stage_root)
    if (not root.is_absolute() or root.is_symlink() or
            root.resolve() == Path("/") or not root.is_dir()):
        raise NotReady("a separate absolute MLD DESTDIR is required")
    helper = checked_staging_file(
        root, Path("usr/sbin/vdr-suite-backend-agent-autostart"))
    if not helper.stat().st_mode & stat.S_IXUSR:
        raise NotReady("staged first-run helper is not executable")
    units = ("vdr-suite-daemon.service", "vdr-suite-backend-agent.service",
             "vdr-suite-first-run.service", "vdr-suite-first-run.path")
    for unit_dir in (Path("usr/lib/systemd/system"),
                     Path("lib/systemd/system")):
        if all((root / unit_dir / name).is_file() for name in units):
            for name in units:
                checked_staging_file(root, unit_dir / name)
            return
    raise NotReady("MLD staged package missing shared first-run systemd units")


def main():
    return _shared_main(status_prefix="MLD_FIRST_RUN_STAGE",
                        package_preflight=package_preflight)


if __name__ == "__main__":
    raise SystemExit(main())
