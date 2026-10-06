# Install-Runtime Deployment Reliability Contract

Status: **Binding operational contract for repository-built yaVDR runtime acceptance.**

This contract closes the gap between a correct repository/CI state and a partially
updated live installation. It does not introduce Debian packaging and it does not
change the future package-productization roadmap.

## Problem

The source tree is not the installed runtime tree.

`web/frontend` contains source-only tests and inputs, while `install-runtime`
also composes or aliases deployable frontend files through the root
`Makefile` and additive `mk/*.mk` install hooks. A direct source-to-live copy
can therefore leave a current `index.html` next to stale or missing runtime
assets even when source and CI are correct.

The canonical truth for deployable runtime files is the output of
`install-runtime` in a clean `DESTDIR`, never the raw source tree.

## Canonical workflow

From the intended repository head:

```bash
make stage-install-runtime RUNTIME_STAGE=/tmp/vdr-suite-runtime-stage
sudo make deploy-install-runtime \
  RUNTIME_STAGE=/tmp/vdr-suite-runtime-stage \
  LIVE_ROOT=/
make check-install-runtime-deployment \
  RUNTIME_STAGE=/tmp/vdr-suite-runtime-stage \
  LIVE_ROOT=/
```

`stage-install-runtime`:

1. builds the current runtime binaries through the existing runtime build graph;
2. removes the previous staging root;
3. invokes the existing `install-runtime` target with `DESTDIR` and
   `PREFIX=/usr`, including every additive install hook;
4. seals the resulting tree with a SHA-256/mode manifest.

The seal prevents a staging tree from being modified between staging and
deployment without detection.

`deploy-install-runtime` consumes only the sealed staging tree. It does not
read deployable frontend files directly from `web/frontend`.

Existing files below `/etc` are preserved, matching the repository's
non-destructive configuration-upgrade rule. Other regular install-runtime files
are replaced from staging.

The staged frontend tree at
`/usr/share/vdr-suite/web/frontend` is deployed as one complete replacement
tree. Stale live-only files are removed by that replacement. This prevents the
known half-upgraded state where only selected frontend files were copied.

## Required runtime acceptance

Repository/CI success is not sufficient evidence that yaVDR is running the same
runtime candidate.

After every repository-built runtime/frontend installation used for acceptance,
`check-install-runtime-deployment` is mandatory. Acceptance is valid only when
it prints:

```text
RUNTIME_DEPLOYMENT_MATCH=YES
```

The check compares the sealed install-runtime manifest with the live
installation. For the frontend it additionally requires an exact file-set match,
so missing, stale and extra frontend artifacts fail the check.

Do not use `find web/frontend` versus the live frontend as deployment evidence.
That comparison mixes source-only and composed runtime artifacts and is not an
install contract.

Do not manually cherry-pick frontend files from the source tree into
`/usr/share/vdr-suite/web/frontend`.

For a frontend-only candidate, a daemon restart is not implied by this contract.
Browser acceptance must load a fresh document/runtime after the deployment.
If runtime binaries changed, service restart and the corresponding runtime
acceptance remain separately governed by the changed slice.

## Regression guard

`tools/test_install_runtime_stage.py` reproduces the discovered failure class:

- current `index.html`;
- stale frontend content;
- a missing composed frontend artifact;
- an extra stale live-only frontend artifact.

The guard proves that the pre-deployment check fails, deployment from the sealed
stage repairs the live tree, the post-deployment check passes, existing
configuration remains unchanged, and any post-seal staging mutation is rejected.

The guard is part of `test-ci-packaging`.

## Non-goals

This slice does not add:

- `debian/` metadata;
- package maintainer scripts;
- package upgrade/remove/purge policy;
- automatic daemon restart or activation;
- a second frontend build/composition path;
- Phase 70 or another Multiuser slice.

The existing ADR-0037 staged-install boundary remains authoritative; this
contract adds the missing safe live-deployment and acceptance discipline on top
of that boundary.
