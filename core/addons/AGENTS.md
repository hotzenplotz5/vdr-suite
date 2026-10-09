# Agent rules: Core Add-on Access Policy

Inherit repository root [AGENTS.md](../../AGENTS.md) and architectural
constraints in [ADR-0068](../../docs/adr/ADR-0068-optional-addons-and-source-packaging.md).

1. Core is the authority for permissions, canonical BackendAccessPolicy,
   operation identity, handler registration, provenance and enable/disable.
2. Never interpret a module-provided capability, script path or permission
   request as sufficient to activate executable code.
3. A resolved global role.admin grant may authorize reading administrative
   inventory, but it must not authorize media import or other mutation.
4. Media Tools import needs an independent backend-scoped right; no generic
   recordings.execute or admin override is allowed.
5. Do not connect the standalone policy evaluator to the public API or
   executable import workflow until canonical grant vocabulary, durable
   administrator intent, handler trust and media reconciliation are ready.
6. Validate the exact candidate with make test-addon-access-policy without
   live media, daemon deployment, installation or service restarts.

7. The `AddonActivationIntentRepository` stores only administrative desired
   state, NEVER effective activation. Mutations require canonical resolved
   global-administrator grants and an exact optimistic revision; enabling
   additionally requires independent trusted package/handler/backend evidence.
   Do not wire a public route or runtime hook to this repository without
   explicit authoritative provenance, audit, migrations, lifecycle and tests.
8. Never let disabled/uninstalled modules become active merely because an
   old SQLite desired-enable row survives a restart or package upgrade.
   Loss of any trust, handler health, permission or backend authority must
   independently force effective capabilities to empty.
9. Run `make test-addon-activation-intent` with an isolated build directory
   and explicitly assigned `TMPDIR`; it creates only a temporary SQLite DB.

10. The internal `AddonAdmissionReadService` always replaces any caller
    claim of `administratorEnabled` with the persisted per-backend desired
    state. Missing schema, invalid state or SQLite failure must deny, never
    fall back to a caller-provided true flag. It is diagnostic-only:
    `AdmissionPreview.executable` remains false; no router/worker may use a
    positive preview as execution authorization. Recheck authoritative state
    with generation and revision fencing for any future actual operation.
