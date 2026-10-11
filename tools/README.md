# VDR-Suite tools

For a daemon-only update, use **tools/install_daemon.py** and the review procedure in **docs/development/daemon-only-installer.md**.

The installer separates a read-only check, persistent build and explicit installation. It does not silently rebuild a candidate, back up a database or delete finished binaries.

For coordinated daemon/frontend releases, the existing **stage-install-runtime**, **deploy-install-runtime** and **check-install-runtime-deployment** Makefile targets remain authoritative.
