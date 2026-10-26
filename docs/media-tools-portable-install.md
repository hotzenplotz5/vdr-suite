# Media Tools – portable staged installation and adoption

This is the **add-on payload only**. It is neither a VDR-Suite full-runtime
upgrade nor an init-system adapter. It uses Python 3, GNU Make, an ELF-capable
C++ toolchain and the already existing Suite stage/seal contract; the file
deployment itself has no dependency on systemd, dpkg, Ubuntu or MLD tooling.
**Cross-distribution ABI, path compatibility and runtime activation are not
certified by this command.** The installation paths are currently the Suite
FHS paths under `/usr`, not arbitrary `PREFIX` installations.

## Reuse the installed Suite stage — never rebuild the daemon

The three worker executables are built separately with
`make addon-media-import addon-media-check addon-media-shrink`. Existing
builds can be reused with **no C++ compilation**. The portable prepare
step **requires an existing, sealed Suite stage** from the original runtime
installation and refuses to rebuild the daemon:

```sh
make prepare-media-tools \
    MEDIA_TOOLS_BASE_STAGE=/absolute/existing/sealed/Suite/stage \
    MEDIA_TOOLS_WORKER_SOURCE_COMMIT=EXACT_40_HEX_BUILD_COMMIT \
    MEDIA_TOOLS_WORKDIR=/absolute/new/prepared-workdir
```

The default worker source is `$(BUILD_DIR)/addons` (typically
`.build/addons`); override with `MEDIA_TOOLS_WORKER_DIR` if needed.
Preparation verifies the previous Suite seal, checks that
`MEDIA_TOOLS_WORKER_SOURCE_COMMIT` is an ancestor of the current checkout
and that the worker source/header/add-on manifest have not changed between
the two commits, then copies the three already built ELF workers and
prints their import-worker SHA256. The live host stays unchanged. Never overwrite a partial or previous workdir.

After independent review of that **exact** handler digest, seal the stage:

```sh
make stage-media-tools \
    MEDIA_TOOLS_WORKDIR=/absolute/new/prepared-workdir \
    APPROVED_HANDLER_SHA256=REVIEWED_NEW_64_HEX_DIGEST
```

The stage contains both Suite and optional Media Tools for verification,
but the **payload-only** installer deploys ONLY the add-on-owned allowlist,
not the previous Suite daemon, backend agent or frontend.

### First installation or identical worker adoption

```sh
sudo make install-media-tools \
    MEDIA_TOOLS_WORKDIR=/absolute/new/prepared-workdir \
    APPROVED_HANDLER_SHA256=REVIEWED_NEW_64_HEX_DIGEST
```

The unchanged root approval is never implicitly granted, rewritten or
revoked. Identical existing manual workers can be adopted by installing
the missing versioned inventory. Repeated identical installs are no-ops.
Package-managed files are rejected, not overlaid by the source installer.

### Update of an existing *manually approved* older worker

Changing the approved import worker must be an explicit separate operator
decision. Before invoking this, independently confirm that the Agent and
all Media Tools execution are quiescent, no add-on job is active and the
previous grant is genuinely the old installed handler. This is **not**
automatically established by the installer. The two exact SHA256 values
come from independent review and the old installed root approval,
respectively.

```sh
sudo env MEDIA_TOOLS_QUIESCED=YES \
    MEDIA_TOOLS_APPROVAL_ROTATION=REVIEWED_UPDATE \
    make install-media-tools \
    MEDIA_TOOLS_WORKDIR=/absolute/new/prepared-workdir \
    APPROVED_HANDLER_SHA256=REVIEWED_NEW_64_HEX_DIGEST \
    PREVIOUS_APPROVED_HANDLER_SHA256=REVIEWED_OLD_64_HEX_DIGEST
```

Only an existing canonical **manual** root grant whose digest matches
both the expected old SHA and installed old handler may be rotated.
The add-on lock must already exist and must be exclusively acquired.
Backend-specific grants block automatic rotation, because their owner
must migrate them separately. An inactive or unavailable runtime should
never be silently marked ready. Installation neither starts services nor
automatically activates the add-on.

The previous root approval is backed up together with the add-on files,
and the new canonical record is installed **last**, after all new worker
bytes have been copied and verified. On an ordinary error the tool
attempts to roll back both approval and payload. The service remains in
the operator-selected state. The root grant is never changed on a
normal install; only this explicit rotation path may change it.

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
