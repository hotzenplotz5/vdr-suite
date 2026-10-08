# Installed Add-on Registry and Activation-Denial Contract

Status: implemented **read-only, no production activation**.

## Boundary

The source tree contains four optional add-on products; a browser-side frontend registration does not imply server authorization. The first installed registry is an isolated operator/developer diagnostic implemented by `tools/addons/addon_registry.py`. It is **not wired to the daemon**, has no external HTTP endpoint, cannot install or execute packages and cannot grant any user permission.

Package metadata lives in `/usr/share/vdr-suite/addons/<id>/addon.json`, `README.md`, `AGENTS.md`. These files originate from the per-module `DESTDIR` source build, not from user-provided HTTP data. Administrator desired-state files, when provisioned by a separate trusted deployment mechanism, have this exact format in `/etc/vdr-suite/addons-enabled.d/<id>.json`:

```json
{"schemaVersion":1,"module":"rectools","enabled":true}
```

**No provisioning tool or service mutates that directory in this slice.** An enabled entry is only an *intent*. Present schema-v1 manifests are `scaffold`: no runtime entrypoints, permissions, dependencies or executable capabilities may be declared. Every module remains `effectiveEnabled: false`, even when the desired-state file says true.

## Trust and status

Read-only CLI: `python3 tools/addons/addon_registry.py --json` (uses fixed system roots; missing directories yield empty inventory). Source tests: `make check-addon-registry`. No internet, user database, service stop or installed daemon is needed.

Validation requires: bounded regular manifest/document files; a conforming schema-v1 manifest; the directory identifier matching manifest ID; unique package identity; no symlinks or group/other-writable installed metadata; administrator policy with strict typed JSON; no unknown fields or duplicate JSON keys; and the default inactive state when policy is absent. Bad data yields `invalid_or_untrusted` and **never** grants an effective capability. The source-defined registry does not verify Debian package signatures/dpkg ownership; file ownership checks are **necessary, not sufficient** for trust.

Example diagnostic output (from an isolated test package, not a claim about the live machine):

```json
{
  "schemaVersion": 1,
  "enabledCapabilities": [],
  "modules": [
    {
      "id": "rectools",
      "installed": true,
      "version": "0.1.0",
      "package": "vdr-suite-addon-media-tools",
      "requestedEnabled": true,
      "effectiveEnabled": false,
      "capabilities": [],
      "reason": "scaffold_not_executable"
    }
  ]
}
```

`installed` currently denotes valid recognized **files** under the managed metadata directory, **not** proof that dpkg registered the package. A later runtime owner must verify package manager/trusted distribution provenance, matching executable install paths and manifests. No status response may be treated as sufficient for authorization.

## Requirements for actual activation and UI discovery

Activation is an AND condition, never a `frontend.registerModule()` or config checkbox shortcut:

1. Genuine module package verified as trusted and compatible with Suite API plus architecture; module manifest v2 specifies stable capability and dependency contracts.
2. Administrator has allowed it and explicitly approved any changes to package/runtime dependencies.
3. Owning backend is present and healthy for the requested operation; source roots and scope are registered, and unsupported backends fail closed.
4. An actual restricted worker/adapter exists, with bounded execution and durable job/state/error/retry fencing, recording reconciliation and audit.
5. **Each request** is authorized against canonical account/device grants and target backend/scope; "addon installed" never grants `recordings.import` or other rights.
6. Client capability discovery only exposes rights-resolved functions, with no raw command, filesystem or secret paths; server enforces the same check on every call.
7. Disable/upgrade/removal reconcile running operations and handle old versions/revisions safely; no in-place untrusted dynamic C++ ABI or uncontrolled script loading.

Rights must be added through existing canonical grant vocabulary and migrations, not a parallel add-on permission database. No web management controls, public API routes or executor entrypoints should be advertised until the applicable acceptance tests prove these gates.

## Next implementation dependencies

A production **C++ core add-on capability/availability service** and a read-only public-v1 API facade must consume a trusted, versioned registry through the *same* authorization pipeline as existing client APIs, not execute this Python CLI as the request handler. Follow with secured admin activation mutation (optimistic revision, idempotency, audit, dependency validation) and an independently packageable worker. The C++ Media Tools preview remains deliberately outside base `make install`.

Module-specific rules remain in `modules/{rectools,image,music,tvscraper}/AGENTS.md`; all changes require per-module README and manifest alignment.
