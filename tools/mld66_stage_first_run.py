#!/usr/bin/env python3
"""Compatibility entrypoint for MLD package recipes.

The first-run configuration implementation is shared by all distributions.
Keep the established MLD command and output contract for existing recipes.
"""
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


def main():
    return _shared_main(status_prefix="MLD_FIRST_RUN_STAGE")


if __name__ == "__main__":
    raise SystemExit(main())
