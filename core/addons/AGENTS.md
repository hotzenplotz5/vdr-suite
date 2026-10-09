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
