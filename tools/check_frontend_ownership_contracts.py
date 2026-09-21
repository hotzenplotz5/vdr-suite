#!/usr/bin/env python3
"""Stable entry point for current frontend ownership contracts.

The current ownership implementation still expresses the direct-daemon asset
markers with absolute `/frontend/...` paths. Slice 3A makes the HTML bootstrap
relative so the same document also works below `/vdr-suite`. Normalize only the
index document presented to the legacy ownership checks; the dedicated public-
origin architecture check validates the real relative paths.
"""

import frontend_ownership_contracts_core as core
import frontend_ownership_contracts_current as current


_original_read = current.read
_original_combined_install_source = current.combined_install_source


def read_with_direct_daemon_paths(path):
    text = _original_read(path)
    if path == current.FRONTEND / "index.html":
        return (
            text.replace("../frontend/", "/frontend/")
            .replace("../channel-logos/", "/channel-logos/")
        )
    return text


current.read = read_with_direct_daemon_paths
core.read = read_with_direct_daemon_paths


def combined_install_source_with_public_origin():
    return (
        _original_combined_install_source()
        + "\n"
        + _original_read(current.ROOT / "mk/public-origin.mk")
    )


current.combined_install_source = combined_install_source_with_public_origin


if __name__ == "__main__":
    raise SystemExit(current.main())
