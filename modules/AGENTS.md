# Agent rules: optional module platform

Root [AGENTS.md](../AGENTS.md) applies throughout `modules/`. Each module
**must** also have its own `AGENTS.md`, `README.md` and `addon.json`;
those specific instructions supplement, rather than override, these rules.

- Any package-derived metadata is untrusted; a manifest may neither grant
  rights nor declare a module running. A module registry is not a code loader.
- The owning VDR-Suite daemon/Agent decides actor permissions, backend scope,
  accepted capabilities, operation fencing and resource lifecycle.
- Package installation and administrator activation are distinct: packages
  install disabled; activation must fail closed until the registered handler,
  dependency/compatibility, actor and backend gates exist.
- Modules remain individually stageable/packageable, with explicit source and
  install contracts; do not add them implicitly to the base `make install`.
- Update each affected module's README, AGENTS and manifest when its runtime
  or package contract changes. Keep tests and higher-level ADRs consistent.
- Do not install, restart or mutate live VDR files without explicit consent
  and a host-specific read-only resource preflight.

- The `core/addons` policy evaluator is not a replacement for the canonical `RequestSecurityContext` grant resolution or `BackendAccessPolicy` write gate. Do not expose it to the HTTP router, fake package provenance, or claim a live handler before those gates are wired and verified.
