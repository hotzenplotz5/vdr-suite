#!/usr/bin/env python3
"""P8 entry point: integrate the P6 host scan before any future install writes.

Until approved transition and package-install ownership policies exist, no
install, build or uninstall operation is dispatched by this entry point.
"""
import argparse
import json
from pathlib import Path
import sys

from packaging_host_collision_preflight import inspect as inspect_host


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--check", action="store_true",
                        help="read-only host inspection; never authorizes install")
    action.add_argument("--install", action="store_true",
                        help="enforce host inspection before installation (currently blocked)")
    action.add_argument("--build", action="store_true",
                        help="reserved P8 build interface (not implemented)")
    action.add_argument("--uninstall", action="store_true",
                        help="reserved P8 uninstall interface (not implemented)")
    parser.add_argument("--root", type=Path, default=Path("/"),
                        help="inspection root; use a temporary fixture for offline tests")
    args = parser.parse_args(argv)

    if args.build or args.uninstall:
        # Build/staging already belongs to debian/rules and mk/install.mk.
        # Do not introduce a competing package builder or live uninstaller.
        print("P8_ACTION=BLOCKED: no approved P8 " +
              ("build" if args.build else "uninstall") +
              " implementation", file=sys.stderr)
        return 3

    try:
        observation = inspect_host(args.root)
    except (OSError, ValueError) as exc:
        print("P8_HOST_PREFLIGHT=BLOCKED: " + str(exc), file=sys.stderr)
        return 2

    if args.check:
        print(json.dumps(observation, indent=2, sort_keys=True))
        # Diagnostic success is not approval. Fail closed even for a clean
        # host, so a shell chain cannot mistake this for permission to install.
        return 2 if observation["verdict"] == "BLOCKED" else 3

    # The P6 result always has install_approved=False. Even an empty scan
    # is not a release or migration approval. There is deliberately no
    # subprocess/apt/dpkg/staging/deploy fallback beyond this point.
    outcome = {
        "schema": 1,
        "action": "install",
        "decision": "BLOCKED",
        "install_approved": False,
        "host_preflight_verdict": observation["verdict"],
        "host_preflight_checks": observation["checks"],
        "reason": "P8 transition policy and direct dpkg protection not yet approved",
        "read_only": True,
    }
    print(json.dumps(outcome, indent=2, sort_keys=True))
    return 3


if __name__ == "__main__":
    sys.exit(main())
