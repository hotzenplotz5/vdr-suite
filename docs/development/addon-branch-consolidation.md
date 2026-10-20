# Add-on branch consolidation and canonical ownership

Status: consolidation of historical add-on branches, 2026-10-20.
This document is a source-tree integration record, **not** a claim that the
add-on runtime is enabled or installed.

## Single development branch

All future Add-on Architecture / Media Tools / Registry / image / music /
tvscraper slices continue directly on
`work/addon-platform-registry-lifecycle`. No more per-slice successor
branches are necessary. Synchronize it with current `main` using regular
non-force merges without changing `main`.

Historical development branches to be absorbed by ancestry:

- `work/addon-architecture-foundation` (already ancestral)
- `work/addon-registry-contract`
- `work/addon-platform-registry`

The older registry branches contain a second, offline
`tools/addons/addon_registry.py` and tests for it. That prototype uses
filesystem configuration under
`/etc/vdr-suite/addons-enabled.d/<id>.json` to describe hypothetical
opt-in. It was not connected to the daemon or an execution adapter.

## Reconciliation of conflicting registry designs

**Do not deploy both registry variants or combine their enable switches.**
The *current* developer-only installed metadata inventory is
`tools/addons/installed_registry.py`, with a reviewed core-owned package
identity list and always-empty effective capabilities. Its metadata file
presence is NOT proof of Debian package ownership or trust.

The newer internal C++ desired-state owner is
`AddonActivationIntentRepository` (SQLite, backend-scoped, revision-fenced)
with `AddonAdmissionReadService` (non-executing fail-closed preview).
It is not connected to public API, Daemon or Backend Agent and must not
enable a module by itself. No runtime should consult the historical
file-based opt-in, and no automatic migration is authorized.

The old registry implementation and tests remain recoverable through the
two merge parents in Git history, but the superseded duplicate files are
**not** introduced as active sources into the consolidated tip. Its useful
filesystem owner/mode checks are prospective hardening inputs for the single
canonical inventory, not a second authority or a trusted package signature.

The optional C++ Media Tools import prototype remains inert and subject to
the source containment, actor, backend, package provenance, bounded-worker,
operation fencing and recording reconciliation acceptance gates. Base
`make install` must never stage add-ons implicitly.

## Branch cleanup gate

Only delete the three superseded remote add-on refs after the current
consolidated branch **contains each exact old tip by ancestry**, the changed
surface has passed isolated tests, and no open PR, workflow, Launchpad recipe,
release or protected/deployment consumer depends on their names. Deletion
does not change commit history or delete `main`.

The continued active branch is not a request to merge to `main`.
