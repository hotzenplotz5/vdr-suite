# Media Tools – portable staged installation and adoption

This is the **add-on payload only**. It is neither a VDR-Suite full-runtime
upgrade nor an init-system adapter. It uses Python 3, GNU Make, an ELF-capable
C++ toolchain and the already existing Suite stage/seal contract; the file
deployment itself has no dependency on systemd, dpkg, Ubuntu or MLD tooling.
**Cross-distribution ABI, path compatibility and runtime activation are not
certified by this command.** The installation paths are currently the Suite
FHS paths under `/usr`, not arbitrary `PREFIX` installations.

## From a reviewed, unprivileged source checkout

The first step is **not run with sudo**. Use a **separately reviewed** exact
64-character import-handler SHA256 from a candidate whose source/manifest
you have independently verified. Do not blindly copy a newly printed digest
and consider it approval.

```sh
make stage-media-tools APPROVED_HANDLER_SHA256=REVIEWED_64_HEX_DIGEST
```

This builds the Suite runtime and the three add-on workers in the isolated
`.build/media-tools-prepared` directory. It creates a complete sealed
`stage-with-media-tools` with a versioned add-on file inventory. The
preparer refuses to overwrite a preexisting work directory. To prepare a new
version, choose a new `MEDIA_TOOLS_WORKDIR`, not an old stage to overwrite.

After separately confirming that the backend Agent has no active work and
the handler approval is consistent, the root-only step is:

```sh
sudo make install-media-tools APPROVED_HANDLER_SHA256=REVIEWED_64_HEX_DIGEST
```

The install target **does not build**, stop/start/enable services or alter
VDR-Suite binaries. It verifies the complete sealed stage and copies only
the exact allowlisted Media Tools metadata, inventory and three workers
to their FHS locations. It preserves every file under `/etc`, the
SQLite database/WAL, Suite frontend, recordings and all installed
permissions of existing directories. Root approval is **not issued or
rewritten**, and activation remains an independent step.

**Existing manual installs:** matching worker bytes can be adopted; a
missing versioned inventory can be installed without replacing those
workers. Repeating the install with identical files returns
`MEDIA_TOOLS_INSTALL=ALREADY_CURRENT`. A package-managed payload is not
silently overlaid by this manual installer.

**Different worker bytes:** the installer refuses any live worker
replacement without a separate operator statement that the Media Tools
runtime is quiescent (`MEDIA_TOOLS_QUIESCED=YES`). This is an
operator attestation, **not an automated proof that all work has drained**.
An existing root approval must already refer to the new approved import
handler digest; it is never silently rewritten to permit executable code.
Operators must independently arrange safe service/agent quiescence and
freigabe, appropriate to the target system, before setting this
environment variable. A mismatch fails closed.

Before writing, the tool freezes reviewed add-on bytes and copies the
previous add-on files to a newly created private backup directory. It
uses per-file atomic replacement, performs post-write SHA256 verification
and attempts to restore already changed files if the operation fails.
Backups are retained and their paths printed; no automatic service
restart occurs. A system crash during installation requires inspection
of the recorded private backup before recovery. This is deliberately
**not** a promise of atomic all-file or cross-filesystem transactions.

To install into a disposable root for testing, supply `LIVE_ROOT=/absolute/path`:

```sh
make install-media-tools \
    MEDIA_TOOLS_STAGE=/absolute/sealed/stage-with-media-tools \
    APPROVED_HANDLER_SHA256=REVIEWED_64_HEX_DIGEST \
    LIVE_ROOT=/absolute/disposable/root
```

This does **not** package Media Tools for the distro, install or repair
systemd units, create import-source configuration, activate an Agent,
migrate historical P7 state, or verify real VDR readback. Those remain
separate acceptance gates.

```sh
make test-install-media-tools-payload
```

Tests exercise synthetic roots only and do not touch the real VDR host.
